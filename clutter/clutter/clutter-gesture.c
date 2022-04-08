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
 * SECTION:clutter-gesture
 * @Title: ClutterGesture
 * @Short_Description: Action for touch and mouse gestures
 *
 * #ClutterGesture is a sub-class of #ClutterAction that implements the
 * logic for recognizing touch and mouse gestures.
 *
 * Implementing a #ClutterGesture is done by subclassing #ClutterGesture,
 * connecting to the points_began()/moved()/ended() and cancelled() vfuncs,
 * and then moving the gesture through the #ClutterGestureState state-machine
 * using clutter_gesture_set_state().
 *
 * ## Recognizing new gestures
 *
 * #ClutterGesture uses five separate states to differentiate between the
 * phases of gesture recognition. Those states also define whether to block or
 * allow event delivery:
 *
 * - WAITING: The gesture will be starting out in this state if no points
 *   are available. When points are added, the state automatically moves
 *   to POSSIBLE before the points_began() vfunc gets called.
 *
 * - POSSIBLE: This is the state the gesture will be in when points_began()
 *   gets called the first time. As soon as the implementation is reasonably
 *   sure that the sequence of events is the gesture, it should set the state
 *   to RECOGNIZING.
 *
 * - RECOGNIZING: A continuous gesture is being recognized. In this state
 *   the implementation usually triggers UI changes as feedback to the user.
 *
 * - COMPLETED: The gesture was sucessfully recognized and has been completed.
 *   The gesture will automatically move to state WAITING after all the
 *   remaining points have ended.
 *
 * - CANCELLED: The gesture was either not started at all because preconditions
 *   were not fulfilled or it was cancelled by the implementation.
 *   The gesture will automatically move to state WAITING after all the
 *   remaining points have ended.
 *
 * Each #ClutterGesture starts out in the WAITING state and automatically
 * moves to POSSIBLE and calls the #ClutterGestureClass.points_began()
 * virtual function when the first point is added. From then on, the
 * implementation will receive points_moved(), points_ended() and
 * points_cancelled() events for all points that have been added. Using these
 * events, the implementation is supposed to move the #ClutterGesture through
 * the #ClutterGestureState state-machine.
 *
 * Note that it's very important that the gesture *always* ends up in either
 * the COMPLETED or the CANCELLED state after points have been added. You
 * should never leave a gesture in the POSSIBLE state.
 *
 * Note that it's not guaranteed that clutter_gesture_set_state() will always
 * (and immediately) enter the requested state. To deal with this, never
 * assume the state has changed after calling set_state(), and react to state
 * changes (for example to emit your own signals) by listening to the
 * state_changed() vfunc.
 *
 * ## Relationships of gestures
 *
 * By default, when multiple gestures try to recognize while sharing one or
 * more points, the first gesture to move to RECOGNIZING wins, and implicitly
 * moves all conflicting gestures to state CANCELLED. This behavior can be
 * prohibited by using the clutter_gesture_can_not_cancel() API or by
 * implementing the should_influence() or should_be_influenced_by() vfuncs
 * in your #ClutterGesture subclass.
 *
 * The relationship between two gestures that are on different actors and
 * don't conflict over any points can also be controlled. By default, globally
 * only a single gesture is allowed to be in the RECOGNIZING state. This
 * default is mostly to avoid UI bugs and complexity that will appear when
 * recognizing multiple gestures at the same time. It's possible to allow
 * starting/recognizing one gesture while another is already in state
 * RECOGNIZING by using the clutter_gesture_recognize_independently_from() API
 * or by implementing the should_start_while() or the other_gesture_may_start()
 * vfuncs in the #ClutterGesture subclass.
 */

#include "clutter-build-config.h"

#include "clutter-gesture.h"

#include "clutter-debug.h"
#include "clutter-enum-types.h"
#include "clutter-marshal.h"
#include "clutter-private.h"
#include "clutter-stage-private.h"

#include <graphene.h>

static const char * state_to_string[] = {
  "WAITING",
  "POSSIBLE",
  "RECOGNIZING",
  "COMPLETED",
  "CANCELLED",
};
G_STATIC_ASSERT (sizeof (state_to_string) / sizeof (state_to_string[0]) == CLUTTER_N_GESTURE_STATES);

typedef struct
{
  ClutterEvent *latest_event;

  ClutterInputDevice *device;
  ClutterInputDevice *source_device;
  ClutterEventSequence *sequence;

  unsigned int n_buttons_pressed;
} GesturePointPrivate;

typedef struct _ClutterGesturePrivate ClutterGesturePrivate;

struct _ClutterGesturePrivate
{
  GArray *points;
  GArray *public_points;
  GPtrArray *emission_points;

  unsigned int point_indices;

  ClutterGestureState state;

  uint64_t allowed_device_types;

  GHashTable *in_relationship_with;

  GPtrArray *cancel_on_recognizing;

  GHashTable *can_not_cancel;
  GHashTable *recognize_independently_from;
};

enum
{
  PROP_0,

  PROP_STATE,

  PROP_LAST
};

enum
{
  MAY_RECOGNIZE,

  LAST_SIGNAL
};

static GParamSpec *obj_props[PROP_LAST];
static unsigned int obj_signals[LAST_SIGNAL] = { 0, };

static GPtrArray *all_active_gestures = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (ClutterGesture, clutter_gesture, CLUTTER_TYPE_ACTION)

#define DEVICE_TYPE_TO_BIT(device_type) (1 << device_type)

static inline void
debug_message (ClutterGesture *self,
               const char     *format,
               ...) __attribute__ ((format (gnu_printf, 2, 3)));

inline void
debug_message (ClutterGesture *self,
               const char     *format,
               ...)
{
  if (G_UNLIKELY (clutter_debug_flags & CLUTTER_DEBUG_GESTURES))
    {
      va_list args;
      char *str;
      const char *name;

      va_start (args, format);

      str = g_strdup_vprintf (format, args);
      name = clutter_actor_meta_get_name (CLUTTER_ACTOR_META (self));

      CLUTTER_NOTE (GESTURES,
                    "<%s> [%p] %s",
                    name ? name : G_OBJECT_TYPE_NAME (self),
                    self, str);

      g_free (str);
      va_end (args);
    }
}

static GesturePointPrivate *
find_point (ClutterGesture        *self,
            ClutterInputDevice    *device,
            ClutterEventSequence  *sequence,
            ClutterGesturePoint  **public_point)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  unsigned int i;

  if (public_point)
    *public_point = NULL;

  for (i = 0; i < priv->points->len; i++)
    {
      GesturePointPrivate *iter = &g_array_index (priv->points, GesturePointPrivate, i);

      if (iter->device == device && iter->sequence == sequence)
        {
          if (public_point && i < priv->public_points->len)
            *public_point = &g_array_index (priv->public_points, ClutterGesturePoint, i);

          return iter;
        }
    }

  return NULL;
}

static GesturePointPrivate *
register_point (ClutterGesture     *self,
                const ClutterEvent *event)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  GesturePointPrivate *point;
  ClutterInputDevice *source_device = clutter_event_get_source_device (event);
  ClutterInputDevice *device = clutter_event_get_device (event);
  ClutterEventSequence *sequence = clutter_event_get_event_sequence (event);

  g_array_set_size (priv->points, priv->points->len + 1);
  point = &g_array_index (priv->points, GesturePointPrivate, priv->points->len - 1);

  point->device = device;
  point->sequence = sequence;
  point->source_device = source_device;
  point->n_buttons_pressed = 0;

  debug_message (self, "Registered new point, n points now: %u",
                 priv->points->len);

  return point;
}

static void
set_state_authoritative (ClutterGesture      *self,
                         ClutterGestureState  new_state);

static void
unregister_point (ClutterGesture       *self,
                  ClutterInputDevice   *device,
                  ClutterEventSequence *sequence)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  int i;

  for (i = 0; i < priv->points->len; i++)
    {
      GesturePointPrivate *iter = &g_array_index (priv->points, GesturePointPrivate, i);

      if (iter->device == device && iter->sequence == sequence)
        {
          g_array_remove_index (priv->points, i);
          if (i < priv->public_points->len)
            g_array_remove_index (priv->public_points, i);

          break;
        }
    }

  if (priv->points->len == 0 &&
      (priv->state == CLUTTER_GESTURE_STATE_COMPLETED ||
       priv->state == CLUTTER_GESTURE_STATE_CANCELLED))
    set_state_authoritative (self, CLUTTER_GESTURE_STATE_WAITING);
}

static void
free_private_point_data (GesturePointPrivate *point)
{
  clutter_event_free (point->latest_event);
}

static inline void
emit_points_vfunc (ClutterGesture *self,
                   void          (*vfunc)(ClutterGesture *, const ClutterGesturePoint **, unsigned int),
                   GPtrArray      *array)
{
  vfunc (self, (const ClutterGesturePoint **) array->pdata, array->len);
}

static void
cancel_points_by_sequences (ClutterGesture       *self,
                            ClutterInputDevice   *device,
                            ClutterEventSequence *sequences,
                            size_t                n_sequences)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  unsigned int i;
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);
  gpointer *array = (gpointer *) sequences;

  if (priv->state == CLUTTER_GESTURE_STATE_CANCELLED ||
      priv->state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      g_assert (priv->public_points->len == 0);

      if (n_sequences == 0)
        {
          unregister_point (self, device, NULL);
        }
      else
        {
          for (i = 0; i < n_sequences; i++)
            unregister_point (self, device, (ClutterEventSequence *) &array[i]);
        }

      return;
    }

  g_assert (priv->emission_points->len == 0);

  if (n_sequences == 0)
    {
      ClutterGesturePoint *public_point;

      find_point (self, device, NULL, &public_point);

      if (public_point)
        g_ptr_array_add (priv->emission_points, public_point);
    }
  else
    {
      for (i = 0; i < n_sequences; i++)
        {
          ClutterEventSequence *sequence = (ClutterEventSequence *) &array[i];
          ClutterGesturePoint *public_point;

          find_point (self, device, sequence, &public_point);

          if (public_point)
            g_ptr_array_add (priv->emission_points, public_point);
        }
    }

  if (priv->emission_points->len == 0)
    return;

  if (gesture_class->points_cancelled)
    emit_points_vfunc (self, gesture_class->points_cancelled, priv->emission_points);

  g_ptr_array_set_size (priv->emission_points, 0);

  if (n_sequences == 0)
    {
      unregister_point (self, device, NULL);
    }
  else
    {
      for (i = 0; i < n_sequences; i++)
        unregister_point (self, device, (ClutterEventSequence *) &array[i]);
    }
}

static void
cancel_all_points (ClutterGesture *self)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  unsigned int i;
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);

  if (priv->state == CLUTTER_GESTURE_STATE_CANCELLED ||
      priv->state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      g_assert (priv->public_points->len == 0);

      g_array_set_size (priv->points, 0);
      set_state_authoritative (self, CLUTTER_GESTURE_STATE_WAITING);
      return;
    }

  g_assert (priv->emission_points->len == 0);

  for (i = 0; i < priv->public_points->len; i++)
    g_ptr_array_add (priv->emission_points, &g_array_index (priv->public_points, ClutterGesturePoint, i));

  if (priv->emission_points->len == 0)
    return;

  if (gesture_class->points_cancelled)
    emit_points_vfunc (self, gesture_class->points_cancelled, priv->emission_points);

  g_ptr_array_set_size (priv->emission_points, 0);

  g_array_set_size (priv->points, 0);
  g_array_set_size (priv->public_points, 0);

  if (priv->state == CLUTTER_GESTURE_STATE_CANCELLED ||
      priv->state == CLUTTER_GESTURE_STATE_COMPLETED)
    set_state_authoritative (self, CLUTTER_GESTURE_STATE_WAITING);
}

static gboolean
other_gesture_allowed_to_start (ClutterGesture *self,
                                ClutterGesture *other_gesture)
{
  ClutterGesturePrivate *other_priv = clutter_gesture_get_instance_private (other_gesture);
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);
  ClutterGestureClass *other_gesture_class = CLUTTER_GESTURE_GET_CLASS (other_gesture);

  if (other_priv->recognize_independently_from &&
      g_hash_table_contains (other_priv->recognize_independently_from, self))
    return TRUE;

  /* Default: Only a single gesture can be recognizing globally at a time */
  gboolean should_start = FALSE;

  if (other_gesture_class->should_start_while)
    other_gesture_class->should_start_while (other_gesture, self, &should_start);

  if (gesture_class->other_gesture_may_start)
    gesture_class->other_gesture_may_start (self, other_gesture, &should_start);

  return should_start;
}

static gboolean
new_gesture_allowed_to_start (ClutterGesture *self)
{
  unsigned int n_active_gestures, i;

  n_active_gestures = all_active_gestures ? all_active_gestures->len : 0;

  for (i = 0; i < n_active_gestures; i++)
    {
      ClutterGesture *existing_gesture = g_ptr_array_index (all_active_gestures, i);
      ClutterGesturePrivate *other_priv =
        clutter_gesture_get_instance_private (existing_gesture);

      if (existing_gesture == self)
        continue;

      /* For gestures in relationship we have different APIs */
      if (g_hash_table_contains (other_priv->in_relationship_with, self))
        continue;

      if (other_priv->state == CLUTTER_GESTURE_STATE_RECOGNIZING)
        {
          if (!other_gesture_allowed_to_start (existing_gesture, self))
            return FALSE;
        }
    }

  return TRUE;
}

static gboolean
gesture_may_start (ClutterGesture *self)
{
  gboolean may_recognize;

  if (!new_gesture_allowed_to_start (self))
    {
      debug_message (self,
                    "gesture may not recognize, another gesture is already running");
      return FALSE;
    }

  g_signal_emit (self, obj_signals[MAY_RECOGNIZE], 0, &may_recognize);
  if (!may_recognize)
    {
      debug_message (self,
                     "::may-recognize prevented gesture from recognizing");
      return FALSE;
    }

  return TRUE;
}

static void
maybe_cancel_independent_gestures (ClutterGesture *self)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  int i;

  g_assert (all_active_gestures != NULL);

  for (i = all_active_gestures->len - 1; i >= 0; i--)
    {
      if (i >= all_active_gestures->len)
        continue;

      ClutterGesture *other_gesture = g_ptr_array_index (all_active_gestures, i);
      ClutterGesturePrivate *other_priv =
        clutter_gesture_get_instance_private (other_gesture);

      if (other_gesture == self)
        continue;

      /* For gestures in relationship we have different APIs */
      if (g_hash_table_contains (priv->in_relationship_with, other_gesture))
        continue;

      if (other_priv->state == CLUTTER_GESTURE_STATE_POSSIBLE &&
          !other_gesture_allowed_to_start (self, other_gesture))
        set_state_authoritative (other_gesture, CLUTTER_GESTURE_STATE_CANCELLED);
    }
}

static void
set_state (ClutterGesture      *self,
           ClutterGestureState  new_state)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  ClutterGestureState old_state;
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);

  if (priv->state == new_state &&
      new_state != CLUTTER_GESTURE_STATE_RECOGNIZING)
    {
      debug_message (self, "Skipping state change %s -> %s",
                     state_to_string[priv->state], state_to_string[new_state]);
      return;
    }

  switch (priv->state)
    {
    case CLUTTER_GESTURE_STATE_WAITING:
      g_assert (new_state == CLUTTER_GESTURE_STATE_POSSIBLE);
      break;
    case CLUTTER_GESTURE_STATE_POSSIBLE:
      g_assert (new_state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
                new_state == CLUTTER_GESTURE_STATE_CANCELLED);
      break;
    case CLUTTER_GESTURE_STATE_RECOGNIZING:
      g_assert (new_state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
                new_state == CLUTTER_GESTURE_STATE_COMPLETED ||
                new_state == CLUTTER_GESTURE_STATE_CANCELLED);
      break;
    case CLUTTER_GESTURE_STATE_COMPLETED:
      g_assert (new_state == CLUTTER_GESTURE_STATE_WAITING);
      break;
    case CLUTTER_GESTURE_STATE_CANCELLED:
      g_assert (new_state == CLUTTER_GESTURE_STATE_WAITING);
      break;
    case CLUTTER_N_GESTURE_STATES:
      g_assert_not_reached ();
      break;
    }

  if (priv->state == CLUTTER_GESTURE_STATE_WAITING)
    {
      if (new_state == CLUTTER_GESTURE_STATE_POSSIBLE)
        {
          if (!gesture_may_start (self))
            {
              /* No vfuncs/signals have been emitted yet, so let's pretend nothing
               * happened and remain in WAITING...
               */
              return;
            }

          if (all_active_gestures == NULL)
            all_active_gestures = g_ptr_array_sized_new (64);

          g_ptr_array_add (all_active_gestures, self);
        }
    }

  if (priv->state == CLUTTER_GESTURE_STATE_POSSIBLE)
    {
      if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZING)
        {
          if (!gesture_may_start (self))
            {
              set_state (self, CLUTTER_GESTURE_STATE_CANCELLED);
              return;
            }
        }
    }

  old_state = priv->state;
  priv->state = new_state;

  if (new_state == CLUTTER_GESTURE_STATE_RECOGNIZING)
    {
      ClutterActor *actor;

      g_assert (priv->points->len == priv->public_points->len);

      actor = clutter_actor_meta_get_actor (CLUTTER_ACTOR_META (self));
      if (actor)
        {
          ClutterStage *stage = CLUTTER_STAGE (clutter_actor_get_stage (actor));

          if (stage)
            {
              unsigned int i;

              for (i = 0; i < priv->points->len; i++)
                {
                  GesturePointPrivate *point = &g_array_index (priv->points, GesturePointPrivate, i);

                  clutter_stage_set_sequence_claimed_by_gesture (stage, point->device, point->sequence);
                }
            }
        }

      maybe_cancel_independent_gestures (self);
    }

  if (new_state == CLUTTER_GESTURE_STATE_CANCELLED ||
      new_state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      g_array_set_size (priv->public_points, 0);
      priv->point_indices = 0;
    }

  if (new_state == CLUTTER_GESTURE_STATE_WAITING)
    {
      GHashTableIter iter;
      ClutterGesture *other_gesture;

      g_assert (g_ptr_array_remove (all_active_gestures, self));

      g_array_set_size (priv->points, 0);

      g_hash_table_iter_init (&iter, priv->in_relationship_with);
      while (g_hash_table_iter_next (&iter, (gpointer *) &other_gesture, NULL))
        {
          ClutterGesturePrivate *other_priv =
            clutter_gesture_get_instance_private (other_gesture);

          g_assert (g_hash_table_remove (other_priv->in_relationship_with, self));

          g_hash_table_iter_remove (&iter);
        }

      g_ptr_array_set_size (priv->cancel_on_recognizing, 0);
    }

  if (gesture_class->state_changed)
    gesture_class->state_changed (self, old_state, new_state);

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_STATE]);

  debug_message (self, "State changed: %s -> %s",
                 state_to_string[old_state], state_to_string[new_state]);
}

static void
maybe_move_to_waiting (ClutterGesture *self)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);

  if (priv->points->len == 0 &&
      (priv->state == CLUTTER_GESTURE_STATE_COMPLETED ||
       priv->state == CLUTTER_GESTURE_STATE_CANCELLED))
    set_state (self, CLUTTER_GESTURE_STATE_WAITING);
}

static void
maybe_influence_other_gestures (ClutterGesture *self)
{
  ClutterGesturePrivate *priv =
    clutter_gesture_get_instance_private (self);

  if (priv->state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
      priv->state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      unsigned int len, i;

      /* Clear the cancel_on_recognizing array now already so that other
       * gestures cancelling us won't clear the array right underneath
       * our feet.
       */
      len = priv->cancel_on_recognizing->len;
      priv->cancel_on_recognizing->len = 0;

      for (i = 0; i < len; i++)
        {
          ClutterGesture *other_gesture = priv->cancel_on_recognizing->pdata[i];

          if (!g_hash_table_contains (priv->in_relationship_with, other_gesture))
            continue;

          set_state (other_gesture, CLUTTER_GESTURE_STATE_CANCELLED);
          maybe_move_to_waiting (other_gesture);
        }
    }
}

void
set_state_authoritative (ClutterGesture      *self,
                         ClutterGestureState  new_state)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);

  /* Moving to COMPLETED always goes through RECOGNIZING */
  if (priv->state != CLUTTER_GESTURE_STATE_RECOGNIZING &&
      new_state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      set_state (self, CLUTTER_GESTURE_STATE_RECOGNIZING);

      if (priv->state == CLUTTER_GESTURE_STATE_RECOGNIZING)
        set_state (self, CLUTTER_GESTURE_STATE_COMPLETED);
      else
        g_assert (priv->state == CLUTTER_GESTURE_STATE_CANCELLED);

      maybe_influence_other_gestures (self);
      maybe_move_to_waiting (self);
      return;
    }

  set_state (self, new_state);
  if (priv->state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
      priv->state == CLUTTER_GESTURE_STATE_CANCELLED)
    maybe_influence_other_gestures (self);
  maybe_move_to_waiting (self);
}

static void
update_point_from_event (GesturePointPrivate *point,
                         ClutterGesturePoint *public_point,
                         const ClutterEvent  *event)
{
  float x, y;

  if (point->latest_event)
    clutter_event_free ((ClutterEvent *) point->latest_event);
  point->latest_event = clutter_event_copy (event);

  public_point->latest_event = point->latest_event;
  public_point->event_time = clutter_event_get_time (event);

  clutter_event_get_coords (event, &x, &y);

  if (clutter_event_type (event) == CLUTTER_BUTTON_PRESS ||
      clutter_event_type (event) == CLUTTER_TOUCH_BEGIN)
    {
      public_point->begin_coords.x = x;
      public_point->begin_coords.y = y;
    }
  else if (clutter_event_type (event) == CLUTTER_MOTION ||
           clutter_event_type (event) == CLUTTER_TOUCH_UPDATE)
    {
      public_point->move_coords.x = x;
      public_point->move_coords.y = y;
    }
  else
    {
      public_point->end_coords.x = x;
      public_point->end_coords.y = y;
    }

  public_point->last_coords.x = public_point->latest_coords.x;
  public_point->last_coords.y = public_point->latest_coords.y;

  public_point->latest_coords.x = x;
  public_point->latest_coords.y = y;
}

static gboolean
clutter_gesture_real_may_recognize (ClutterGesture *self)
{
  return TRUE;
}

static gboolean
clutter_gesture_handle_event (ClutterAction      *action,
                              const ClutterEvent *event)
{
  ClutterGesture *self = CLUTTER_GESTURE (action);
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);
  ClutterEventType event_type;
  ClutterInputDevice *device = clutter_event_get_device (event);
  ClutterEventSequence *sequence = clutter_event_get_event_sequence (event);
  GesturePointPrivate *point;
  ClutterGesturePoint *public_point;

  if (clutter_event_get_flags (event) & CLUTTER_EVENT_FLAG_SYNTHETIC)
    return CLUTTER_EVENT_PROPAGATE;

  event_type = clutter_event_type (event);

  if (event_type != CLUTTER_BUTTON_PRESS &&
      event_type != CLUTTER_MOTION &&
      event_type != CLUTTER_BUTTON_RELEASE &&
      event_type != CLUTTER_TOUCH_BEGIN &&
      event_type != CLUTTER_TOUCH_UPDATE &&
      event_type != CLUTTER_TOUCH_END &&
      event_type != CLUTTER_TOUCH_CANCEL &&
      event_type != CLUTTER_ENTER &&
      event_type != CLUTTER_LEAVE)
    return CLUTTER_EVENT_PROPAGATE;

  if ((point = find_point (self, device, sequence, &public_point)) == NULL)
    return CLUTTER_EVENT_PROPAGATE;

  g_assert (priv->state != CLUTTER_GESTURE_STATE_WAITING);

  if (event_type == CLUTTER_BUTTON_PRESS)
    {
      point->n_buttons_pressed++;
      if (point->n_buttons_pressed >= 2)
        return CLUTTER_EVENT_PROPAGATE;
    }
  else if (event_type == CLUTTER_BUTTON_RELEASE)
    {
      point->n_buttons_pressed--;
      if (point->n_buttons_pressed >= 1)
        return CLUTTER_EVENT_PROPAGATE;
    }

  if (priv->state == CLUTTER_GESTURE_STATE_CANCELLED ||
      priv->state == CLUTTER_GESTURE_STATE_COMPLETED)
    {
      g_assert (priv->public_points->len == 0);

      if (event_type == CLUTTER_BUTTON_RELEASE ||
          event_type == CLUTTER_TOUCH_END ||
          event_type == CLUTTER_TOUCH_CANCEL)
        unregister_point (self, device, sequence);

      return CLUTTER_EVENT_PROPAGATE;
    }

  switch (event_type)
    {
    case CLUTTER_BUTTON_PRESS:
    case CLUTTER_TOUCH_BEGIN:
      g_assert (public_point == NULL);

      g_array_set_size (priv->public_points, priv->public_points->len + 1);
      public_point = &g_array_index (priv->public_points, ClutterGesturePoint, priv->public_points->len - 1);

      public_point->index = priv->point_indices;
      priv->point_indices++;

      update_point_from_event (point, public_point, event);

      g_ptr_array_add (priv->emission_points, public_point);
      if (gesture_class->points_began)
        emit_points_vfunc (self, gesture_class->points_began, priv->emission_points);

      g_ptr_array_set_size (priv->emission_points, 0);
      break;

    case CLUTTER_MOTION:
    case CLUTTER_TOUCH_UPDATE:
      g_assert (public_point != NULL);

      update_point_from_event (point, public_point, event);

      g_ptr_array_add (priv->emission_points, public_point);
      if (gesture_class->points_moved)
        emit_points_vfunc (self, gesture_class->points_moved, priv->emission_points);

      g_ptr_array_set_size (priv->emission_points, 0);
      break;

    case CLUTTER_BUTTON_RELEASE:
    case CLUTTER_TOUCH_END:
      g_assert (public_point != NULL);

      update_point_from_event (point, public_point, event);

      g_ptr_array_add (priv->emission_points, public_point);
      if (gesture_class->points_ended)
        emit_points_vfunc (self, gesture_class->points_ended, priv->emission_points);

      g_ptr_array_set_size (priv->emission_points, 0);
      unregister_point (self, device, sequence);
      break;

    case CLUTTER_TOUCH_CANCEL:
      g_assert (public_point != NULL);

      g_ptr_array_add (priv->emission_points, public_point);
      if (gesture_class->points_cancelled)
        emit_points_vfunc (self, gesture_class->points_cancelled, priv->emission_points);

      g_ptr_array_set_size (priv->emission_points, 0);
      unregister_point (self, device, sequence);
      break;

    case CLUTTER_ENTER:
    case CLUTTER_LEAVE:
      if (public_point && gesture_class->crossing_event)
        {
          ClutterCrossingEvent *crossing = (ClutterCrossingEvent *) event;

          gesture_class->crossing_event (self,
                                         public_point,
                                         event_type,
                                         clutter_event_get_time (event),
                                         clutter_event_get_flags (event),
                                         crossing->source,
                                         crossing->related);
        }
      break;

    default:
      break;
    }

  return CLUTTER_EVENT_PROPAGATE;
}

static void
clutter_gesture_sequences_cancelled (ClutterAction        *action,
                                     ClutterInputDevice   *device,
                                     ClutterEventSequence *sequences,
                                     size_t                n_sequences)
{
  cancel_points_by_sequences (CLUTTER_GESTURE (action), device, sequences, n_sequences);
}

static gboolean
clutter_gesture_should_handle_sequence (ClutterAction      *action,
                                        const ClutterEvent *event)
{
  ClutterGesture *self = CLUTTER_GESTURE (action);
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  ClutterInputDevice *source_device = clutter_event_get_source_device (event);

  if (priv->state == CLUTTER_GESTURE_STATE_CANCELLED)
    return FALSE;

  if (priv->points->len > 0)
    {
      /* Only allow new points coming from the same input device */
      if (g_array_index (priv->points, GesturePointPrivate, 0).source_device != source_device)
        return FALSE;
    }
  else
    {
      ClutterInputDeviceType device_type =
        clutter_input_device_get_device_type (source_device);

      if ((priv->allowed_device_types & DEVICE_TYPE_TO_BIT (device_type)) == 0)
        return FALSE;

      if (priv->state == CLUTTER_GESTURE_STATE_WAITING)
        {
          set_state_authoritative (self, CLUTTER_GESTURE_STATE_POSSIBLE);
          if (priv->state != CLUTTER_GESTURE_STATE_POSSIBLE)
            return FALSE;
        }
    }

  register_point (self, event);

  return TRUE;
}

static void
setup_influence_on_other_gesture (ClutterGesture *self,
                                  ClutterGesture *other_gesture,
                                  gboolean       *cancel_other_gesture_on_recognizing)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);
  ClutterGestureClass *gesture_class = CLUTTER_GESTURE_GET_CLASS (self);
  ClutterGestureClass *other_gesture_class = CLUTTER_GESTURE_GET_CLASS (other_gesture);

  /* The default: We cancel other gestures when we recognize */
  gboolean cancel = TRUE;

  /* First check with the implementation specific APIs */
  if (gesture_class->should_influence)
    gesture_class->should_influence (self, other_gesture, &cancel);

  if (other_gesture_class->should_be_influenced_by)
    other_gesture_class->should_be_influenced_by (other_gesture, self, &cancel);

  /* Then apply overrides made using the public methods */
  if (priv->can_not_cancel &&
      g_hash_table_contains (priv->can_not_cancel, other_gesture))
    cancel = FALSE;

  *cancel_other_gesture_on_recognizing = cancel;
}

static int
clutter_gesture_setup_sequence_relationship (ClutterAction        *action_1,
                                             ClutterAction        *action_2,
                                             ClutterInputDevice   *device,
                                             ClutterEventSequence *sequence)
{
  if (!CLUTTER_IS_GESTURE (action_1) || !CLUTTER_IS_GESTURE (action_2))
    return 0;

  ClutterGesture *gesture_1 = CLUTTER_GESTURE (action_1);
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (action_2);
  ClutterGesturePrivate *priv_1 = clutter_gesture_get_instance_private (gesture_1);
  ClutterGesturePrivate *priv_2 = clutter_gesture_get_instance_private (gesture_2);
  gboolean cancel_1_on_recognizing;
  gboolean cancel_2_on_recognizing;

  g_assert (find_point (gesture_1, device, sequence, NULL) != NULL &&
            find_point (gesture_2, device, sequence, NULL) != NULL);

  /* If gesture 1 knows gesture 2 (this implies vice-versa), everything's
   * figured out already, we won't negotiate again for any new shared sequences!
   */
  if (g_hash_table_contains (priv_1->in_relationship_with, gesture_2))
    {
      cancel_1_on_recognizing = g_ptr_array_find (priv_2->cancel_on_recognizing, gesture_1, NULL);
      cancel_2_on_recognizing = g_ptr_array_find (priv_1->cancel_on_recognizing, gesture_2, NULL);
    }
  else
    {
      setup_influence_on_other_gesture (gesture_1, gesture_2,
                                        &cancel_2_on_recognizing);

      setup_influence_on_other_gesture (gesture_2, gesture_1,
                                        &cancel_1_on_recognizing);

      CLUTTER_NOTE (GESTURES,
                    "Setting up relation between \"<%s> [<%s>:%p]\" (cancel: %d) "
                    "and \"<%s> [<%s>:%p]\" (cancel: %d)",
                    clutter_actor_meta_get_name (CLUTTER_ACTOR_META (gesture_1)),
                    G_OBJECT_TYPE_NAME (gesture_1), gesture_1,
                    cancel_1_on_recognizing,
                    clutter_actor_meta_get_name (CLUTTER_ACTOR_META (gesture_2)),
                    G_OBJECT_TYPE_NAME (gesture_2), gesture_2,
                    cancel_2_on_recognizing);

      g_hash_table_add (priv_1->in_relationship_with, g_object_ref (gesture_2));
      g_hash_table_add (priv_2->in_relationship_with, g_object_ref (gesture_1));

      if (cancel_2_on_recognizing)
        g_ptr_array_add (priv_1->cancel_on_recognizing, gesture_2);

      if (cancel_1_on_recognizing)
        g_ptr_array_add (priv_2->cancel_on_recognizing, gesture_1);
    }

  if (cancel_2_on_recognizing && !cancel_1_on_recognizing)
    return -1;

  if (!cancel_2_on_recognizing && cancel_1_on_recognizing)
    return 1;

  return 0;
}

static void
clutter_gesture_set_actor (ClutterActorMeta *meta,
                           ClutterActor     *actor)
{
  ClutterGesture *self = CLUTTER_GESTURE (meta);
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);

  if (priv->public_points->len)
    {
      debug_message (self,
                     "Detaching from actor while gesture has points, cancelling "
                     "%u points",
                     priv->public_points->len);

      cancel_all_points (self);
    }

  CLUTTER_ACTOR_META_CLASS (clutter_gesture_parent_class)->set_actor (meta, actor);
}

static void
clutter_gesture_set_property (GObject      *gobject,
                              unsigned int  prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_gesture_get_property (GObject      *gobject,
                              unsigned int  prop_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  ClutterGesture *self = CLUTTER_GESTURE (gobject);

  switch (prop_id)
    {
    case PROP_STATE:
      g_value_set_enum (value, clutter_gesture_get_state (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
other_gesture_disposed (gpointer user_data,
                        GObject    *finalized_gesture)
{
  GHashTable *hashtable = user_data;

  g_hash_table_remove (hashtable, finalized_gesture);
}

static void
destroy_weak_ref_hashtable (GHashTable *hashtable)
{
  GHashTableIter iter;
  GObject *key;

  g_hash_table_iter_init (&iter, hashtable);
  while (g_hash_table_iter_next (&iter, (gpointer *) &key, NULL))
    g_object_weak_unref (key, other_gesture_disposed, hashtable);

  g_hash_table_destroy (hashtable);
}

static void
clutter_gesture_finalize (GObject *gobject)
{
  ClutterGesture *self = CLUTTER_GESTURE (gobject);
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);

  if (priv->state != CLUTTER_GESTURE_STATE_WAITING)
    g_assert (g_ptr_array_remove (all_active_gestures, self));

  g_array_unref (priv->points);
  g_array_unref (priv->public_points);

  g_assert (g_hash_table_size (priv->in_relationship_with) == 0);
  g_hash_table_destroy (priv->in_relationship_with);

  g_assert (priv->cancel_on_recognizing->len == 0);
  g_ptr_array_free (priv->cancel_on_recognizing, TRUE);

  if (priv->can_not_cancel)
    destroy_weak_ref_hashtable (priv->can_not_cancel);
  if (priv->recognize_independently_from)
    destroy_weak_ref_hashtable (priv->recognize_independently_from);

  G_OBJECT_CLASS (clutter_gesture_parent_class)->finalize (gobject);
}

static void
clutter_gesture_class_init (ClutterGestureClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorMetaClass *meta_class = CLUTTER_ACTOR_META_CLASS (klass);
  ClutterActionClass *action_class = CLUTTER_ACTION_CLASS (klass);

  klass->may_recognize = clutter_gesture_real_may_recognize;

  action_class->handle_event = clutter_gesture_handle_event;
  action_class->sequences_cancelled = clutter_gesture_sequences_cancelled;
  action_class->should_handle_sequence = clutter_gesture_should_handle_sequence;
  action_class->setup_sequence_relationship = clutter_gesture_setup_sequence_relationship;

  meta_class->set_actor = clutter_gesture_set_actor;

  gobject_class->set_property = clutter_gesture_set_property;
  gobject_class->get_property = clutter_gesture_get_property;
  gobject_class->finalize = clutter_gesture_finalize;

  /**
   * ClutterGesture:state:
   *
   * The current state of the gesture.
   */
  obj_props[PROP_STATE] =
    g_param_spec_enum ("state",
                      P_("State"),
                      P_("The current state of the gesture"),
                      CLUTTER_TYPE_GESTURE_STATE,
                      CLUTTER_GESTURE_STATE_WAITING,
                      CLUTTER_PARAM_READABLE |
                      G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class,
                                     PROP_LAST,
                                     obj_props);

  /**
   * ClutterGesture::may-recognize:
   * @gesture: the #ClutterGesture that emitted the signal
   *
   * The ::may-recognize signal is emitted if the gesture might become
   * active and move to RECOGNIZING. Its purpose is to allow the
   * implementation or a user of a gesture to prohibit the gesture
   * from starting when needed.
   *
   * Returns: %TRUE if the gesture may recognize, %FALSE if it may not.
   */
  obj_signals[MAY_RECOGNIZE] =
    g_signal_new (I_("may-recognize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterGestureClass, may_recognize),
                  g_signal_accumulator_first_wins,
                  NULL, NULL,
                  G_TYPE_BOOLEAN, 0,
                  G_TYPE_NONE);
}

static void
clutter_gesture_init (ClutterGesture *self)
{
  ClutterGesturePrivate *priv = clutter_gesture_get_instance_private (self);

  priv->points = g_array_sized_new (FALSE, TRUE, sizeof (GesturePointPrivate), 3);
  g_array_set_clear_func (priv->points, (GDestroyNotify) free_private_point_data);
  priv->public_points = g_array_sized_new (FALSE, TRUE, sizeof (ClutterGesturePoint), 3);
  priv->emission_points = g_ptr_array_sized_new (3);

  priv->point_indices = 0;

  priv->state = CLUTTER_GESTURE_STATE_WAITING;

  priv->allowed_device_types =
    DEVICE_TYPE_TO_BIT (CLUTTER_POINTER_DEVICE) |
    DEVICE_TYPE_TO_BIT (CLUTTER_TOUCHPAD_DEVICE) |
    DEVICE_TYPE_TO_BIT (CLUTTER_TOUCHSCREEN_DEVICE) |
    DEVICE_TYPE_TO_BIT (CLUTTER_TABLET_DEVICE);

  priv->in_relationship_with = g_hash_table_new_full (NULL, NULL, (GDestroyNotify) g_object_unref, NULL);

  priv->cancel_on_recognizing = g_ptr_array_new ();

  priv->can_not_cancel = NULL;
  priv->recognize_independently_from = NULL;
}

/**
 * clutter_gesture_new:
 *
 * Creates a new #ClutterGesture instance.
 *
 * Returns: the newly created #ClutterGesture
 */
ClutterAction *
clutter_gesture_new (void)
{
  return g_object_new (CLUTTER_TYPE_GESTURE, NULL);
}

/**
 * clutter_gesture_set_state:
 * @self: a #ClutterGesture
 * @state: the new #ClutterGestureState
 *
 * Sets the state of the gesture.
 */
void
clutter_gesture_set_state (ClutterGesture      *self,
                           ClutterGestureState  state)
{
  ClutterGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_GESTURE (self));

  priv = clutter_gesture_get_instance_private (self);

  debug_message (self, "State change requested: %s -> %s",
                 state_to_string[priv->state], state_to_string[state]);

  if ((priv->state == CLUTTER_GESTURE_STATE_WAITING &&
       (state == CLUTTER_GESTURE_STATE_POSSIBLE)) ||
      (priv->state == CLUTTER_GESTURE_STATE_POSSIBLE &&
       (state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
        state == CLUTTER_GESTURE_STATE_COMPLETED ||
        state == CLUTTER_GESTURE_STATE_CANCELLED)) ||
      (priv->state == CLUTTER_GESTURE_STATE_RECOGNIZING &&
       (state == CLUTTER_GESTURE_STATE_RECOGNIZING ||
        state == CLUTTER_GESTURE_STATE_COMPLETED ||
        state == CLUTTER_GESTURE_STATE_CANCELLED)) ||
      (priv->state == CLUTTER_GESTURE_STATE_COMPLETED &&
       (state == CLUTTER_GESTURE_STATE_WAITING)) ||
      (priv->state == CLUTTER_GESTURE_STATE_CANCELLED &&
       (state == CLUTTER_GESTURE_STATE_WAITING)))
    {
      set_state_authoritative (self, state);
    }
  else
    {
      /* For sake of simplicity, never complain about unnecessary tries to cancel */
      if (state == CLUTTER_GESTURE_STATE_CANCELLED)
        return;

      g_warning ("gesture <%s> [<%s>:%p]: Requested invalid state change: %s -> %s",
                 clutter_actor_meta_get_name (CLUTTER_ACTOR_META (self)),
                 G_OBJECT_TYPE_NAME (self), self,
                 state_to_string[priv->state], state_to_string[state]);
    }
}

/**
 * clutter_gesture_get_state:
 * @self: a #ClutterGesture
 *
 * Gets the current state of the gesture.
 *
 * Returns: the #ClutterGestureState
 */
ClutterGestureState
clutter_gesture_get_state (ClutterGesture *self)
{
  ClutterGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_GESTURE (self), CLUTTER_GESTURE_STATE_WAITING);

  priv = clutter_gesture_get_instance_private (self);

  return priv->state;
}

/**
 * clutter_gesture_get_points:
 * @self: a #ClutterGesture
 * @n_points: (out) (optional): the number of points in the returned array
 *
 * Retrieves an array of the points the gesture is using, the array is
 * ordered in the order the points were added in (newest to oldest).
 *
 * Returns: (array length=n_points): the points of the gesture
 */
const ClutterGesturePoint *
clutter_gesture_get_points (ClutterGesture *self,
                            unsigned int   *n_points)
{
  ClutterGesturePrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_GESTURE (self), NULL);

  priv = clutter_gesture_get_instance_private (self);

  if (n_points)
    *n_points = priv->public_points->len;

  if (priv->public_points->len == 0)
    return NULL;

  return (const ClutterGesturePoint *) priv->public_points->data;
}

/**
 * clutter_gesture_set_allowed_device_types:
 * @self: a #ClutterGesture
 * @allowed_device_types: (array length=n_device_types): the allowed device types
 * @n_device_types: the number of elements in @allowed_device_types
 *
 * Sets the types of input devices that are allowed to add new points to
 * the gesture.
 */
void
clutter_gesture_set_allowed_device_types (ClutterGesture         *self,
                                          ClutterInputDeviceType *allowed_device_types,
                                          size_t                  n_device_types)
{
  ClutterGesturePrivate *priv;
  unsigned int i;

  g_return_if_fail (CLUTTER_IS_GESTURE (self));

  priv = clutter_gesture_get_instance_private (self);

  priv->allowed_device_types = 0;

  for (i = 0; i < n_device_types; i++)
    {
      ClutterInputDeviceType device_type = allowed_device_types[i];

      if (device_type >= CLUTTER_N_DEVICE_TYPES)
        {
          g_warning ("Invalid device type passed to set_allowed_device_types()");
          continue;
        }

      priv->allowed_device_types |= DEVICE_TYPE_TO_BIT (device_type);
    }
}

/**
 * clutter_gesture_can_not_cancel:
 * @self: a #ClutterGesture
 * @other_gesture: the other #ClutterGesture
 *
 * In case @self and @other_gesture are operating on the same points, calling
 * this function will make sure that @self does not cancel @other_gesture
 * when @self moves to state RECOGNIZING.
 *
 * To allow two gestures to recognize simultaneously using the same set of
 * points (for example a zoom and a rotate gesture on the same actor), call
 * clutter_gesture_can_not_cancel() twice, so that both gestures can not
 * cancel each other.
 */
void
clutter_gesture_can_not_cancel (ClutterGesture *self,
                                ClutterGesture *other_gesture)
{
  ClutterGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_GESTURE (self));
  g_return_if_fail (CLUTTER_IS_GESTURE (other_gesture));

  priv = clutter_gesture_get_instance_private (self);

  if (!priv->can_not_cancel)
    priv->can_not_cancel = g_hash_table_new (NULL, NULL);

  if (!g_hash_table_add (priv->can_not_cancel, other_gesture))
    return;

  g_object_weak_ref (G_OBJECT (other_gesture),
                     (GWeakNotify) other_gesture_disposed,
                     priv->can_not_cancel);
}

/**
 * clutter_gesture_recognize_independently_from:
 * @self: a #ClutterGesture
 * @other_gesture: the other #ClutterGesture
 *
 * In case @self and @other_gesture are operating on a different set of points,
 * calling this function will allow @self to start while @other_gesture is
 * already in state RECOGNIZING.
 */
void
clutter_gesture_recognize_independently_from (ClutterGesture *self,
                                              ClutterGesture *other_gesture)
{
  ClutterGesturePrivate *priv;

  g_return_if_fail (CLUTTER_IS_GESTURE (self));
  g_return_if_fail (CLUTTER_IS_GESTURE (other_gesture));

  priv = clutter_gesture_get_instance_private (self);

  if (!priv->recognize_independently_from)
    priv->recognize_independently_from = g_hash_table_new (NULL, NULL);

  if (!g_hash_table_add (priv->recognize_independently_from, other_gesture))
    return;

  g_object_weak_ref (G_OBJECT (other_gesture),
                     (GWeakNotify) other_gesture_disposed,
                     priv->recognize_independently_from);
}
