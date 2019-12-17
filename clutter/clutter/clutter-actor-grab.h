/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright © 2009, 2010, 2011  Intel Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Jonas Dreßler <verdre@v0yd.nl>
 */

#ifndef __clutter_actor_grab_H__
#define __clutter_actor_grab_H__

#if !defined(__CLUTTER_H_INSIDE__) && !defined(CLUTTER_COMPILATION)
#error "Only <clutter/clutter.h> can be included directly."
#endif

#include <clutter/clutter-grab.h>

#define CLUTTER_TYPE_ACTOR_GRAB (clutter_actor_grab_get_type ())

CLUTTER_EXPORT
G_DECLARE_DERIVABLE_TYPE (ClutterActorGrab, clutter_actor_grab,
                          CLUTTER, ACTOR_GRAB, ClutterGrab);

/**
 * ClutterActorGrabClass:
 *
 * The #ClutterActorGrabClass structure contains
 * only private data
 */
typedef struct _ClutterActorGrabClass
{
  ClutterGrabClass parent_class;

} ClutterActorGrabClass;

CLUTTER_EXPORT
ClutterActorGrab * clutter_actor_grab_new (ClutterActor *topmost_actor);

CLUTTER_EXPORT
ClutterActor * clutter_actor_grab_get_grab_actor (ClutterActorGrab *grab);

#endif /* __CLUTTER_INPUT_DEVICE_H__ */
