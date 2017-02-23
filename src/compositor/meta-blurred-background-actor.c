//
//  Copyright (C) 2015 Deepin Technology Co., Ltd.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <config.h>

#include <math.h>
#include <GL/gl.h>
#include <clutter/clutter.h>

#include "cogl-utils.h"
#include "clutter-utils.h"
#include <meta/errors.h>
#include <meta/screen.h>
#include <meta/meta-blurred-background-actor.h>
#include "meta-background-private.h"
#include "meta-cullable.h"

enum
{
    PROP_META_SCREEN = 1,
    PROP_MONITOR,
    PROP_BACKGROUND,
    PROP_RADIUS,
    PROP_ROUNDS,
};

typedef enum {
    CHANGED_BACKGROUND = 1 << 0,
    CHANGED_EFFECTS = 1 << 1,
    CHANGED_ALL = 0xFFFF
} ChangedFlags;

// "#extension GL_ARB_shader_texture_lod: enable\n"
static const char* gaussian_blur_global_definition =
"#ifdef GL_ARB_shader_texture_lod\n"
"#define texpick texture2DLod\n"
"#else\n"
"#define texpick texture2DLod\n"
"#endif\n";

// kernel[0] = radius, kernel[1-20] = offset, kernel[21-40] = weight
static const char* gaussian_blur_glsl_declarations =
"uniform float kernel[41];"
"uniform vec2 resolution;";

static const char* vs_code = 
"float lod = 0.0;"
"cogl_texel = texpick(cogl_sampler, cogl_tex_coord.st, lod) * kernel[21];"
"for (int i = 1; i < kernel[0]; i++) {"
    "cogl_texel += texpick(cogl_sampler, cogl_tex_coord.st - vec2(0.0, kernel[1+i]/resolution.y), lod) * kernel[21+i];"
    "cogl_texel += texpick(cogl_sampler, cogl_tex_coord.st + vec2(0.0, kernel[1+i]/resolution.y), lod) * kernel[21+i];"
"}";

static const char* vs_code_h =
"float lod = 0.0;"
"vec2 tc = vec2(cogl_tex_coord.s, cogl_tex_coord.t);"
"cogl_texel = texpick(cogl_sampler, tc, lod) * kernel[21];"
"for (int i = 1; i < kernel[0]; i++) {"
    "cogl_texel += texpick(cogl_sampler, tc + vec2(kernel[1+i]/resolution.x, 0.0), lod) * kernel[21+i];"
    "cogl_texel += texpick(cogl_sampler, tc - vec2(kernel[1+i]/resolution.x, 0.0), lod) * kernel[21+i];"
"}";


static void build_gaussian_blur_kernel(int* pradius, float* offset, float* weight)
{
    int radius = *pradius;
    radius += (radius + 1) % 2;
    int sz = (radius+2)*2-1;
    int N = sz-1;
    float sigma = 1.5f;

    float sum = powf(2, N);
    weight[radius+1] = 1.0;
    for (int i = 1; i < radius+2; i++) {
        weight[radius-i+1] = weight[radius-i+2] * (N-i+1) / i;
    }
    sum -= (weight[radius+1] + weight[radius]) * 2.0;

    for (int i = 0; i < radius; i++) {
        offset[i] = (float)i*sigma;
        weight[i] /= sum;
    }

    *pradius = radius;

#if 0
    //step2: interpolate
    radius = (radius+1)/2;
    for (int i = 1; i < radius; i++) {
        float w = weight[i*2] + weight[i*2-1];
        float off = (offset[i*2] * weight[i*2] + offset[i*2-1] * weight[i*2-1]) / w;
        offset[i] = off;
        weight[i] = w;
    }
    *pradius = radius;
#endif
}

struct _MetaBlurredBackgroundActorPrivate
{
    MetaScreen *screen;
    int monitor;

    MetaBackground *background;

    gboolean blurred;
    int radius;
    float kernel[41];
    int rounds;

    ChangedFlags changed;
    CoglPipeline *pl_passthrough;

    CoglPipeline *pipeline;
    CoglPipeline *pipeline2;

    CoglTexture* texture; // background texture
    cairo_rectangle_int_t texture_area;
    CoglTexture* fbTex, *fbTex2;
    CoglOffscreen* fb, *fb2;
    float fb_width;
    float fb_height;

    cairo_region_t *clip_region;
};

static void cullable_iface_init (MetaCullableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (MetaBlurredBackgroundActor, meta_blurred_background_actor,
        CLUTTER_TYPE_ACTOR,
        G_IMPLEMENT_INTERFACE (META_TYPE_CULLABLE, cullable_iface_init));

static void set_clip_region (MetaBlurredBackgroundActor *self,
        cairo_region_t      *clip_region)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    g_clear_pointer (&priv->clip_region, (GDestroyNotify) cairo_region_destroy);
    if (clip_region)
        priv->clip_region = cairo_region_copy (clip_region);
}

static void meta_blurred_background_actor_dispose (GObject *object)
{
    MetaBlurredBackgroundActor *self = META_BLURRED_BACKGROUND_ACTOR (object);
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    set_clip_region (self, NULL);
    if (priv->fbTex) {
        cogl_object_unref (priv->fbTex);
        cogl_object_unref (priv->fbTex2);
        cogl_object_unref (priv->fb);
        cogl_object_unref (priv->fb2);
        cogl_object_unref (priv->texture);
        priv->fbTex = NULL;
    }
    meta_blurred_background_actor_set_background (self, NULL);
    if (priv->pipeline) {
        cogl_object_unref (priv->pipeline);
        cogl_object_unref (priv->pipeline2);
        cogl_object_unref (priv->pl_passthrough);
        priv->pipeline = NULL;
        priv->pipeline2 = NULL;
    }

    G_OBJECT_CLASS (meta_blurred_background_actor_parent_class)->dispose (object);
}

static void get_preferred_size (MetaBlurredBackgroundActor *self,
        gfloat              *width,
        gfloat              *height)
{
    MetaBlurredBackgroundActorPrivate *priv = META_BLURRED_BACKGROUND_ACTOR (self)->priv;
    MetaRectangle monitor_geometry;

    meta_screen_get_monitor_geometry (priv->screen, priv->monitor, &monitor_geometry);

    if (width != NULL)
        *width = monitor_geometry.width;

    if (height != NULL)
        *height = monitor_geometry.height;
}

static void meta_blurred_background_actor_get_preferred_width (ClutterActor *actor,
        gfloat        for_height,
        gfloat       *min_width_p,
        gfloat       *natural_width_p)
{
    gfloat width;

    get_preferred_size (META_BLURRED_BACKGROUND_ACTOR (actor), &width, NULL);

    if (min_width_p)
        *min_width_p = width;
    if (natural_width_p)
        *natural_width_p = width;
}

static void meta_blurred_background_actor_get_preferred_height (ClutterActor *actor,
        gfloat        for_width,
        gfloat       *min_height_p,
        gfloat       *natural_height_p)

{
    gfloat height;

    get_preferred_size (META_BLURRED_BACKGROUND_ACTOR (actor), NULL, &height);

    if (min_height_p)
        *min_height_p = height;
    if (natural_height_p)
        *natural_height_p = height;
}

static void make_pipeline (MetaBlurredBackgroundActor* self)
{
    MetaBlurredBackgroundActorPrivate* priv = self->priv;
    static CoglPipeline *template = NULL;

    if (template == NULL) {
        /* Cogl automatically caches pipelines with no eviction policy,
         * so we need to prevent identical pipelines from getting cached
         * separately, by reusing the same shader snippets.
         */
        template = COGL_PIPELINE (meta_create_texture_pipeline (NULL));
        CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT_GLOBALS,
                gaussian_blur_global_definition, NULL);
        cogl_pipeline_add_snippet (template, snippet);
        cogl_object_unref (snippet);
    }

    priv->pl_passthrough = cogl_pipeline_copy (template);

    priv->pipeline = cogl_pipeline_copy (template);

    CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
            gaussian_blur_glsl_declarations, NULL);
    cogl_snippet_set_replace (snippet, vs_code);
    cogl_pipeline_add_layer_snippet (priv->pipeline, 0, snippet);
    cogl_object_unref (snippet);

    priv->pipeline2 = cogl_pipeline_copy (template);

    snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
            gaussian_blur_glsl_declarations, NULL);
    cogl_snippet_set_replace (snippet, vs_code_h);
    cogl_pipeline_add_layer_snippet (priv->pipeline2, 0, snippet);
    cogl_object_unref (snippet);
}

static float scale = 0.25f;
static void create_texture (MetaBlurredBackgroundActor* self)
{
    MetaBlurredBackgroundActorPrivate* priv = self->priv;
    gfloat width, height;

    CoglContext *ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());

    get_preferred_size (self, &width, &height);
    width *= scale;
    height *= scale;

    if (priv->fbTex2) {
        cogl_object_unref (priv->fbTex2);
        priv->fbTex2 = NULL;
    }

    priv->fb_width = width;
    priv->fb_height = height;
    meta_verbose ("%s: recreate fbTex (%f, %f)\n", __func__, width, height);

    priv->fbTex = cogl_texture_2d_new_with_size(ctx, priv->fb_width, priv->fb_height);
    cogl_texture_set_components(priv->fbTex, COGL_TEXTURE_COMPONENTS_RGBA);
    cogl_primitive_texture_set_auto_mipmap(priv->fbTex, TRUE);

    CoglError *error = NULL;
    if (cogl_texture_allocate(priv->fbTex, &error) == FALSE) {
        meta_verbose ("cogl_texture_allocat failed: %s\n", error->message);
    }

    priv->fb = cogl_offscreen_new_with_texture(priv->fbTex);
    if (cogl_framebuffer_allocate(priv->fb, &error) == FALSE) {
        meta_verbose ("cogl_framebuffer_allocate failed: %s\n", error->message);
    }

    cogl_framebuffer_orthographic(priv->fb, 0, 0,
            width, height, -1., 1.);

    priv->fbTex2 = cogl_texture_2d_new_with_size(ctx, priv->fb_width, priv->fb_height);
    cogl_texture_set_components(priv->fbTex2, COGL_TEXTURE_COMPONENTS_RGBA);

    if (cogl_texture_allocate(priv->fbTex2, &error) == FALSE) {
        meta_verbose ("cogl_texture_allocat failed: %s\n", error->message);
    }

    priv->fb2 = cogl_offscreen_new_with_texture(priv->fbTex2);
    if (cogl_framebuffer_allocate(priv->fb2, &error) == FALSE) {
        meta_verbose ("cogl_framebuffer_allocate failed: %s\n", error->message);
    }

    cogl_framebuffer_orthographic(priv->fb2, 0, 0,
            width, height, -1., 1.);
}

static int get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void preblur_texture(MetaBlurredBackgroundActor* self)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;
    int uniform_no = cogl_pipeline_get_uniform_location (priv->pipeline, "kernel");
    cogl_pipeline_set_uniform_float (priv->pipeline, uniform_no, 1, 41, priv->kernel);

    float resolution[2] = {priv->fb_width, priv->fb_height};
    uniform_no = cogl_pipeline_get_uniform_location(priv->pipeline, "resolution");
    cogl_pipeline_set_uniform_float(priv->pipeline, uniform_no, 2, 1, resolution);


    uniform_no = cogl_pipeline_get_uniform_location (priv->pipeline2, "kernel");
    cogl_pipeline_set_uniform_float (priv->pipeline2, uniform_no, 1, 41, priv->kernel);

    uniform_no = cogl_pipeline_get_uniform_location(priv->pipeline2, "resolution");
    cogl_pipeline_set_uniform_float(priv->pipeline2, uniform_no, 2, 1, resolution);

    int start = get_time();
    for (int i = 0; i < priv->rounds; i++) {
        CoglTexture* tex1 = i == 0 ? priv->texture : priv->fbTex2;
        cogl_pipeline_set_layer_texture (priv->pipeline, 0, tex1);

        cogl_framebuffer_draw_textured_rectangle (priv->fb, priv->pipeline, 
                0.0f, 0.0f, priv->fb_width, priv->fb_height,
                0.0f, 0.0f, 1.00f, 1.00f);

        if (i > 0) cogl_framebuffer_finish(priv->fb);


        cogl_pipeline_set_layer_texture (priv->pipeline2, 0, priv->fbTex);
        cogl_framebuffer_draw_textured_rectangle (
                priv->fb2, priv->pipeline2,
                0.0f, 0.0f, priv->fb_width, priv->fb_height,
                0.0f, 0.0f, 1.00f, 1.00f);

        if (i > 0) cogl_framebuffer_finish(priv->fb2);
    }
    meta_verbose ("preblur rendering time: %d\n", get_time() - start);

    cogl_object_unref (priv->fbTex);
    cogl_object_unref (priv->fb);
    cogl_object_unref (priv->fb2);
    priv->fbTex = NULL;
    priv->fb = NULL;
    priv->fb2 = NULL;
}

static void setup_pipeline (MetaBlurredBackgroundActor   *self,
        cairo_rectangle_int_t *actor_pixel_rect)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    if (priv->changed) {
        CoglPipelineWrapMode wrap_mode;
        priv->texture = meta_background_get_texture (priv->background,
                priv->monitor,
                &priv->texture_area,
                &wrap_mode);

        cogl_primitive_texture_set_auto_mipmap (priv->texture, TRUE);

        if (priv->radius) {
            cogl_pipeline_set_layer_texture (priv->pipeline, 0, priv->texture);
            cogl_pipeline_set_layer_filters (priv->pipeline, 0,
                    COGL_PIPELINE_FILTER_LINEAR_MIPMAP_LINEAR,
                    COGL_PIPELINE_FILTER_LINEAR);
            cogl_pipeline_set_layer_wrap_mode (priv->pipeline, 0, wrap_mode);

            create_texture (self);

            cogl_pipeline_set_layer_texture (priv->pipeline2, 0, priv->fbTex);
            cogl_pipeline_set_layer_filters (priv->pipeline2, 0,
                    COGL_PIPELINE_FILTER_LINEAR_MIPMAP_LINEAR,
                    COGL_PIPELINE_FILTER_LINEAR);
            cogl_pipeline_set_layer_wrap_mode (priv->pipeline2, 0, wrap_mode);

            preblur_texture (self);
        }
        priv->changed = 0;
    }

}

static gboolean meta_blurred_background_actor_get_paint_volume (
        ClutterActor       *actor,
        ClutterPaintVolume *volume)
{
    return clutter_paint_volume_set_from_allocation (volume, actor);
}

static void meta_blurred_background_actor_paint (ClutterActor *actor)
{
    MetaBlurredBackgroundActor *self = META_BLURRED_BACKGROUND_ACTOR (actor);
    MetaBlurredBackgroundActorPrivate *priv = self->priv;
    ClutterActorBox actor_box;
    cairo_rectangle_int_t bounding;

    if ((priv->clip_region && cairo_region_is_empty (priv->clip_region)))
        return;

    clutter_actor_get_content_box (actor, &actor_box);
    bounding.x = actor_box.x1;
    bounding.y = actor_box.y1;
    bounding.width = actor_box.x2 - actor_box.x1;
    bounding.height = actor_box.y2 - actor_box.y1;

    guint8 opacity;
    opacity = clutter_actor_get_paint_opacity (CLUTTER_ACTOR (self));
    cogl_pipeline_set_color4ub (priv->pl_passthrough,
            opacity, opacity, opacity, opacity);

    setup_pipeline (self, &bounding);
    if (priv->radius > 0) {
        cogl_pipeline_set_layer_texture (priv->pl_passthrough, 0, priv->fbTex2);

    } else {
        cogl_pipeline_set_layer_texture (priv->pl_passthrough, 0, priv->texture);
    }
    cogl_framebuffer_draw_textured_rectangle (
            cogl_get_draw_framebuffer (), priv->pl_passthrough,
            bounding.x, bounding.y, bounding.width, bounding.height,
            0.0f, 0.0f, 1.00f, 1.00f);
}

static void meta_blurred_background_actor_set_property (GObject      *object,
        guint         prop_id,
        const GValue *value,
        GParamSpec   *pspec)
{
    MetaBlurredBackgroundActor *self = META_BLURRED_BACKGROUND_ACTOR (object);
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    switch (prop_id)
    {
        case PROP_META_SCREEN:
            priv->screen = g_value_get_object (value);
            break;
        case PROP_MONITOR:
            priv->monitor = g_value_get_int (value);
            break;
        case PROP_BACKGROUND:
            meta_blurred_background_actor_set_background (self, g_value_get_object (value));
            break;
        case PROP_RADIUS:
            meta_blurred_background_actor_set_radius (self,
                    g_value_get_int (value));
            break;
        case PROP_ROUNDS:
            meta_blurred_background_actor_set_rounds (self,
                    g_value_get_int (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void meta_blurred_background_actor_get_property (GObject      *object,
        guint         prop_id,
        GValue       *value,
        GParamSpec   *pspec)
{
    MetaBlurredBackgroundActorPrivate *priv = META_BLURRED_BACKGROUND_ACTOR (object)->priv;

    switch (prop_id)
    {
        case PROP_META_SCREEN:
            g_value_set_object (value, priv->screen);
            break;
        case PROP_MONITOR:
            g_value_set_int (value, priv->monitor);
            break;
        case PROP_BACKGROUND:
            g_value_set_object (value, priv->background);
            break;
        case PROP_RADIUS:
            g_value_set_int (value, priv->radius);
            break;
        case PROP_ROUNDS:
            g_value_set_int (value, priv->rounds);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

    static void
meta_blurred_background_actor_class_init (MetaBlurredBackgroundActorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
    GParamSpec *param_spec;

    g_type_class_add_private (klass, sizeof (MetaBlurredBackgroundActorPrivate));

    object_class->dispose = meta_blurred_background_actor_dispose;
    object_class->set_property = meta_blurred_background_actor_set_property;
    object_class->get_property = meta_blurred_background_actor_get_property;

    actor_class->get_preferred_width = meta_blurred_background_actor_get_preferred_width;
    actor_class->get_preferred_height = meta_blurred_background_actor_get_preferred_height;
    actor_class->get_paint_volume = meta_blurred_background_actor_get_paint_volume;
    actor_class->paint = meta_blurred_background_actor_paint;

    param_spec = g_param_spec_object ("meta-screen",
            "MetaScreen",
            "MetaScreen",
            META_TYPE_SCREEN,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_property (object_class,
            PROP_META_SCREEN,
            param_spec);

    param_spec = g_param_spec_int ("monitor",
            "monitor",
            "monitor",
            0, G_MAXINT, 0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_property (object_class,
            PROP_MONITOR,
            param_spec);

    param_spec = g_param_spec_object ("background",
            "Background",
            "MetaBackground object holding background parameters",
            META_TYPE_BACKGROUND,
            G_PARAM_READWRITE);

    g_object_class_install_property (object_class,
            PROP_BACKGROUND,
            param_spec);

    param_spec = g_param_spec_int ("radius",
                "blur radius",
                "blur radius",
                0,
                19,
                5,
                G_PARAM_READWRITE);

    g_object_class_install_property (object_class,
            PROP_RADIUS,
            param_spec);

    param_spec = g_param_spec_int ("rounds",
                "blur rounds",
                "blur rounds",
                1,
                100,
                4,
                G_PARAM_READWRITE);

    g_object_class_install_property (object_class,
            PROP_ROUNDS,
            param_spec);
}

static void meta_blurred_background_actor_init (MetaBlurredBackgroundActor *self)
{
    MetaBlurredBackgroundActorPrivate *priv;

    priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
            META_TYPE_BLURRED_BACKGROUND_ACTOR,
            MetaBlurredBackgroundActorPrivate);

    priv->radius = 0; // means no blur
    priv->rounds = 1;
}

ClutterActor * meta_blurred_background_actor_new (MetaScreen *screen,
        int         monitor)
{
    MetaBlurredBackgroundActor *self;

    self = g_object_new (META_TYPE_BLURRED_BACKGROUND_ACTOR,
            "meta-screen", screen,
            "monitor", monitor,
            NULL);

    make_pipeline (self);
    return CLUTTER_ACTOR (self);
}

static void meta_blurred_background_actor_cull_out (MetaCullable   *cullable,
        cairo_region_t *unobscured_region,
        cairo_region_t *clip_region)
{
    MetaBlurredBackgroundActor *self = META_BLURRED_BACKGROUND_ACTOR (cullable);
    set_clip_region (self, clip_region);
}

static void meta_blurred_background_actor_reset_culling (MetaCullable *cullable)
{
    MetaBlurredBackgroundActor *self = META_BLURRED_BACKGROUND_ACTOR (cullable);
    set_clip_region (self, NULL);
}

static void cullable_iface_init (MetaCullableInterface *iface)
{
    iface->cull_out = meta_blurred_background_actor_cull_out;
    iface->reset_culling = meta_blurred_background_actor_reset_culling;
}

cairo_region_t * meta_blurred_background_actor_get_clip_region (MetaBlurredBackgroundActor *self)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;
    return priv->clip_region;
}

static void invalidate_pipeline (MetaBlurredBackgroundActor *self,
        ChangedFlags         changed)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    priv->changed |= changed;
}

static void on_background_changed (MetaBackground      *background,
        MetaBlurredBackgroundActor *self)
{
    invalidate_pipeline (self, CHANGED_ALL);
    clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

void meta_blurred_background_actor_set_background (MetaBlurredBackgroundActor *self,
        MetaBackground      *background)
{
    MetaBlurredBackgroundActorPrivate *priv;

    g_return_if_fail (META_IS_BLURRED_BACKGROUND_ACTOR (self));
    g_return_if_fail (background == NULL || META_IS_BACKGROUND (background));

    priv = self->priv;

    if (background == priv->background)
        return;

    if (priv->background) {
        g_signal_handlers_disconnect_by_func (priv->background,
                (gpointer)on_background_changed, self);
        g_object_unref (priv->background);
        priv->background = NULL;
    }

    if (background) {
        priv->background = g_object_ref (background);
        g_signal_connect (priv->background, "changed",
                G_CALLBACK (on_background_changed), self);
    }

    invalidate_pipeline (self, CHANGED_ALL);
    clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

void meta_blurred_background_actor_set_radius (MetaBlurredBackgroundActor *self, int radius)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    g_return_if_fail (META_IS_BLURRED_BACKGROUND_ACTOR (self));
    g_return_if_fail (radius >= 0 && radius <= 19);

    if (priv->radius != radius) {
        priv->radius = radius;
        if (radius > 0) {
            build_gaussian_blur_kernel(&radius, &priv->kernel[1], &priv->kernel[21]);
            priv->radius = radius;
            priv->kernel[0] = radius;
        }

        invalidate_pipeline (self, CHANGED_EFFECTS);
        clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
    }
}


void meta_blurred_background_actor_set_rounds (MetaBlurredBackgroundActor *self, int rounds)
{
    MetaBlurredBackgroundActorPrivate *priv = self->priv;

    g_return_if_fail (META_IS_BLURRED_BACKGROUND_ACTOR (self));
    g_return_if_fail (rounds >= 1 && rounds <= 100);

    if (priv->rounds != rounds) {
        priv->rounds = rounds;
        if (rounds > 0) {
            priv->rounds = rounds;
        }

        invalidate_pipeline (self, CHANGED_EFFECTS);
        clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
    }
}
