#define CLUTTER_DISABLE_DEPRECATION_WARNINGS
#include <clutter/clutter.h>

#include "tests/clutter-test-utils.h"

static void
gesture_changed_state_once (ClutterGesture      *gesture,
                            GParamSpec          *spec,
                            ClutterGestureState *state_ptr)
{
  *state_ptr = clutter_gesture_get_state (gesture);

  g_signal_handlers_disconnect_by_func (gesture, gesture_changed_state_once, state_ptr);
}

static void
on_presented (ClutterStage     *stage,
              ClutterStageView *view,
              ClutterFrameInfo *frame_info,
              gboolean         *was_presented)
{
  *was_presented = TRUE;
}

static void
emit_event_and_wait (ClutterStage     *stage,
                     gboolean         *was_presented,
                     ClutterEventType  type,
                     float             x,
                     float             y)
{
  ClutterSeat *seat =
    clutter_backend_get_default_seat (clutter_get_default_backend ());
  ClutterInputDevice *pointer = clutter_seat_get_pointer (seat);
  ClutterEvent *event = clutter_event_new (type);

  *was_presented = FALSE;

  clutter_event_set_coords (event, x, y);
  clutter_event_set_device (event, pointer);
  clutter_event_set_stage (event, stage);

  clutter_event_put (event);
  clutter_event_free (event);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (stage));

  while (!*was_presented)
    g_main_context_iteration (NULL, FALSE);
}

#if 0
static void
emit_touch_event_and_wait (ClutterStage     *stage,
                           gboolean         *was_presented,
                           ClutterEventType  type,
                           int               touch_sequence,
                           float             x,
                           float             y)
{
  ClutterSeat *seat =
    clutter_backend_get_default_seat (clutter_get_default_backend ());
  ClutterInputDevice *pointer = clutter_seat_get_pointer (seat);
  ClutterEvent *event = clutter_event_new (type);
  ClutterEventSequence *sequence = GINT_TO_POINTER (touch_sequence);

  *was_presented = FALSE;

  clutter_event_set_coords (event, x, y);
  clutter_event_set_device (event, pointer);
  clutter_event_set_source_device (event, pointer);
  clutter_event_set_stage (event, stage);
  event->touch.sequence = sequence;

  clutter_event_put (event);
  clutter_event_free (event);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (stage));

  while (!*was_presented)
    g_main_context_iteration (NULL, FALSE);
}
#endif

static void
emit_touch_event_and_wait (ClutterStage     *stage,
                           gboolean         *was_presented,
                           ClutterEventType  type,
                           unsigned int      slot,
                           float             x,
                           float             y)
{
  ClutterSeat *seat =
    clutter_backend_get_default_seat (clutter_get_default_backend ());
  ClutterInputDevice *pointer = clutter_seat_get_pointer (seat);
  ClutterEvent *event = clutter_event_new (type);

  *was_presented = FALSE;

  clutter_event_set_coords (event, x, y);
  clutter_event_set_device (event, pointer);
  clutter_event_set_stage (event, stage);

  event->touch.sequence = GINT_TO_POINTER (slot + 1);

  /* Set the event as synthetic so that we don't try to accept/reject it with x11 */
  event->any.flags |= CLUTTER_EVENT_FLAG_SYNTHETIC;

  clutter_event_put (event);
  clutter_event_free (event);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (stage));

  while (!*was_presented)
    g_main_context_iteration (NULL, FALSE);
}

static void
gesture_relationship_freed_despite_relationship (void)
{
  ClutterAction *action_1 = clutter_gesture_new ();
  ClutterAction *action_2 = clutter_gesture_new ();

  g_object_add_weak_pointer (G_OBJECT (action_1), (gpointer *) &action_1);
  g_object_add_weak_pointer (G_OBJECT (action_2), (gpointer *) &action_2);

  clutter_gesture_can_not_cancel (CLUTTER_GESTURE (action_1),
                                  CLUTTER_GESTURE (action_2));

  g_object_unref (action_2);
  g_assert_null (action_2);

  g_object_unref (action_1);
  g_assert_null (action_1);
}

static void
gesture_relationship_simple (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_simple_2 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_COMPLETED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_two_points (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 0, 15, 15);
  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 1, 15, 20);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 1, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 0, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}


static void
gesture_relationship_two_points_two_actors (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterActor *second_actor = clutter_actor_new ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  clutter_actor_set_size (second_actor, 20, 20);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_2));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  /* Need to wait for one presentation so that picking with second actor works */
  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 0, 15, 15);
  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 1, 15, 50);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 0, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 0, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_COMPLETED);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 0, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 1, 15, 50);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_destroy (second_actor);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_global_inhibit_move_to_possible (void)
{
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);

  g_object_unref (gesture_1);
  g_object_unref (gesture_2);
}

static void
gesture_relationship_global_cancel_on_recognize (void)
{
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  ClutterGestureState gesture_2_state_change;

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  g_signal_connect (gesture_2, "notify::state",
                    G_CALLBACK (gesture_changed_state_once),
                    &gesture_2_state_change);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (gesture_2_state_change == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);

  g_object_unref (gesture_1);
  g_object_unref (gesture_2);
}

static void
gesture_relationship_global_recognize_independently (void)
{
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));

  clutter_gesture_recognize_independently_from (gesture_2, gesture_1);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_COMPLETED);

  g_object_unref (gesture_1);
  g_object_unref (gesture_2);
}

static void
gesture_relationship_global_recognize_independently_2 (void)
{
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));

  clutter_gesture_recognize_independently_from (gesture_1, gesture_2);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);

  g_object_unref (gesture_1);
  g_object_unref (gesture_2);
}

static void
gesture_relationship_change (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_can_not_cancel (gesture_1, gesture_2);
  clutter_gesture_relationships_changed (gesture_2);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_COMPLETED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

#if 0
static void
gesture_relationship_change_2 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterActor *second_actor = clutter_actor_new ();

  clutter_actor_set_size (second_actor, 20, 20);
  clutter_actor_set_x (second_actor, 15);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;
  unsigned int n_points;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_2));
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 1, 10, 10);

  clutter_gesture_get_points (gesture_1, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_BEGIN, 2, 17, 10);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_get_points (gesture_1, &n_points);
  g_assert_cmpint (n_points, ==, 2);
  clutter_gesture_get_points (gesture_2, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_CANCELLED);
  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 2, 17, 10);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  emit_touch_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_TOUCH_END, 1, 0, 0);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_destroy (second_actor);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}
#endif

static void
gesture_relationship_failure_requirement_1 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_COMPLETED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_failure_requirement_2 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_failure_requirement_3 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_failure_requirement_4 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  gboolean was_presented;
  ClutterGestureState gesture_1_state;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);


  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);

  g_signal_connect (gesture_1, "notify::state",
                    G_CALLBACK (gesture_changed_state_once),
                    &gesture_1_state);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);

  /* Should go into RECOGNIZING first, then into COMPLETED, then WAITING */
  g_assert_true (gesture_1_state == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_influencing_cascade (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  ClutterGesture *gesture_3 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-3", NULL));
  ClutterGesture *gesture_4 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-4", NULL));
  gboolean was_presented = FALSE;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_3));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_4));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);
  clutter_gesture_can_not_cancel (gesture_1, gesture_4);
  clutter_gesture_require_failure_of (gesture_4, gesture_3);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  clutter_gesture_set_state (gesture_4, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  clutter_gesture_set_state (gesture_4, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_3));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_4));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_influencing_cascade_2 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  ClutterGesture *gesture_3 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-3", NULL));
  ClutterGesture *gesture_4 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-4", NULL));
  gboolean was_presented = FALSE;
  ClutterGestureState gesture_1_state, gesture_4_state;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_3));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_4));

  clutter_gesture_require_failure_of (gesture_1, gesture_2);
  clutter_gesture_can_not_cancel (gesture_1, gesture_4);
  clutter_gesture_can_not_cancel (gesture_4, gesture_1);
  clutter_gesture_require_failure_of (gesture_4, gesture_3);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  clutter_gesture_set_state (gesture_4, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  g_signal_connect (gesture_1, "notify::state",
                    G_CALLBACK (gesture_changed_state_once),
                    &gesture_1_state);
  g_signal_connect (gesture_4, "notify::state",
                    G_CALLBACK (gesture_changed_state_once),
                    &gesture_4_state);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  clutter_gesture_set_state (gesture_4, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (gesture_1_state == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (gesture_4_state == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_4) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_3));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_4));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_influencing_execution_order (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  ClutterGesture *gesture_3 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-3", NULL));
  gboolean was_presented = FALSE;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_3));

  /* gesture_1 cancels gesture_3, but gesture_1 recognizing recursively
   * triggers gesture_3 to recognize via gesture_2.
   * gesture_3 should be cancelled before that happens.
   */
  clutter_gesture_require_failure_of (gesture_3, gesture_2);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

  clutter_gesture_set_state (gesture_3, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_CANCELLED);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_3));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_relationship_event_order (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-1", NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-2", NULL));
  ClutterGesture *gesture_3 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_GESTURE, "name", "gesture-3", NULL));
  gboolean was_presented = FALSE;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  ClutterActor *second_actor = clutter_actor_new ();

  clutter_actor_set_size (second_actor, 200, 200);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  clutter_actor_show (stage);

  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_3));

  clutter_gesture_require_failure_of (gesture_2, gesture_1);
  clutter_gesture_require_failure_of (gesture_3, gesture_1);
  clutter_gesture_can_not_cancel (gesture_1, gesture_2);
  clutter_gesture_can_not_cancel (gesture_1, gesture_3);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_RECOGNIZING);
  clutter_gesture_set_state (gesture_3, CLUTTER_GESTURE_STATE_RECOGNIZING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_RECOGNIZE_PENDING);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);

  clutter_gesture_set_state (gesture_3, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_destroy (second_actor);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

CLUTTER_TEST_SUITE (
  CLUTTER_TEST_UNIT ("/gesture/relationship/freed-despite-relationship", gesture_relationship_freed_despite_relationship);
  CLUTTER_TEST_UNIT ("/gesture/relationship/simple", gesture_relationship_simple);
  CLUTTER_TEST_UNIT ("/gesture/relationship/simple-2", gesture_relationship_simple_2);
  CLUTTER_TEST_UNIT ("/gesture/relationship/two-points", gesture_relationship_two_points);
  CLUTTER_TEST_UNIT ("/gesture/relationship/two-points-two-actors", gesture_relationship_two_points_two_actors);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-inhibit-move-to-possible", gesture_relationship_global_inhibit_move_to_possible);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-cancel-on-recognize", gesture_relationship_global_cancel_on_recognize);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-recognize-independently", gesture_relationship_global_recognize_independently);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-recognize-independently-2", gesture_relationship_global_recognize_independently_2);
  CLUTTER_TEST_UNIT ("/gesture/relationship/failure-requirement-1", gesture_relationship_failure_requirement_1);
  CLUTTER_TEST_UNIT ("/gesture/relationship/failure-requirement-2", gesture_relationship_failure_requirement_2);
  CLUTTER_TEST_UNIT ("/gesture/relationship/failure-requirement-3", gesture_relationship_failure_requirement_3);
  CLUTTER_TEST_UNIT ("/gesture/relationship/failure-requirement-4", gesture_relationship_failure_requirement_4);
  CLUTTER_TEST_UNIT ("/gesture/relationship/influencing-cascade", gesture_relationship_influencing_cascade);
  CLUTTER_TEST_UNIT ("/gesture/relationship/influencing-cascade-2", gesture_relationship_influencing_cascade_2);
  CLUTTER_TEST_UNIT ("/gesture/relationship/influencing-execution-order", gesture_relationship_influencing_execution_order);
  CLUTTER_TEST_UNIT ("/gesture/relationship/influencing-event-order", gesture_relationship_event_order);
  CLUTTER_TEST_UNIT ("/gesture/relationship/change", gesture_relationship_change);
#if 0
  CLUTTER_TEST_UNIT ("/gesture/relationship-change-2", gesture_relationship_change_2);
#endif
)
