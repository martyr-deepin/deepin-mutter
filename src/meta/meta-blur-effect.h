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

#ifndef __META_BLUR_EFFECT_H__
#define __META_BLUR_EFFECT_H__

#include <clutter/clutter.h>
#include <clutter/clutter-effect.h>

G_BEGIN_DECLS

#define META_TYPE_BLUR_EFFECT        (meta_blur_effect_get_type ())
#define META_BLUR_EFFECT(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), META_TYPE_BLUR_EFFECT, MetaBlurEffect))
#define META_IS_BLUR_EFFECT(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), META_TYPE_BLUR_EFFECT))

#define META_BLUR_EFFECT_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), META_TYPE_BLUR_EFFECT, MetaBlurEffectClass))
#define META_IS_BLUR_EFFECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), META_TYPE_BLUR_EFFECT))
#define META_BLUR_EFFECT_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), META_TYPE_BLUR_EFFECT, MetaBlurEffectClass))

typedef struct _MetaBlurEffect       MetaBlurEffect;
typedef struct _MetaBlurEffectClass  MetaBlurEffectClass;

GType meta_blur_effect_get_type (void) G_GNUC_CONST;

ClutterEffect* meta_blur_effect_new (int radius);

G_END_DECLS

#endif /* __META_BLUR_EFFECT_H__ */

