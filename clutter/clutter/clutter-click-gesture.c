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
 * SECTION:clutter-tap-gesture
 * @Title: ClutterClickGesture
 * @Short_Description: Action for tap gestures
 */

#include "clutter-build-config.h"

#include "clutter-click-gesture.h"

#include "clutter-debug.h"
#include "clutter-enum-types.h"
#include "clutter-marshal.h"
#include "clutter-private.h"

typedef struct _ClutterClickGesturePrivate ClutterClickGesturePrivate;

struct _ClutterClickGesturePrivate
{
  gboolean pressed;

  int cancel_threshold;

  unsigned int n_clicks_required;
  unsigned int n_clicks_happened;
  unsigned int next_click_timeout_id;

  gboolean is_touch;

  graphene_point_t press_coords;
  unsigned int press_button;
  ClutterModifierType modifier_state;
};

enum
{
  PROP_0,

  PROP_CANCEL_THRESHOLD,
  PROP_N_CLICKS_REQUIRED,
  PROP_PRESSED,

  PROP_LAST
};

enum
{
  CLICKED,

  LAST_SIGNAL
};

static GParamSpec *obj_props[PROP_LAST] = { NULL, };
static unsigned int obj_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (ClutterClickGesture, clutter_click_gesture, CLUTTER_TYPE_GESTURE)

static unsigned int
get_next_click_timeout_ms (void)
{
  ClutterSettings *settings = clutter_settings_get_default ();
  int double_click_time_ms;

  g_object_get (settings, "double-click-time",
                &double_click_time_ms, NULL);

  if (double_click_time_ms < 0)
    return 100;

  return double_click_time_ms;
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

static void
set_pressed (ClutterClickGesture *self,
             gboolean            pressed)
{
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);

  if (priv->pressed == pressed)
    return;

  priv->pressed = pressed;
  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_PRESSED]);
}

static gboolean
next_click_timed_out (gpointer user_data)
{
  ClutterClickGesture *self = user_data;
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);

  clutter_gesture_set_state (CLUTTER_GESTURE (self),
                             CLUTTER_GESTURE_STATE_CANCELLED);
  set_pressed (self, FALSE);

  priv->next_click_timeout_id = 0;
  return G_SOURCE_REMOVE;
}

static void
points_began (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gesture);
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);
  const ClutterGesturePoint *point = points[0];
  unsigned int total_n_points;
  gboolean is_touch;
  unsigned int press_button;
  ClutterModifierType modifier_state;

  clutter_gesture_get_points (gesture, &total_n_points);
  if (total_n_points != 1)
    {
      clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
      return;
    }

  if (priv->next_click_timeout_id)
    {
      g_source_remove (priv->next_click_timeout_id);
      priv->next_click_timeout_id = 0;
    }

  is_touch = clutter_event_type (point->latest_event) == CLUTTER_TOUCH_BEGIN;
  press_button = is_touch ? 0 : clutter_event_get_button (point->latest_event);
  modifier_state = clutter_event_get_state (point->latest_event);

  if (priv->n_clicks_happened == 0)
    {
      priv->is_touch = is_touch;
      priv->press_button = press_button;
      priv->modifier_state = modifier_state;
      priv->press_coords = point->begin_coords;
    }
  else
    {
      float distance =
        graphene_point_distance (&priv->press_coords, &point->begin_coords, NULL, NULL);

      if (priv->is_touch != is_touch ||
          priv->press_button != press_button ||
          (priv->cancel_threshold >= 0 && distance > priv->cancel_threshold))
        {
          set_pressed (self, FALSE);
          clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
          return;
        }
    }

  if (priv->n_clicks_required > 1)
    {
      priv->next_click_timeout_id =
        g_timeout_add (get_next_click_timeout_ms (), next_click_timed_out, self);
    }

  set_pressed (self, TRUE);
}

static void
points_moved (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gesture);
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);
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
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gesture);
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);
  const ClutterGesturePoint *point = points[0];

  priv->n_clicks_happened += 1;

  if (priv->n_clicks_happened == priv->n_clicks_required)
    {
      ClutterModifierType modifier_state;

      if (priv->next_click_timeout_id)
        {
          g_source_remove (priv->next_click_timeout_id);
          priv->next_click_timeout_id = 0;
        }

      /* Exclude any button-mask so that we can compare
       * the press and release states properly */
      modifier_state = clutter_event_get_state (point->latest_event) &
        ~(CLUTTER_BUTTON1_MASK | CLUTTER_BUTTON2_MASK | CLUTTER_BUTTON3_MASK |
          CLUTTER_BUTTON4_MASK | CLUTTER_BUTTON5_MASK);

      /* if press and release states don't match we
       * simply ignore modifier keys. i.e. modifier keys
       * are expected to be pressed throughout the whole
       * click */
      if (modifier_state != priv->modifier_state)
        priv->modifier_state = 0;

      if (priv->pressed)
        clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_COMPLETED);
      else
        clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
    }

  set_pressed (self, FALSE);
}

static void
points_cancelled (ClutterGesture             *gesture,
                  const ClutterGesturePoint **points,
                  unsigned int                n_points)
{
  clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
}

static void
crossing_event (ClutterGesture            *gesture,
                const ClutterGesturePoint *point,
                ClutterEventType           type,
                uint32_t                   time,
                ClutterEventFlags          flags,
                ClutterActor              *source_actor,
                ClutterActor              *related_actor)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gesture);
  ClutterActor *actor;

  actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (self));

  if (source_actor == actor)
    set_pressed (self, type == CLUTTER_ENTER);
}

static void
state_changed (ClutterGesture      *gesture,
               ClutterGestureState  old_state,
               ClutterGestureState  new_state)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gesture);
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);

  if (new_state == CLUTTER_GESTURE_STATE_COMPLETED)
    g_signal_emit (self, obj_signals[CLICKED], 0);

  if (new_state == CLUTTER_GESTURE_STATE_COMPLETED ||
      new_state == CLUTTER_GESTURE_STATE_CANCELLED)
    {
      set_pressed (self, FALSE);

      if (priv->next_click_timeout_id)
        {
          g_source_remove (priv->next_click_timeout_id);
          priv->next_click_timeout_id = 0;
        }

      priv->n_clicks_happened = 0;
      priv->press_coords.x = 0;
      priv->press_coords.y = 0;
      priv->press_button = 0;
      priv->modifier_state = 0;
    }
}

static void
clutter_click_gesture_set_property (GObject      *gobject,
                                   unsigned int  prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_CANCEL_THRESHOLD:
      clutter_click_gesture_set_cancel_threshold (self, g_value_get_int (value));
      break;

    case PROP_N_CLICKS_REQUIRED:
      clutter_click_gesture_set_n_clicks_required (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_click_gesture_get_property (GObject      *gobject,
                                   unsigned int  prop_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  ClutterClickGesture *self = CLUTTER_CLICK_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_CANCEL_THRESHOLD:
      g_value_set_int (value, clutter_click_gesture_get_cancel_threshold (self));
      break;

    case PROP_N_CLICKS_REQUIRED:
      g_value_set_uint (value, clutter_click_gesture_get_n_clicks_required (self));
      break;

    case PROP_PRESSED:
      g_value_set_boolean (value, clutter_click_gesture_get_pressed (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_click_gesture_class_init (ClutterClickGestureClass *klass)
{
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = clutter_click_gesture_set_property;
  gobject_class->get_property = clutter_click_gesture_get_property;

  gesture_class->points_began = points_began;
  gesture_class->points_moved = points_moved;
  gesture_class->points_ended = points_ended;
  gesture_class->points_cancelled = points_cancelled;
  gesture_class->crossing_event = crossing_event;
  gesture_class->state_changed = state_changed;

  /**
   * ClutterClickGesture:cancel-threshold:
   *
   * Threshold in pixels to cancel the gesture, use -1 to disable the threshold.
   * The default will be set to the dnd-drag-threshold set in Clutter.
   */
  obj_props[PROP_CANCEL_THRESHOLD] =
    g_param_spec_int ("cancel-threshold",
                      P_("cancel-threshold"),
                      P_("cancel-threshold"),
                      -1, G_MAXINT, 0,
                      CLUTTER_PARAM_READWRITE |
                      G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterClickGesture:n-clicks-required:
   *
   * The number of clicks required for the gesture to recognize, this can
   * be used to implement double-click gestures.
   */
  obj_props[PROP_N_CLICKS_REQUIRED] =
    g_param_spec_uint ("n-clicks-required",
                       P_("n-clicks-required"),
                       P_("n-clicks-required"),
                       1, G_MAXUINT, 1,
                       CLUTTER_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterClickGesture:pressed:
   *
   * Whether the clickable actor should be in "pressed" state
   */
  obj_props[PROP_PRESSED] =
    g_param_spec_boolean ("pressed",
                          P_("Actor pressed"),
                          P_("Whether the clickable should be in pressed state"),
                          FALSE,
                          CLUTTER_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class,
                                     PROP_LAST,
                                     obj_props);

  /**
   * ClutterClickGesture::clicked:
   * @gesture: the #ClutterClickGesture that emitted the signal
   *
   * The ::clicked signal is emitted when the #ClutterActor to which
   * a #ClutterClickGesture has been applied should respond to a
   * pointer button press and release events
   */
  obj_signals[CLICKED] =
    g_signal_new (I_("clicked"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0, G_TYPE_NONE);
}

static void
clutter_click_gesture_init (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv =
    clutter_click_gesture_get_instance_private (self);

  priv->pressed = FALSE;

  priv->cancel_threshold = get_default_cancel_threshold ();

  priv->n_clicks_required = 1;
  priv->n_clicks_happened = 0;
  priv->next_click_timeout_id = 0;

  priv->press_button = 0;
  priv->modifier_state = 0;
}

/**
 * clutter_click_gesture_new:
 *
 * Creates a new #ClutterClickGesture instance
 *
 * Returns: the newly created #ClutterClickGesture
 */
ClutterAction *
clutter_click_gesture_new (void)
{
  return g_object_new (CLUTTER_TYPE_CLICK_GESTURE, NULL);
}

/**
 * clutter_click_gesture_get_pressed:
 * @self: a #ClutterClickGesture
 *
 * Gets whether the click actions actor should be in the "pressed" state.
 *
 * Returns: The "pressed" state
 */
gboolean
clutter_click_gesture_get_pressed (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_CLICK_GESTURE (self), FALSE);

  priv = clutter_click_gesture_get_instance_private (self);

  return priv->pressed;
}

/**
 * clutter_click_gesture_get_cancel_threshold:
 * @self: a #ClutterClickGesture
 *
 * Gets the movement threshold in pixels that cancels the click action.
 *
 * Returns: The cancel threshold in pixels
 */
int
clutter_click_gesture_get_cancel_threshold (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_CLICK_GESTURE (self), -1);

  priv = clutter_click_gesture_get_instance_private (self);

  return priv->cancel_threshold;
}

/**
 * clutter_click_gesture_set_cancel_threshold:
 * @self: a #ClutterClickGesture
 * @cancel_threshold: the threshold in pixels, or -1 to disable the threshold
 *
 * Sets the movement threshold in pixels that cancels the click action.
 */
void
clutter_click_gesture_set_cancel_threshold (ClutterClickGesture *self,
                                           int                 cancel_threshold)
{
  ClutterClickGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CLICK_GESTURE (self));

  priv = clutter_click_gesture_get_instance_private (self);

  if (priv->cancel_threshold == cancel_threshold)
    return;

  priv->cancel_threshold = cancel_threshold;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_CANCEL_THRESHOLD]);
}


/**
 * clutter_click_gesture_get_n_clicks_required:
 * @self: a #ClutterClickGesture
 *
 * Gets the number of clicks required for the click action to recognize.
 *
 * Returns: The number of clicks
 */
unsigned int
clutter_click_gesture_get_n_clicks_required (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_CLICK_GESTURE (self), FALSE);

  priv = clutter_click_gesture_get_instance_private (self);

  return priv->n_clicks_required;
}

/**
 * clutter_click_gesture_set_n_clicks_required:
 * @self: a #ClutterClickGesture
 * @n_clicks_required: the number of clicks required
 *
 * Sets the number of clicks required for the gesture to recognize, this can
 * be used to implement double-click gestures.
 */
void
clutter_click_gesture_set_n_clicks_required (ClutterClickGesture *self,
                                            unsigned int        n_clicks_required)
{
  ClutterClickGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CLICK_GESTURE (self));

  priv = clutter_click_gesture_get_instance_private (self);

  if (priv->n_clicks_required == n_clicks_required)
    return;

  priv->n_clicks_required = n_clicks_required;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_N_CLICKS_REQUIRED]);
}

/**
 * clutter_click_gesture_get_button:
 * @self: a #ClutterClickGesture
 *
 * Retrieves the button that was pressed.
 *
 * Returns: the button value
 */
unsigned int
clutter_click_gesture_get_button (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_CLICK_GESTURE (self), 0);

  priv = clutter_click_gesture_get_instance_private (self);

  return priv->press_button;
}

/**
 * clutter_click_gesture_get_state:
 * @self: a #ClutterClickGesture
 *
 * Retrieves the modifier state of the click action.
 *
 * Returns: the modifier state parameter, or 0
 */
ClutterModifierType
clutter_click_gesture_get_state (ClutterClickGesture *self)
{
  ClutterClickGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_CLICK_GESTURE (self), 0);

  priv = clutter_click_gesture_get_instance_private (self);

  return priv->modifier_state;
}

/**
 * clutter_click_gesture_get_coords:
 * @self: a #ClutterClickGesture
 * @coords: (out): #graphene_point_t
 *
 * Retrieves the coordinates of the click, should be used inside a ::clicked
 * signal handler.
 */
void
clutter_click_gesture_get_coords (ClutterClickGesture *self,
                                  graphene_point_t    *coords)
{
  ClutterClickGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CLICK_GESTURE (self));

  priv = clutter_click_gesture_get_instance_private (self);

  if (coords)
    *coords = priv->press_coords;
}
