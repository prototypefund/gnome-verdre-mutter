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

#ifndef __CLUTTER_RECOGNIZING_GESTURES_TRACKER_H__
#define __CLUTTER_RECOGNIZING_GESTURES_TRACKER_H__

#include <clutter/clutter-gesture.h>

#define CLUTTER_TYPE_RECOGNIZING_GESTURES_TRACKER (clutter_recognizing_gestures_tracker_get_type ())

CLUTTER_EXPORT
G_DECLARE_FINAL_TYPE (ClutterRecognizingGesturesTracker, clutter_recognizing_gestures_tracker,
                      CLUTTER, RECOGNIZING_GESTURES_TRACKER, ClutterGesture)

ClutterRecognizingGesturesTracker * clutter_recognizing_gestures_tracker_new (void);

#endif /* __CLUTTER_RECOGNIZING_GESTURES_TRACKER_H__ */
