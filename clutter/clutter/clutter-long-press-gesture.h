/*
 * Copyright (C) 2022 Jonas Dreßler <verdre@v0yd.nl>
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
 */

#if !defined(__CLUTTER_H_INSIDE__) && !defined(CLUTTER_COMPILATION)
#error "Only <clutter/clutter.h> can be included directly."
#endif

#ifndef __CLUTTER_LONG_PRESS_GESTURE_H__
#define __CLUTTER_LONG_PRESS_GESTURE_H__

#include <clutter/clutter-gesture.h>

#define CLUTTER_TYPE_LONG_PRESS_GESTURE (clutter_long_press_gesture_get_type ())

CLUTTER_EXPORT
G_DECLARE_DERIVABLE_TYPE (ClutterLongPressGesture, clutter_long_press_gesture,
                          CLUTTER, LONG_PRESS_GESTURE, ClutterGesture)

typedef struct _ClutterLongPressGestureClass ClutterLongPressGestureClass;

/**
 * ClutterLongPressGestureClass:
 */
struct _ClutterLongPressGestureClass
{
  /*< private >*/
  ClutterGestureClass parent_class;
};

CLUTTER_EXPORT
ClutterAction * clutter_long_press_gesture_new (void);

CLUTTER_EXPORT
int clutter_long_press_gesture_get_cancel_threshold (ClutterLongPressGesture *self);

CLUTTER_EXPORT
void clutter_long_press_gesture_set_cancel_threshold (ClutterLongPressGesture *self,
                                                      int                      cancel_threshold);

CLUTTER_EXPORT
int clutter_long_press_gesture_get_long_press_duration (ClutterLongPressGesture *self);

CLUTTER_EXPORT
void clutter_long_press_gesture_set_long_press_duration (ClutterLongPressGesture *self,
                                                         int                      long_press_duration);

CLUTTER_EXPORT
unsigned int clutter_long_press_gesture_get_button (ClutterLongPressGesture *self);

CLUTTER_EXPORT
ClutterModifierType clutter_long_press_gesture_get_state (ClutterLongPressGesture *self);

CLUTTER_EXPORT
void clutter_long_press_gesture_get_coords (ClutterLongPressGesture *self,
                                            graphene_point_t        *coords);

#endif /* __CLUTTER_LONG_PRESS_GESTURE_H__ */
