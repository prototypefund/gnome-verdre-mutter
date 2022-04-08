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

#ifndef __CLUTTER_GESTURE_H__
#define __CLUTTER_GESTURE_H__

#if !defined(__CLUTTER_H_INSIDE__) && !defined(CLUTTER_COMPILATION)
#error "Only <clutter/clutter.h> can be included directly."
#endif

#include <clutter/clutter-action.h>

#define CLUTTER_TYPE_GESTURE (clutter_gesture_get_type ())

CLUTTER_EXPORT
G_DECLARE_DERIVABLE_TYPE (ClutterGesture, clutter_gesture,
                          CLUTTER, GESTURE, ClutterAction)

typedef struct _ClutterGestureClass ClutterGestureClass;

/**
 * ClutterGestureClass:
 */
struct _ClutterGestureClass
{
  /*< private >*/
  ClutterActionClass parent_class;

  /*< public >*/
  /**
   * ClutterGestureClass::points_began:
   * @self: the `ClutterGesture`
   * @points: (array length=n_points): the array of points
   * @n_points: the number of elements in @points
   *
   * Emitted when one or more points have begun.
   */
  void (* points_began) (ClutterGesture             *self,
                         const ClutterGesturePoint **points,
                         unsigned int                n_points);

  /**
   * ClutterGestureClass::points_moved:
   * @self: the `ClutterGesture`
   * @points: (array length=n_points): the array of points
   * @n_points: the number of elements in @points
   *
   * Emitted when one or more points have moved.
   */
  void (* points_moved) (ClutterGesture             *self,
                         const ClutterGesturePoint **points,
                         unsigned int                n_points);

  /**
   * ClutterGestureClass::points_ended:
   * @self: the `ClutterGesture`
   * @points: (array length=n_points): the array of points
   * @n_points: the number of elements in @points
   *
   * Emitted when one or more points have ended.
   */
  void (* points_ended) (ClutterGesture             *self,
                         const ClutterGesturePoint **points,
                         unsigned int                n_points);

  /**
   * ClutterGestureClass::points_cancelled:
   * @self: the `ClutterGesture`
   * @points: (array length=n_points): the array of points
   * @n_points: the number of elements in @points
   *
   * Emitted when one or more points have been cancelled.
   */
  void (* points_cancelled) (ClutterGesture             *self,
                             const ClutterGesturePoint **points,
                             unsigned int                n_points);

  void (* state_changed) (ClutterGesture      *self,
                          ClutterGestureState  old_state,
                          ClutterGestureState  new_state);

  void (* crossing_event) (ClutterGesture            *self,
                           const ClutterGesturePoint *point,
                           ClutterEventType           type,
                           uint32_t                   time,
                           ClutterEventFlags          flags,
                           ClutterActor              *source_actor,
                           ClutterActor              *related_actor);

  gboolean (* may_recognize) (ClutterGesture *self);
};

CLUTTER_EXPORT
ClutterAction * clutter_gesture_new (void);

CLUTTER_EXPORT
void clutter_gesture_set_state (ClutterGesture      *self,
                                ClutterGestureState  state);

CLUTTER_EXPORT
ClutterGestureState clutter_gesture_get_state (ClutterGesture *self);

CLUTTER_EXPORT
const ClutterGesturePoint * clutter_gesture_get_points (ClutterGesture *self,
                                                        unsigned int   *n_points);

CLUTTER_EXPORT
void clutter_gesture_set_allowed_device_types (ClutterGesture         *self,
                                               ClutterInputDeviceType *allowed_device_types,
                                               size_t                  n_device_types);

#endif /* __CLUTTER_GESTURE_H__ */
