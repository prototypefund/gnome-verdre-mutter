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

  /**
   * ClutterGestureClass::should_influence:
   * @self: the #ClutterGesture
   * @other_gesture: the #ClutterGesture that should be influenced
   * @cancel_on_recognizing: (inout): whether to cancel @other_gesture
   *   when @self recognizes
   * @inhibit_until_cancelled: (inout): whether to inhibit @other_gesture
   *   until @self got cancelled
   *
   * This virtual function is called to request whether @self should
   * influence @other_gesture, i.e. whether @other_gesture should be moved
   * to state CANCELLED when @self enters RECOGNIZING.
   */
  void (* should_influence) (ClutterGesture *self,
                             ClutterGesture *other_gesture,
                             gboolean       *cancel_on_recognizing,
                             gboolean       *inhibit_until_cancelled);

  /**
   * ClutterGestureClass::should_be_influenced_by:
   * @self: the #ClutterGesture
   * @other_gesture: the influencing #ClutterGesture
   * @cancelled_on_recognizing: (inout): whether @self should be cancelled
   *   when @other_gesture recognizes
   * @inhibited_until_cancelled: (inout): whether @self should be inhibited
   *   until @other_gesture got cancelled
   *
   * This virtual function is called to request whether @other_gesture should
   * influence @self, i.e. whether @self should be moved to state
   * CANCELLED when @other_gesture enters RECOGNIZING.
   */
  void (* should_be_influenced_by) (ClutterGesture *self,
                                    ClutterGesture *other_gesture,
                                    gboolean       *cancelled_on_recognizing,
                                    gboolean       *inhibited_until_cancelled);

  /**
   * ClutterGestureClass::should_start_while:
   * @self: the `ClutterGesture`
   * @recognizing_gesture: the already recognizing gesture
   * @should_start: (inout): whether @self should start
   *
   * This virtual function is called to request whether @self should start
   * while @recognizing_gesture is currently in state RECOGNIZING.
   * Implementations can override this function to specify their
   * relationship with any other gesture globally.
   *
   * By default, starting while another gesture is recognizing is disallowed.
   */
  void (* should_start_while) (ClutterGesture *self,
                               ClutterGesture *recognizing_gesture,
                               gboolean       *should_start);

  /**
   * ClutterGestureClass::other_gesture_may_start:
   * @self: the `ClutterGesture`
   * @other_gesture: the other gesture
   * @should_start: (inout): whether @other_gesture may start
   *
   * This virtual function is called to request whether @other_gesture may
   * start while @self is already RECOGNIZING.
   * Implementations can override this function to specify their
   * relationship with any other gesture globally.
   *
   * Overriding this virtual function takes precedence over an override of
   * ClutterGestureClass::should_start_while() done by @other_gesture.
   */
  void (* other_gesture_may_start) (ClutterGesture *self,
                                    ClutterGesture *other_gesture,
                                    gboolean       *should_start);
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
void clutter_gesture_set_wait_points_removed (ClutterGesture *self,
                                         gboolean        wait_points_removed);

CLUTTER_EXPORT
void clutter_gesture_set_allowed_device_types (ClutterGesture         *self,
                                               ClutterInputDeviceType *allowed_device_types,
                                               size_t                  n_device_types);

CLUTTER_EXPORT
void clutter_gesture_can_not_cancel (ClutterGesture *self,
                                     ClutterGesture *other_gesture);

CLUTTER_EXPORT
void clutter_gesture_require_failure_of (ClutterGesture *self,
                                         ClutterGesture *other_gesture);

CLUTTER_EXPORT
void clutter_gesture_recognize_independently_from (ClutterGesture *self,
                                                   ClutterGesture *other_gesture);

#endif /* __CLUTTER_GESTURE_H__ */
