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

#ifndef __CLUTTER_PAN_GESTURE_H__
#define __CLUTTER_PAN_GESTURE_H__

#include <clutter/clutter-gesture.h>

#define CLUTTER_TYPE_PAN_GESTURE (clutter_pan_gesture_get_type ())

CLUTTER_EXPORT
G_DECLARE_DERIVABLE_TYPE (ClutterPanGesture, clutter_pan_gesture,
                          CLUTTER, PAN_GESTURE, ClutterGesture)

typedef struct _ClutterPanGestureClass ClutterPanGestureClass;

/**
 * ClutterPanGestureClass:
 */
struct _ClutterPanGestureClass
{
  /*< private >*/
  ClutterGestureClass parent_class;
};


CLUTTER_EXPORT
ClutterAction * clutter_pan_gesture_new (void);

CLUTTER_EXPORT
unsigned int clutter_pan_gesture_get_begin_threshold (ClutterPanGesture *self);

CLUTTER_EXPORT
void clutter_pan_gesture_set_begin_threshold (ClutterPanGesture *self,
                                              unsigned int       begin_threshold);

CLUTTER_EXPORT
void clutter_pan_gesture_set_pan_axis (ClutterPanGesture *self,
                                       ClutterPanAxis     axis);

CLUTTER_EXPORT
ClutterPanAxis clutter_pan_gesture_get_pan_axis (ClutterPanGesture *self);

CLUTTER_EXPORT
void clutter_pan_gesture_set_min_n_points (ClutterPanGesture *self,
                                           unsigned int       min_n_points);

CLUTTER_EXPORT
unsigned int clutter_pan_gesture_get_min_n_points (ClutterPanGesture *self);

CLUTTER_EXPORT
void clutter_pan_gesture_set_max_n_points (ClutterPanGesture *self,
                                           unsigned int       max_n_points);

CLUTTER_EXPORT
unsigned int clutter_pan_gesture_get_max_n_points (ClutterPanGesture *self);

#endif /* __CLUTTER_PAN_GESTURE_H__ */
