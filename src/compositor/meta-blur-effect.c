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
#include <GL/gl.h>
#include <GL/glx.h>

#include <clutter/clutter.h>

#include <clutter/clutter-effect.h>
#include <meta/meta-blur-effect.h>
#include <math.h>
#include <meta/util.h>

#include <cogl/cogl.h>

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
"float lod = 4.6;"
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
    float sigma = 2.0f;

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
    fprintf (stderr, "kernel = %d\n", radius);
    for (int i = 0; i < radius; i++) {
        fprintf (stderr, "%f %f\n", offset[i], weight[i]);
    }
#endif

#if 1
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

struct _MetaBlurEffect
{
    ClutterOffscreenEffect parent_instance;

    /* a back pointer to our actor, so that we can query it */
    ClutterActor *actor;

    int radius;

    float kernel[41];

    CoglMatrix oldmat;
    CoglMatrix oldproj;
    float v[4];

    CoglTexture* fbTex;
    CoglOffscreen* fb;
    float fb_width;
    float fb_height;


    CoglPipeline *pipeline;
    CoglPipeline *pipeline2;

    float tex_width;
    float tex_height;
};

struct _MetaBlurEffectClass
{
    ClutterOffscreenEffectClass parent_class;

    CoglPipeline *base_pipeline;
};

G_DEFINE_TYPE (MetaBlurEffect, meta_blur_effect, CLUTTER_TYPE_OFFSCREEN_EFFECT);

enum {
  PROP_0,

  PROP_RADIUS,

  LAST_PROP,
};

static GParamSpec *obj_props[LAST_PROP];

static void meta_blur_effect_get_property(GObject        *object,
        guint            prop_id,
        GValue          *value,
        GParamSpec      *pspec)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (object);

    switch (prop_id)
    {
        case PROP_RADIUS:
            g_value_set_int (value, self->radius);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void meta_blur_effect_set_property(GObject         *object,
        guint            prop_id,
        const GValue    *value,
        GParamSpec      *pspec)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (object);
    switch (prop_id)
    {
        case PROP_RADIUS:
        {
            self->radius = g_value_get_int (value);

            int rad = self->radius;
            build_gaussian_blur_kernel(&rad, &self->kernel[1], &self->kernel[21]);
            self->kernel[0] = rad;

            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean meta_blur_effect_pre_paint (ClutterEffect *effect)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (effect);
    ClutterEffectClass *parent_class;

    if (!clutter_actor_meta_get_enabled (CLUTTER_ACTOR_META (effect)))
        return FALSE;

    self->actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (effect));
    if (self->actor == NULL)
        return FALSE;

    if (!clutter_feature_available (CLUTTER_FEATURE_SHADERS_GLSL))
    {
        /* if we don't have support for GLSL shaders then we
         * forcibly disable the ActorClutter
         */
        g_warning ("Unable to use the ShaderEffect: the graphics hardware "
                "or the current GL driver does not implement support "
                "for the GLSL shading language.");
        clutter_actor_meta_set_enabled (CLUTTER_ACTOR_META (effect), FALSE);
        return FALSE;
    }

    parent_class = CLUTTER_EFFECT_CLASS (meta_blur_effect_parent_class);
    if (parent_class->pre_paint (effect))
    {
        ClutterOffscreenEffect *offscreen_effect = CLUTTER_OFFSCREEN_EFFECT (effect);
        CoglHandle texture = clutter_offscreen_effect_get_texture (offscreen_effect);

        cogl_pipeline_set_layer_texture (self->pipeline, 0, texture);
        cogl_pipeline_set_layer_filters (self->pipeline, 0, COGL_PIPELINE_FILTER_LINEAR_MIPMAP_LINEAR,
                COGL_PIPELINE_FILTER_LINEAR);
        cogl_pipeline_set_layer_wrap_mode (self->pipeline, 0, COGL_PIPELINE_WRAP_MODE_REPEAT);

        cogl_pipeline_set_layer_texture (self->pipeline2, 0, self->fbTex);
        cogl_pipeline_set_layer_filters (self->pipeline2, 0, COGL_PIPELINE_FILTER_LINEAR_MIPMAP_LINEAR,
                COGL_PIPELINE_FILTER_LINEAR);
        cogl_pipeline_set_layer_wrap_mode (self->pipeline2, 0, COGL_PIPELINE_WRAP_MODE_REPEAT);

        cogl_get_modelview_matrix (&self->oldmat);
        cogl_get_projection_matrix (&self->oldproj);
        cogl_get_viewport (self->v);

        return TRUE;
    }
    else
        return FALSE;
}

void (*meta_gen_mipmap)(GLenum      target);
void (*meta_tex_parameteri)(GLenum      target,
        GLenum      pname, GLint   param);
void (*meta_bind_texture)( GLenum      target,
            GLuint      texture);

static float vps = 1.00;
static void meta_blur_effect_paint_target (ClutterOffscreenEffect *effect)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (effect);
    guint8 paint_opacity;

    paint_opacity = clutter_actor_get_paint_opacity (self->actor);

    unsigned int gl_handle, gl_target;
    CoglHandle texture;
    meta_bind_texture = cogl_get_proc_address ("glBindTexture");
    meta_gen_mipmap = cogl_get_proc_address ("glGenerateMipmap");

#if 1
    texture = clutter_offscreen_effect_get_texture (CLUTTER_OFFSCREEN_EFFECT(self));
    cogl_texture_get_gl_texture (texture, &gl_handle, &gl_target);

    meta_bind_texture(gl_target, gl_handle);
    meta_gen_mipmap(GL_TEXTURE_2D);
    meta_bind_texture(GL_TEXTURE_2D, 0);
#endif


    {
        ClutterPerspective perspective;
        ClutterStage* stage = clutter_actor_get_stage (self->actor);
        clutter_stage_get_perspective (stage, &perspective);

        cogl_framebuffer_set_viewport (self->fb, self->v[0], self->v[1], self->v[2], self->v[3]);

        cogl_push_framebuffer (self->fb);
        cogl_set_modelview_matrix (&self->oldmat);
        cogl_set_projection_matrix (&self->oldproj);

        cogl_set_source (self->pipeline);
        int uniform_no = cogl_pipeline_get_uniform_location (self->pipeline, "kernel");
        cogl_pipeline_set_uniform_float (self->pipeline, uniform_no, 1, 41, self->kernel);

        float resolution[2] = {self->fb_width, self->fb_height};
        uniform_no = cogl_pipeline_get_uniform_location(self->pipeline, "resolution");
        cogl_pipeline_set_uniform_float(self->pipeline, uniform_no, 2, 1, resolution);

        cogl_rectangle_with_texture_coords (0.0f, 0.0f, self->fb_width, self->fb_height,
                0.0f, 0.0f, 1.00f, 1.00f);
        cogl_pop_framebuffer (); 
    }

#if 0
    texture = self->fbTex;
    cogl_texture_get_gl_texture (texture, &gl_handle, &gl_target);

    meta_bind_texture(gl_target, gl_handle);
    meta_gen_mipmap(GL_TEXTURE_2D);
    meta_bind_texture(GL_TEXTURE_2D, 0);
#endif

    {
        cogl_pipeline_set_color4ub (self->pipeline2,
                paint_opacity,
                paint_opacity,
                paint_opacity,
                paint_opacity);
        cogl_push_source (self->pipeline2);
        int uniform_no = cogl_pipeline_get_uniform_location (self->pipeline2, "kernel");
        cogl_pipeline_set_uniform_float (self->pipeline2, uniform_no, 1, 41, self->kernel);

        float resolution[2] = {self->tex_width, self->tex_height};
        uniform_no = cogl_pipeline_get_uniform_location(self->pipeline2, "resolution");
        cogl_pipeline_set_uniform_float(self->pipeline2, uniform_no, 2, 1, resolution);

        cogl_rectangle_with_texture_coords (0.0f, 0.0f, self->tex_width, self->tex_height,
                0.0f, 0.0f, 1.00f, 1.00f);
        cogl_pop_source ();
    }
}

static gboolean meta_blur_effect_get_paint_volume (ClutterEffect      *effect,
        ClutterPaintVolume *volume)
{
    gfloat cur_width, cur_height;
    ClutterVertex origin;

    clutter_paint_volume_get_origin (volume, &origin);
    cur_width = clutter_paint_volume_get_width (volume);
    cur_height = clutter_paint_volume_get_height (volume);

    int BLUR_PADDING = 2;
    origin.x -= BLUR_PADDING;
    origin.y -= BLUR_PADDING;
    cur_width += 2 * BLUR_PADDING;
    cur_height += 2 * BLUR_PADDING;
    clutter_paint_volume_set_origin (volume, &origin);
    clutter_paint_volume_set_width (volume, cur_width);
    clutter_paint_volume_set_height (volume, cur_height);

    return TRUE;
}

static void meta_blur_effect_dispose (GObject *gobject)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (gobject);

    if (self->fbTex) {
        cogl_object_unref (self->fbTex);
        cogl_object_unref (self->fb);
        self->fbTex = NULL;
    }

    if (self->pipeline != NULL)
    {
        cogl_object_unref (self->pipeline);
        cogl_object_unref (self->pipeline2);
        self->pipeline = NULL;
        self->pipeline2 = NULL;
    }

    G_OBJECT_CLASS (meta_blur_effect_parent_class)->dispose (gobject);
}

static CoglHandle meta_blur_effect_create_texture (ClutterOffscreenEffect* effect,
        gfloat                  width,
        gfloat                  height)
{
    MetaBlurEffect *self = META_BLUR_EFFECT (effect);
    CoglContext *ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());

    CoglHandle texture = cogl_texture_2d_new_with_size(ctx, width, height);
    cogl_primitive_texture_set_auto_mipmap (texture, TRUE);
    g_assert (texture != NULL);
    cogl_texture_allocate (texture, NULL);

    self->tex_width = cogl_texture_get_width (texture);
    self->tex_height = cogl_texture_get_height (texture);

    if (self->fbTex) {
        cogl_object_unref (self->fbTex);
        cogl_object_unref (self->fb);
        self->fbTex = NULL;
    }

    self->fb_width = vps * width;
    self->fb_height = vps * height;

    self->fbTex = cogl_texture_2d_new_with_size(ctx, self->fb_width, self->fb_height);
    cogl_texture_set_components(self->fbTex, COGL_TEXTURE_COMPONENTS_RGBA);
    cogl_primitive_texture_set_auto_mipmap(self->fbTex, TRUE);

    CoglError *error = NULL;
    if (cogl_texture_allocate(self->fbTex, &error) == FALSE) {
        meta_verbose ("cogl_texture_allocat failed: %s\n", error->message);
    }

    self->fb = cogl_offscreen_new_with_texture(self->fbTex);
    if (cogl_framebuffer_allocate(self->fb, &error) == FALSE) {
        meta_verbose ("cogl_framebuffer_allocate failed: %s\n", error->message);
    }

   return texture;
}

static void meta_blur_effect_class_init (MetaBlurEffectClass *klass)
{
    ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS (klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    ClutterOffscreenEffectClass *offscreen_class;

    gobject_class->dispose = meta_blur_effect_dispose;
    gobject_class->set_property = meta_blur_effect_set_property;
    gobject_class->get_property = meta_blur_effect_get_property;

    effect_class->pre_paint = meta_blur_effect_pre_paint;
    effect_class->get_paint_volume = meta_blur_effect_get_paint_volume;

    offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
    offscreen_class->paint_target = meta_blur_effect_paint_target;
    offscreen_class->create_texture = meta_blur_effect_create_texture;

    obj_props[PROP_RADIUS] =
        g_param_spec_int ("radius",
                "blur radius",
                "blur radius",
                0,
                19,
                5,
                G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);
}

static void meta_blur_effect_init (MetaBlurEffect *self)
{
    MetaBlurEffectClass *klass = META_BLUR_EFFECT_GET_CLASS (self);

    CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());
    if (G_UNLIKELY (klass->base_pipeline == NULL)) {

        klass->base_pipeline = cogl_pipeline_new (ctx);

        cogl_pipeline_set_layer_null_texture (klass->base_pipeline,
                0,
                COGL_TEXTURE_TYPE_2D);
    }

    {
        self->pipeline = cogl_pipeline_copy (klass->base_pipeline);
        {
        CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT_GLOBALS,
                gaussian_blur_global_definition, NULL);
        cogl_pipeline_add_snippet (self->pipeline, snippet);
        cogl_object_unref (snippet);
        }

        CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
                gaussian_blur_glsl_declarations, NULL);
        cogl_snippet_set_replace (snippet, vs_code);
        cogl_pipeline_add_layer_snippet (self->pipeline, 0, snippet);
        cogl_object_unref (snippet);
    }

    {
        self->pipeline2 = cogl_pipeline_copy (klass->base_pipeline);
        {
        CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT_GLOBALS,
                gaussian_blur_global_definition, NULL);
        cogl_pipeline_add_snippet (self->pipeline2, snippet);
        cogl_object_unref (snippet);
        }

        CoglSnippet* snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_TEXTURE_LOOKUP,
                gaussian_blur_glsl_declarations, NULL);
        cogl_snippet_set_replace (snippet, vs_code_h);
        cogl_pipeline_add_layer_snippet (self->pipeline2, 0, snippet);
        cogl_object_unref (snippet);

    }

    if (cogl_has_feature(ctx, COGL_FEATURE_ID_OFFSCREEN) == FALSE) {
        meta_verbose ("COGL_FEATURE_ID_OFFSCREEN not supported\n");
    }

    if (cogl_has_feature(ctx, COGL_FEATURE_ID_TEXTURE_NPOT) == FALSE) {
        meta_verbose ("COGL_FEATURE_ID_TEXTURE_NPOT not supported\n");
    }

  CoglDisplay *cogl_display;
  CoglRenderer *cogl_renderer;

  cogl_display = cogl_context_get_display (ctx);
  cogl_renderer = cogl_display_get_renderer (cogl_display);
  switch (cogl_renderer_get_driver (cogl_renderer))
    {
    case COGL_DRIVER_GL3:
      {
          meta_topic (META_DEBUG_COMPOSITOR, "COGL_DRIVER_GL3\n");
          break;
      }
    case COGL_DRIVER_GL:
      {
          meta_topic (META_DEBUG_COMPOSITOR, "COGL_DRIVER_GL\n");
          break;
      }
    default:
      break;
    }
}

/**
 * meta_blur_effect_new:
 *
 * Creates a new #MetaBlurEffect to be used with
 * meta_actor_add_effect()
 *
 * Return value: the newly created #MetaBlurEffect or %NULL
 *
 * Since: 1.4
 */
ClutterEffect * meta_blur_effect_new (int radius)
{
    return g_object_new (META_TYPE_BLUR_EFFECT, "radius", radius, NULL);
}

