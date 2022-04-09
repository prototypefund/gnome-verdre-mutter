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

/**
 * SECTION:clutter-long-press-action
 * @Title: ClutterLongPressGesture
 */

#include "clutter-build-config.h"

#include "clutter-long-press-gesture.h"

#include "clutter-enum-types.h"
#include "clutter-private.h"

typedef struct _ClutterLongPressGesturePrivate ClutterLongPressGesturePrivate;

struct _ClutterLongPressGesturePrivate
{
  int cancel_threshold;

  int long_press_duration;
  unsigned int long_press_timeout_id;

  unsigned int press_button;
  ClutterModifierType modifier_state;
};

enum
{
  PROP_0,

  PROP_CANCEL_THRESHOLD,
  PROP_LONG_PRESS_DURATION,

  PROP_LAST
};

enum
{
  LONG_PRESS_BEGIN,
  LONG_PRESS_END,
  LONG_PRESS_CANCEL,

  LAST_SIGNAL
};

static GParamSpec *obj_props[PROP_LAST] = { NULL, };
static unsigned int obj_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (ClutterLongPressGesture, clutter_long_press_gesture, CLUTTER_TYPE_GESTURE)

static unsigned int
get_default_long_press_duration (void)
{
  int long_press_duration;
  ClutterSettings *settings = clutter_settings_get_default ();

  g_object_get (settings, "long-press-duration",
                &long_press_duration, NULL);

  return long_press_duration;
}

static unsigned int
get_default_cancel_threshold (void)
{
  int threshold;
  ClutterSettings *settings = clutter_settings_get_default ();
  g_object_get (settings, "dnd-drag-threshold", &threshold, NULL);
  if (threshold < 0)
    return 0;

  return threshold;
}

static gboolean
long_press_cb (gpointer user_data)
{
  ClutterLongPressGesture *self = user_data;
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);

  clutter_gesture_set_state (CLUTTER_GESTURE (self),
                             CLUTTER_GESTURE_STATE_RECOGNIZING);

  priv->long_press_timeout_id = 0;
  return G_SOURCE_REMOVE;
}

static void
points_began (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gesture);
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);
  unsigned int n_gesture_points;
  const ClutterGesturePoint *point = points[0];

  clutter_gesture_get_points (gesture, &n_gesture_points);
  if (n_gesture_points > 1)
    {
      clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
      return;
    }

  /* Use the primary button as button number for touch events */
  priv->press_button =
    clutter_event_type (point->latest_event) == CLUTTER_BUTTON_PRESS
      ? clutter_event_get_button (point->latest_event) : CLUTTER_BUTTON_PRIMARY;

  priv->modifier_state = clutter_event_get_state (point->latest_event);
  if (priv->long_press_duration == 0)
    {
      clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_RECOGNIZING);
    }
  else
    {
      priv->long_press_timeout_id =
        g_timeout_add (priv->long_press_duration, long_press_cb, self);
    }
}

static void
points_moved (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gesture);
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);
  const ClutterGesturePoint *point = points[0];

  float distance =
    graphene_point_distance (&point->move_coords, &point->begin_coords, NULL, NULL);

  if (priv->cancel_threshold >= 0 && distance > priv->cancel_threshold)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
points_ended (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_RECOGNIZING)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_RECOGNIZED);
  else
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
points_cancelled (ClutterGesture             *gesture,
                  const ClutterGesturePoint **points,
                  unsigned int                n_points)
{
  clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
state_changed (ClutterGesture      *gesture,
               ClutterGestureState  old_state,
               ClutterGestureState  new_state)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gesture);
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);

  if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZING)
    g_signal_emit (self, obj_signals[LONG_PRESS_BEGIN], 0);

  if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZED)
    g_signal_emit (self, obj_signals[LONG_PRESS_END], 0);

  if (old_state == CLUTTER_GESTURE_STATE_RECOGNIZING &&
      new_state == CLUTTER_GESTURE_STATE_CANCELLED)
    g_signal_emit (self, obj_signals[LONG_PRESS_CANCEL], 0);

  if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZED ||
      new_state == CLUTTER_GESTURE_STATE_CANCELLED)
    {
      if (priv->long_press_timeout_id)
        {
          g_source_remove (priv->long_press_timeout_id);
          priv->long_press_timeout_id = 0;
        }

      priv->press_button = 0;
      priv->modifier_state = 0;
    }
}

static void
clutter_long_press_gesture_set_property (GObject      *gobject,
                                        unsigned int  prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_CANCEL_THRESHOLD:
      clutter_long_press_gesture_set_cancel_threshold (self, g_value_get_int (value));
      break;

    case PROP_LONG_PRESS_DURATION:
      clutter_long_press_gesture_set_long_press_duration (self, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_long_press_gesture_get_property (GObject      *gobject,
                                        unsigned int  prop_id,
                                        GValue       *value,
                                        GParamSpec   *pspec)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_CANCEL_THRESHOLD:
      g_value_set_int (value, clutter_long_press_gesture_get_cancel_threshold (self));
      break;

    case PROP_LONG_PRESS_DURATION:
      g_value_set_int (value, clutter_long_press_gesture_get_long_press_duration (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_long_press_gesture_dispose (GObject *gobject)
{
  ClutterLongPressGesture *self = CLUTTER_LONG_PRESS_GESTURE (gobject);
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);

  if (priv->long_press_timeout_id)
    {
      g_source_remove (priv->long_press_timeout_id);
      priv->long_press_timeout_id = 0;
    }

  G_OBJECT_CLASS (clutter_long_press_gesture_parent_class)->dispose (gobject);
}

static void
clutter_long_press_gesture_class_init (ClutterLongPressGestureClass *klass)
{
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = clutter_long_press_gesture_dispose;
  gobject_class->set_property = clutter_long_press_gesture_set_property;
  gobject_class->get_property = clutter_long_press_gesture_get_property;

  gesture_class->points_began = points_began;
  gesture_class->points_moved = points_moved;
  gesture_class->points_ended = points_ended;
  gesture_class->points_cancelled = points_cancelled;
  gesture_class->state_changed = state_changed;

  /**
   * ClutterLongPressGesture:cancel-threshold:
   *
   * Threshold in pixels to cancel the gesture, use -1 to disable the threshold.
   * The default will be set to the dnd-drag-threshold set in Clutter.
   */
  obj_props[PROP_CANCEL_THRESHOLD] =
    g_param_spec_int ("cancel-threshold",
                      P_("Cancel Threshold"),
                      P_("The cancel threshold in pixels"),
                      -1, G_MAXINT, -1,
                      CLUTTER_PARAM_READWRITE |
                      G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterLongPressGesture:long-press-duration:
   *
   * The minimum duration of a press in milliseconds for it to be recognized
   * as a long press gesture, .
   *
   * A value of -1 will make the #ClutterLongPressGesture use the value of
   * the #ClutterSettings:long-press-duration property.
   */
  obj_props[PROP_LONG_PRESS_DURATION] =
    g_param_spec_int ("long-press-duration",
                      P_("Long Press Duration"),
                      P_("The minimum duration of a long press to recognize the gesture"),
                      -1, G_MAXINT, -1,
                      CLUTTER_PARAM_READWRITE |
                      G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class,
                                     PROP_LAST,
                                     obj_props);

  /**
   * ClutterLongPressGesture::long-press-begin:
   * @gesture: the #ClutterLongPressGesture that emitted the signal
   *
   * The ::long-press signal is emitted during the long press gesture
   * handling.
   *
   * This signal can be emitted multiple times with different states.
   */
  obj_signals[LONG_PRESS_BEGIN] =
    g_signal_new (I_("long-press-begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);

  /**
   * ClutterLongPressGesture::long-press-end:
   * @gesture: the #ClutterLongPressGesture that emitted the signal
   *
   * The ::long-press signal is emitted during the long press gesture
   * handling.
   *
   * This signal can be emitted multiple times with different states.
   */
  obj_signals[LONG_PRESS_END] =
    g_signal_new (I_("long-press-end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);

  /**
   * ClutterLongPressGesture::long-press-cancel:
   * @gesture: the #ClutterLongPressGesture that emitted the signal
   *
   * The ::long-press signal is emitted during the long press gesture
   * handling.
   *
   * This signal can be emitted multiple times with different states.
   */
  obj_signals[LONG_PRESS_CANCEL] =
    g_signal_new (I_("long-press-cancel"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);
}

static void
clutter_long_press_gesture_init (ClutterLongPressGesture *self)
{
  ClutterLongPressGesturePrivate *priv =
    clutter_long_press_gesture_get_instance_private (self);

  priv->cancel_threshold = get_default_cancel_threshold ();
  priv->long_press_duration = get_default_long_press_duration ();
}

/**
 * clutter_long_press_gesture_new:
 *
 * Creates a new #ClutterLongPressGesture instance
 *
 * Returns: the newly created #ClutterLongPressGesture
 */
ClutterAction *
clutter_long_press_gesture_new (void)
{
  return g_object_new (CLUTTER_TYPE_LONG_PRESS_GESTURE, NULL);
}

/**
 * clutter_long_press_gesture_get_cancel_threshold:
 * @self: a #ClutterLongPressGesture
 *
 * Gets the movement threshold in pixels that cancels the click action.
 *
 * Returns: The cancel threshold in pixels
 */
int
clutter_long_press_gesture_get_cancel_threshold (ClutterLongPressGesture *self)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self), -1);

  priv = clutter_long_press_gesture_get_instance_private (self);

  return priv->cancel_threshold;
}

/**
 * clutter_long_press_gesture_set_cancel_threshold:
 * @self: a #ClutterLongPressGesture
 * @cancel_threshold: the threshold in pixels, or -1 to disable the threshold
 *
 * Sets the movement threshold in pixels that cancels the click action.
 */
void
clutter_long_press_gesture_set_cancel_threshold (ClutterLongPressGesture *self,
                                                 int                      cancel_threshold)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self));

  priv = clutter_long_press_gesture_get_instance_private (self);

  if (priv->cancel_threshold == cancel_threshold)
    return;

  priv->cancel_threshold = cancel_threshold;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_CANCEL_THRESHOLD]);
}

int
clutter_long_press_gesture_get_long_press_duration (ClutterLongPressGesture *self)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self), 0);

  priv = clutter_long_press_gesture_get_instance_private (self);

  return priv->long_press_duration;
}

void
clutter_long_press_gesture_set_long_press_duration (ClutterLongPressGesture *self,
                                                   int                     long_press_duration)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self));

  priv = clutter_long_press_gesture_get_instance_private (self);

  if (priv->long_press_duration == long_press_duration)
    return;

  priv->long_press_duration = long_press_duration;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_LONG_PRESS_DURATION]);
}

/**
 * clutter_long_press_gesture_get_button:
 * @self: a #ClutterLongPressGesture
 *
 * Retrieves the button that was pressed.
 *
 * Returns: the button value
 */
unsigned int
clutter_long_press_gesture_get_button (ClutterLongPressGesture *self)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self), 0);

  priv = clutter_long_press_gesture_get_instance_private (self);

  return priv->press_button;
}

/**
 * clutter_long_press_gesture_get_state:
 * @self: a #ClutterLongPressGesture
 *
 * Retrieves the modifier state of the click action.
 *
 * Returns: the modifier state parameter, or 0
 */
ClutterModifierType
clutter_long_press_gesture_get_state (ClutterLongPressGesture *self)
{
  ClutterLongPressGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self), 0);

  priv = clutter_long_press_gesture_get_instance_private (self);

  return priv->modifier_state;
}

/**
 * clutter_long_press_gesture_get_coords:
 * @self: a #ClutterLongPressGesture
 * @coords: (out): a #graphene_point_t
 *
 * Retrieves the modifier state of the click action.
 */
void
clutter_long_press_gesture_get_coords (ClutterLongPressGesture *self,
                                      graphene_point_t       *coords)
{
  const ClutterGesturePoint *points;
  unsigned int n_points;

  g_return_if_fail (CLUTTER_IS_LONG_PRESS_GESTURE (self));

  points = clutter_gesture_get_points (CLUTTER_GESTURE (self), &n_points);

  if (coords && n_points > 0)
    *coords = points[0].latest_coords;
}
