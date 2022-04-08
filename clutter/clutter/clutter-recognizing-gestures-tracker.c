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

#include "clutter-build-config.h"

#include "clutter-recognizing-gestures-tracker-private.h"

#include "clutter-enum-types.h"
#include "clutter-private.h"

struct _ClutterRecognizingGesturesTracker
{
  ClutterGesture parent;
};

G_DEFINE_TYPE (ClutterRecognizingGesturesTracker, clutter_recognizing_gestures_tracker, CLUTTER_TYPE_GESTURE)

static void
points_began (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
}

static void
points_moved (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
}

static void
points_ended (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  unsigned int n_gesture_points;

  clutter_gesture_get_points (gesture, &n_gesture_points);
  if (n_gesture_points == 0)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
points_cancelled (ClutterGesture             *gesture,
                  const ClutterGesturePoint **points,
                  unsigned int                n_points)
{
  unsigned int n_gesture_points;

  clutter_gesture_get_points (gesture, &n_gesture_points);
  if (n_gesture_points == 0)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
clutter_recognizing_gestures_tracker_class_init (ClutterRecognizingGesturesTrackerClass *klass)
{
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_CLASS (klass);

  gesture_class->points_began = points_began;
  gesture_class->points_moved = points_moved;
  gesture_class->points_ended = points_ended;
  gesture_class->points_cancelled = points_cancelled;
}

static void
clutter_recognizing_gestures_tracker_init (ClutterRecognizingGesturesTracker *self)
{
}
