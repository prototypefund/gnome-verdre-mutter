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
 * SECTION:clutter-pan-action
 * @Title: ClutterPanGesture
 * @Short_Description: Action for pan gestures
 *
 * #ClutterPanGesture is a sub-class of #ClutterGesture that implements
 * the logic for recognizing pan gestures.
 */

#include "clutter-build-config.h"

#include "clutter-pan-gesture.h"

#include "clutter-enum-types.h"
#include "clutter-private.h"

#define DEFAULT_BEGIN_THRESHOLD_PX 16

#define EVENT_HISTORY_DURATION_MS 150
#define EVENT_HISTORY_MIN_STORE_INTERVAL_MS 1
#define EVENT_HISTORY_MAX_LENGTH (EVENT_HISTORY_DURATION_MS / EVENT_HISTORY_MIN_STORE_INTERVAL_MS)

typedef struct
{
  graphene_vec2_t delta;
  uint32_t time;
} HistoryEntry;

typedef struct _ClutterPanGesturePrivate ClutterPanGesturePrivate;

struct _ClutterPanGesturePrivate
{
  int begin_threshold;

  GArray *event_history;
  unsigned int event_history_begin_index;

  graphene_vec2_t total_delta;

  ClutterPanAxis pan_axis;

  unsigned int min_n_points;
  unsigned int max_n_points;

  unsigned int use_point;
};

enum
{
  PROP_0,

  PROP_BEGIN_THRESHOLD,
  PROP_PAN_AXIS,
  PROP_MIN_N_POINTS,
  PROP_MAX_N_POINTS,

  PROP_LAST
};


enum
{
  PAN_BEGIN,
  PAN_UPDATE,
  PAN_END,
  PAN_CANCEL,

  LAST_SIGNAL
};

static GParamSpec *obj_props[PROP_LAST] = { NULL, };
static unsigned int obj_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (ClutterPanGesture, clutter_pan_gesture, CLUTTER_TYPE_GESTURE)

static void
add_delta_to_event_history (ClutterPanGesture     *self,
                            const graphene_vec2_t *delta,
                            uint32_t               time)
{
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);
  HistoryEntry *last_history_entry, *history_entry;

  last_history_entry = priv->event_history->len == 0
    ? NULL
    : &g_array_index (priv->event_history,
                      HistoryEntry,
                      (priv->event_history_begin_index - 1) % EVENT_HISTORY_MAX_LENGTH);

  if (last_history_entry &&
      last_history_entry->time > (time - EVENT_HISTORY_MIN_STORE_INTERVAL_MS))
    return;

  if (priv->event_history->len < EVENT_HISTORY_MAX_LENGTH)
    g_array_set_size (priv->event_history, priv->event_history->len + 1);

  history_entry =
    &g_array_index (priv->event_history, HistoryEntry, priv->event_history_begin_index);

  history_entry->delta = *delta;
  history_entry->time = time;

  priv->event_history_begin_index =
    (priv->event_history_begin_index + 1) % EVENT_HISTORY_MAX_LENGTH;
}

static void
calculate_velocity (ClutterPanGesture *self,
                    uint32_t           latest_event_time,
                    graphene_vec2_t   *velocity)
{
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);
  unsigned int i, j;
  uint32_t first_time = 0;
  uint32_t last_time = 0;
  uint32_t time_delta;
  graphene_vec2_t accumulated_deltas = { 0 };

  j = priv->event_history_begin_index;

  for (i = 0; i < priv->event_history->len; i++)
    {
      HistoryEntry *history_entry;

      if (j == priv->event_history->len)
        j = 0;

      history_entry = &g_array_index (priv->event_history, HistoryEntry, j);

      if (history_entry->time >= latest_event_time - EVENT_HISTORY_DURATION_MS)
        {
          if (first_time == 0)
            first_time = history_entry->time;

          graphene_vec2_add (&accumulated_deltas, &history_entry->delta, &accumulated_deltas);

          last_time = history_entry->time;
        }

      j++;
    }

  if (first_time == last_time)
    {
      graphene_vec2_init (velocity, 0, 0);
      return;
    }

  time_delta = last_time - first_time;
  graphene_vec2_init (velocity,
                      graphene_vec2_get_x (&accumulated_deltas) / time_delta,
                      graphene_vec2_get_y (&accumulated_deltas) / time_delta);
}

static void
get_delta_from_points (const ClutterGesturePoint **points,
                       unsigned int                n_points,
                       graphene_vec2_t            *delta)
{
  graphene_vec2_t biggest_pos_delta, biggest_neg_delta;
  unsigned int i;

  graphene_vec2_init (&biggest_pos_delta, 0, 0);
  graphene_vec2_init (&biggest_neg_delta, 0, 0);

  for (i = 0; i < n_points; i++)
    {
      const ClutterGesturePoint *point = points[i];
      float point_d_x = point->move_coords.x - point->last_coords.x;
      float point_d_y = point->move_coords.y - point->last_coords.y;

      if (point_d_x > 0)
        {
          /* meh, graphene API is quite annoying here */
          graphene_vec2_init (&biggest_pos_delta,
                              MAX (point_d_x, graphene_vec2_get_x (&biggest_pos_delta)),
                              graphene_vec2_get_y (&biggest_pos_delta));
        }
      else
        {
          graphene_vec2_init (&biggest_neg_delta,
                              MIN (point_d_x, graphene_vec2_get_x (&biggest_neg_delta)),
                              graphene_vec2_get_y (&biggest_neg_delta));
        }

      if (point_d_y > 0)
        {
          graphene_vec2_init (&biggest_pos_delta,
                              graphene_vec2_get_x (&biggest_pos_delta),
                              MAX (point_d_y, graphene_vec2_get_y (&biggest_pos_delta)));

        }
      else
        {
          graphene_vec2_init (&biggest_neg_delta,
                              graphene_vec2_get_x (&biggest_neg_delta),
                              MIN (point_d_y, graphene_vec2_get_y (&biggest_neg_delta)));
        }
    }

  graphene_vec2_add (&biggest_pos_delta, &biggest_neg_delta, delta);
}

static void
points_began (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gesture);
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);
  unsigned int total_n_points;

  clutter_gesture_get_points (gesture, &total_n_points);
  if (total_n_points < priv->min_n_points)
    return;

  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE &&
      priv->max_n_points != 0 && total_n_points > priv->max_n_points)
    {
      clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
      return;
    }

  if (priv->event_history->len == 0)
    add_delta_to_event_history (self, graphene_vec2_zero (), points[0]->event_time);

  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE &&
      priv->begin_threshold == 0)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_RECOGNIZING);

  /* If we're already recognizing, set state again to claim the new point, too */
  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_RECOGNIZING)
    clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_RECOGNIZING);

  priv->use_point = points[0]->index;
}

static void
points_moved (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gesture);
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);
  unsigned int total_n_points;
  graphene_vec2_t delta;
  float total_distance;

  clutter_gesture_get_points (gesture, &total_n_points);

  /* Right now we never see n_points > 1, because Clutter doesn't have handing
   * for TOUCH_FRAME events and will deliver us every point by itself.
   * When that's working at some point, we'll make use of it in
   * get_delta_from_points() to handle multi-finger pans nicely.
   * For now, we only look at the first point and ignore all other events that
   * happened at the same time.
   */
  if (points[0]->index != priv->use_point)
    return;

  get_delta_from_points (points, n_points, &delta);
  add_delta_to_event_history (self, &delta, points[0]->event_time);

  graphene_vec2_add (&priv->total_delta, &delta, &priv->total_delta);

  total_distance = graphene_vec2_length (&priv->total_delta);

  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE &&
      total_n_points >= priv->min_n_points &&
      (priv->max_n_points != 0 && total_n_points <= priv->max_n_points))
    {
      if ((priv->pan_axis == CLUTTER_PAN_AXIS_BOTH &&
           total_distance >= priv->begin_threshold) ||
          (priv->pan_axis == CLUTTER_PAN_AXIS_X &&
           ABS (graphene_vec2_get_x (&priv->total_delta)) >= priv->begin_threshold) ||
          (priv->pan_axis == CLUTTER_PAN_AXIS_Y &&
           ABS (graphene_vec2_get_y (&priv->total_delta)) >= priv->begin_threshold))
        clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_RECOGNIZING);
    }

  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_RECOGNIZING)
    {
      g_signal_emit (self,
                     obj_signals[PAN_UPDATE], 0,
                     graphene_vec2_get_x (&delta),
                     graphene_vec2_get_y (&delta),
                     total_distance);
    }
}

static void
points_ended (ClutterGesture             *gesture,
              const ClutterGesturePoint **points,
              unsigned int                n_points)
{
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gesture);
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);
  unsigned int total_n_points;
  const ClutterGesturePoint *all_points =
    clutter_gesture_get_points (gesture, &total_n_points);

  if (total_n_points - n_points >= priv->min_n_points)
    {
//g_warning("ENDED using point %d now, total %d", point->index, total_n_points);
      priv->use_point = all_points[0].index != points[0]->index
        ? all_points[0].index : all_points[1].index;

      return;
    }

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
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gesture);
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);

  if (old_state != CLUTTER_GESTURE_STATE_RECOGNIZING &&
      new_state == CLUTTER_GESTURE_STATE_RECOGNIZING)
    {
      unsigned int n_points;
      const ClutterGesturePoint *point =
        &clutter_gesture_get_points (gesture, &n_points)[0];

      g_signal_emit (self,
                     obj_signals[PAN_BEGIN], 0,
                     point->begin_coords.x,
                     point->begin_coords.y);
    }

  if (old_state == CLUTTER_GESTURE_STATE_RECOGNIZING &&
      new_state == CLUTTER_GESTURE_STATE_RECOGNIZED) {
      graphene_vec2_t velocity;
      const ClutterGesturePoint *point =
        &clutter_gesture_get_points (gesture, NULL)[0];

      calculate_velocity (self, point->event_time, &velocity);

      g_signal_emit (self,
                     obj_signals[PAN_END], 0,
                     graphene_vec2_get_x (&velocity),
                     graphene_vec2_get_y (&velocity));
  }

  if (old_state == CLUTTER_GESTURE_STATE_RECOGNIZING &&
      new_state == CLUTTER_GESTURE_STATE_CANCELLED)
      g_signal_emit (self, obj_signals[PAN_CANCEL], 0);

  if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZED ||
      new_state == CLUTTER_GESTURE_STATE_CANCELLED)
    {
      graphene_vec2_init (&priv->total_delta, 0, 0);
      priv->event_history_begin_index = 0;
      g_array_set_size (priv->event_history, 0);
    }
}

static void
clutter_pan_gesture_set_property (GObject      *gobject,
                                  unsigned int  prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_BEGIN_THRESHOLD:
      clutter_pan_gesture_set_begin_threshold (self, g_value_get_uint (value));
      break;

    case PROP_PAN_AXIS:
      clutter_pan_gesture_set_pan_axis (self, g_value_get_enum (value));
      break;

    case PROP_MIN_N_POINTS:
      clutter_pan_gesture_set_min_n_points (self, g_value_get_uint (value));
      break;

    case PROP_MAX_N_POINTS:
      clutter_pan_gesture_set_max_n_points (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
clutter_pan_gesture_get_property (GObject      *gobject,
                                  unsigned int  prop_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  ClutterPanGesture *self = CLUTTER_PAN_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_BEGIN_THRESHOLD:
      g_value_set_uint (value, clutter_pan_gesture_get_begin_threshold (self));
      break;

    case PROP_PAN_AXIS:
      g_value_set_enum (value, clutter_pan_gesture_get_pan_axis (self));
      break;

    case PROP_MIN_N_POINTS:
      g_value_set_uint (value, clutter_pan_gesture_get_min_n_points (self));
      break;

    case PROP_MAX_N_POINTS:
      g_value_set_uint (value, clutter_pan_gesture_get_max_n_points (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
clutter_pan_gesture_finalize (GObject *gobject)
{
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (CLUTTER_PAN_GESTURE (gobject));

  g_array_unref (priv->event_history);

  G_OBJECT_CLASS (clutter_pan_gesture_parent_class)->finalize (gobject);
}

static void
clutter_pan_gesture_class_init (ClutterPanGestureClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterGestureClass *gesture_class =
      CLUTTER_GESTURE_CLASS (klass);

  gobject_class->set_property = clutter_pan_gesture_set_property;
  gobject_class->get_property = clutter_pan_gesture_get_property;
  gobject_class->finalize = clutter_pan_gesture_finalize;

  gesture_class->points_began = points_began;
  gesture_class->points_moved = points_moved;
  gesture_class->points_ended = points_ended;
  gesture_class->points_cancelled = points_cancelled;
  gesture_class->state_changed = state_changed;

  /**
   * ClutterPanGesture:begin-threshold:
   *
   * The threshold in pixels that has to be panned for the gesture to start.
   */
  obj_props[PROP_BEGIN_THRESHOLD] =
    g_param_spec_uint ("begin-threshold",
                       P_("Begin threshold"),
                       P_("The begin threshold"),
                       0, G_MAXUINT, 0,
                       CLUTTER_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterPanGesture:pan-axis:
   *
   * Constraints the panning action to the specified axis.
   */
  obj_props[PROP_PAN_AXIS] =
    g_param_spec_enum ("pan-axis",
                       P_("Pan Axis"),
                       P_("Constraints the panning to an axis"),
                       CLUTTER_TYPE_PAN_AXIS,
                       CLUTTER_PAN_AXIS_BOTH,
                       CLUTTER_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterPanGesture:min-n-points:
   *
   * The minimum number of points for the gesture to start, defaults to 1.
   */
  obj_props[PROP_MIN_N_POINTS] =
    g_param_spec_uint ("min-n-points",
                       P_("Min n points"),
                       P_("Minimum number of points"),
                       1, G_MAXUINT, 1,
                       CLUTTER_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClutterPanGesture:max-n-points:
   *
   * The maximum number of points to use for the pan. Set to 0 to allow
   * an unlimited number. Defaults to 0.
   */
  obj_props[PROP_MAX_N_POINTS] =
    g_param_spec_uint ("max-n-points",
                       P_("Max n points"),
                       P_("Maximum number of points"),
                       0, G_MAXUINT, 1,
                       CLUTTER_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties  (gobject_class,
                                      PROP_LAST,
                                      obj_props);

  /**
   * ClutterPanGesture::pan-begin:
   * @gesture: the #ClutterPanGesture that emitted the signal
   * @x: the x component of the starting point of the pan
   * @y: the y component of the starting point of the pan
   *
   * The ::pan-begin signal is emitted when a pan gesture has been
   * recognized and is now in progress.
   */
  obj_signals[PAN_BEGIN] =
    g_signal_new (I_("pan-begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  G_TYPE_FLOAT, G_TYPE_FLOAT);

  /**
   * ClutterPanGesture::pan-update:
   * @gesture: the #ClutterPanGesture that emitted the signal
   * @delta_x: the x-axis component of the delta since the
   *   last ::pan-update signal emission
   * @delta_y: the y-axis component of the delta since the
   *   last ::pan-update signal emission
   * @total_distance: the total distance that has been panned
   *
   * The ::pan-update signal is emitted when one or multiple points
   * of the pan have changed.
   */
  obj_signals[PAN_UPDATE] =
    g_signal_new (I_("pan-update"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 3,
                  G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);

  /**
   * ClutterPanGesture::pan-end:
   * @gesture: the #ClutterPanGesture that emitted the signal
   * @velocity_x: the x-axis component of the velocity when the pan
   *   ended, in pixels per millisecond
   * @velocity_y: the y-axis component of the velocity when the pan
   *   ended, in pixels per millisecond
   *
   * The ::pan-end signal is emitted when the pan has ended.
   */
  obj_signals[PAN_END] =
    g_signal_new (I_("pan-end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  G_TYPE_FLOAT, G_TYPE_FLOAT);

  /**
   * ClutterPanGesture::pan-cancel:
   * @gesture: the #ClutterPanGesture that emitted the signal
   *
   * The ::pan-cancel signal is emitted when the pan is cancelled.
   */
  obj_signals[PAN_CANCEL] =
    g_signal_new (I_("pan-cancel"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0,
                  G_TYPE_NONE);
}

static void
clutter_pan_gesture_init (ClutterPanGesture *self)
{
  ClutterPanGesturePrivate *priv =
    clutter_pan_gesture_get_instance_private (self);

  priv->begin_threshold = DEFAULT_BEGIN_THRESHOLD_PX;

  priv->event_history =
    g_array_sized_new (FALSE, TRUE, sizeof (HistoryEntry), EVENT_HISTORY_MAX_LENGTH);
  priv->event_history_begin_index = 0;

  priv->pan_axis = CLUTTER_PAN_AXIS_BOTH;
  priv->min_n_points = 1;
  priv->max_n_points = 0;

  priv->use_point = 0;
}

/**
 * clutter_pan_gesture_new:
 *
 * Creates a new #ClutterPanGesture instance
 *
 * Return value: the newly created #ClutterPanGesture
 *
 * Since: 1.12
 */
ClutterAction *
clutter_pan_gesture_new (void)
{
  return g_object_new (CLUTTER_TYPE_PAN_GESTURE, NULL);
}

/**
 * clutter_pan_gesture_get_begin_threshold:
 * @self: a #ClutterPanGesture
 *
 * Gets the movement threshold in pixels that begins the pan action.
 *
 * Returns: The begin threshold in pixels
 */
unsigned int
clutter_pan_gesture_get_begin_threshold (ClutterPanGesture *self)
{
  ClutterPanGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_PAN_GESTURE (self), 0);

  priv = clutter_pan_gesture_get_instance_private (self);

  return priv->begin_threshold;
}

/**
 * clutter_pan_gesture_set_begin_threshold:
 * @self: a #ClutterClickAction
 * @begin_threshold: the threshold in pixels
 *
 * Sets the movement threshold in pixels to begin the pan action.
 */
void
clutter_pan_gesture_set_begin_threshold (ClutterPanGesture *self,
                                         unsigned int       begin_threshold)
{
  ClutterPanGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_PAN_GESTURE (self));

  priv = clutter_pan_gesture_get_instance_private (self);

  if (priv->begin_threshold == begin_threshold)
    return;

  priv->begin_threshold = begin_threshold;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_BEGIN_THRESHOLD]);

  if (clutter_gesture_get_state (CLUTTER_GESTURE (self)) == CLUTTER_GESTURE_STATE_POSSIBLE)
    {
      unsigned int total_n_points;

      clutter_gesture_get_points (CLUTTER_GESTURE (self), &total_n_points);
      if (total_n_points >= priv->min_n_points &&
          (priv->max_n_points == 0 || total_n_points <= priv->max_n_points))
        {
          if ((priv->pan_axis == CLUTTER_PAN_AXIS_BOTH &&
               graphene_vec2_length (&priv->total_delta) >= priv->begin_threshold) ||
              (priv->pan_axis == CLUTTER_PAN_AXIS_X &&
               ABS (graphene_vec2_get_x (&priv->total_delta)) >= priv->begin_threshold) ||
              (priv->pan_axis == CLUTTER_PAN_AXIS_Y &&
               ABS (graphene_vec2_get_y (&priv->total_delta)) >= priv->begin_threshold))
            clutter_gesture_set_state (CLUTTER_GESTURE (self), CLUTTER_GESTURE_STATE_RECOGNIZING);
        }
    }
}

/**
 * clutter_pan_gesture_set_pan_axis:
 * @self: a #ClutterPanGesture
 * @axis: the #ClutterPanAxis
 *
 * Restricts the panning action to a specific axis.
 */
void
clutter_pan_gesture_set_pan_axis (ClutterPanGesture *self,
                                  ClutterPanAxis     axis)
{
  ClutterPanGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_PAN_GESTURE (self));
  g_return_if_fail (axis != CLUTTER_PAN_AXIS_BOTH ||
                    axis != CLUTTER_PAN_AXIS_X ||
                    axis != CLUTTER_PAN_AXIS_Y);

  priv = clutter_pan_gesture_get_instance_private (self);

  if (priv->pan_axis == axis)
    return;

  priv->pan_axis = axis;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_PAN_AXIS]);
}

/**
 * clutter_pan_gesture_get_pan_axis:
 * @self: a #ClutterPanGesture
 *
 * Retrieves the axis constraint set by clutter_pan_gesture_set_pan_axis().
 *
 * Returns: the axis constraint
 */
ClutterPanAxis
clutter_pan_gesture_get_pan_axis (ClutterPanGesture *self)
{
  ClutterPanGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_PAN_GESTURE (self),
                        CLUTTER_PAN_AXIS_BOTH);

  priv = clutter_pan_gesture_get_instance_private (self);

  return priv->pan_axis;
}

/**
 * clutter_pan_gesture_set_min_n_points:
 * @self: a #ClutterPanGesture
 * @min_n_points: the minimum number of points
 *
 * Sets the minimum number of points for the gesture to start.
 */
void
clutter_pan_gesture_set_min_n_points (ClutterPanGesture *self,
                                      unsigned int       min_n_points)
{
  ClutterPanGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_PAN_GESTURE (self));

  priv = clutter_pan_gesture_get_instance_private (self);

  g_return_if_fail (min_n_points >= 1 &&
                    (priv->max_n_points == 0 || min_n_points <= priv->max_n_points));

  if (priv->min_n_points == min_n_points)
    return;

  priv->min_n_points = min_n_points;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_MIN_N_POINTS]);
}

/**
 * clutter_pan_gesture_get_min_n_points:
 * @self: a #ClutterPanGesture
 *
 * Gets the minimum number of points set by
 * clutter_pan_gesture_set_min_n_points().
 *
 * Returns: the minimum number of points
 */
unsigned int
clutter_pan_gesture_get_min_n_points (ClutterPanGesture *self)
{
  ClutterPanGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_PAN_GESTURE (self), 1);

  priv = clutter_pan_gesture_get_instance_private (self);

  return priv->min_n_points;
}

/**
 * clutter_pan_gesture_set_max_n_points:
 * @self: a #ClutterPanGesture
 * @max_n_points: the maximum number of points
 *
 * Sets the maximum number of points to use for the pan. Set to 0 to allow
 * an unlimited number.
 */
void
clutter_pan_gesture_set_max_n_points (ClutterPanGesture *self,
                                     unsigned int      max_n_points)
{
  ClutterPanGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_PAN_GESTURE (self));

  priv = clutter_pan_gesture_get_instance_private (self);

  g_return_if_fail (max_n_points == 0 || max_n_points >= priv->min_n_points);

  if (priv->max_n_points == max_n_points)
    return;

  priv->max_n_points = max_n_points;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_MAX_N_POINTS]);
}

/**
 * clutter_pan_gesture_get_max_n_points:
 * @self: a #ClutterPanGesture
 *
 * Gets the maximum number of points set by
 * clutter_pan_gesture_set_max_n_points().
 *
 * Returns: the maximum number of points
 */
unsigned int
clutter_pan_gesture_get_max_n_points (ClutterPanGesture *self)
{
  ClutterPanGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_PAN_GESTURE (self), 1);

  priv = clutter_pan_gesture_get_instance_private (self);

  return priv->max_n_points;
}
