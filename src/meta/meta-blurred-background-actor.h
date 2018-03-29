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

#ifndef META_BLURRED_BACKGROUND_ACTOR_H
#define META_BLURRED_BACKGROUND_ACTOR_H

#include <clutter/clutter.h>
#include <meta/screen.h>
#include <meta/meta-background.h>

#include <gsettings-desktop-schemas/gdesktop-enums.h>


#define META_TYPE_BLURRED_BACKGROUND_ACTOR            (meta_blurred_background_actor_get_type ())
#define META_BLURRED_BACKGROUND_ACTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), META_TYPE_BLURRED_BACKGROUND_ACTOR, MetaBlurredBackgroundActor))
#define META_BLURRED_BACKGROUND_ACTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), META_TYPE_BLURRED_BACKGROUND_ACTOR, MetaBlurredBackgroundActorClass))
#define META_IS_BLURRED_BACKGROUND_ACTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), META_TYPE_BLURRED_BACKGROUND_ACTOR))
#define META_IS_BLURRED_BACKGROUND_ACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), META_TYPE_BLURRED_BACKGROUND_ACTOR))
#define META_BLURRED_BACKGROUND_ACTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), META_TYPE_BLURRED_BACKGROUND_ACTOR, MetaBlurredBackgroundActorClass))

typedef struct _MetaBlurredBackgroundActor        MetaBlurredBackgroundActor;
typedef struct _MetaBlurredBackgroundActorClass   MetaBlurredBackgroundActorClass;
typedef struct _MetaBlurredBackgroundActorPrivate MetaBlurredBackgroundActorPrivate;

struct _MetaBlurredBackgroundActorClass
{
  /*< private >*/
  ClutterActorClass parent_class;
};

struct _MetaBlurredBackgroundActor
{
  ClutterActor parent;

  MetaBlurredBackgroundActorPrivate *priv;
};

GType meta_blurred_background_actor_get_type (void);

ClutterActor *meta_blurred_background_actor_new    (MetaScreen *screen,
                                            int         monitor);

void meta_blurred_background_actor_set_background  (MetaBlurredBackgroundActor *self,
                                            MetaBackground      *background);

/* radius should be odd now, if == 0, means disable */
void meta_blurred_background_actor_set_radius (MetaBlurredBackgroundActor *self,
                                         int               radius);
void meta_blurred_background_actor_set_rounds (MetaBlurredBackgroundActor *self,
                                         int rounds);

void meta_blurred_background_actor_set_blur_mask (MetaBlurredBackgroundActor *self,
        cairo_surface_t* mask);

void meta_blurred_background_actor_set_enabled (MetaBlurredBackgroundActor *self,
        gboolean enabled);

#endif /* META_BLURRED_BACKGROUND_ACTOR_H */
