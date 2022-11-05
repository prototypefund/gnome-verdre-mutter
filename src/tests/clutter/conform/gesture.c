#define CLUTTER_DISABLE_DEPRECATION_WARNINGS
#include <clutter/clutter.h>

#include "clutter/clutter-mutter.h"

#include "tests/clutter-test-utils.h"

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

static void
gesture_disposed_while_active (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterActor *second_actor = clutter_actor_new ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (clutter_gesture_new ());
  gboolean was_presented;

  clutter_actor_set_size (second_actor, 20, 20);
  clutter_actor_set_x (second_actor, 15);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_1));

  g_object_add_weak_pointer (G_OBJECT (gesture_1), (gpointer *) &gesture_1);

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

  clutter_gesture_set_state (gesture_1, CLUTTER_GESTURE_STATE_COMPLETED);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);

  clutter_actor_destroy (second_actor);
  g_assert_nonnull (gesture_1);
  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_COMPLETED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_null (gesture_1);

  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_state_machine_move_to_waiting (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture = CLUTTER_GESTURE (clutter_gesture_new ());
  gboolean was_presented;
  unsigned int n_points;

  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture));
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_CANCELLED);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 0);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_CANCELLED);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 0);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_multiple_mouse_buttons (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture = CLUTTER_GESTURE (clutter_gesture_new ());
  gboolean was_presented;
  unsigned int n_points;

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture));

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 5, 5);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 1);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);
  clutter_gesture_get_points (gesture, &n_points);
  g_assert_cmpint (n_points, ==, 0);

  clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_event_order (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_PAN_GESTURE, "name", "pan-gesture-1", "begin-threshold", 0, NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_PAN_GESTURE, "name", "pan-gesture-2", "begin-threshold", 0, NULL));
  ClutterGesture *gesture_3 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_PAN_GESTURE, "name", "pan-gesture-3", "begin-threshold", 0, NULL));
  gboolean was_presented;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  ClutterActor *second_actor = clutter_actor_new ();

  clutter_actor_set_size (second_actor, 200, 200);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  clutter_actor_show (stage);

  /* Need to wait for one presentation so that picking with second actor works */
  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);

  /* Default phase is BUBBLE */
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_3));

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_3) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_destroy (second_actor);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}

static void
gesture_event_order_2 (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture_1 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_PAN_GESTURE, "name", "pan-gesture-1", "begin-threshold", 0, NULL));
  ClutterGesture *gesture_2 = CLUTTER_GESTURE (g_object_new (CLUTTER_TYPE_PAN_GESTURE, "name", "pan-gesture-2", "begin-threshold", 0, NULL));
  gboolean was_presented;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  ClutterActor *second_actor = clutter_actor_new ();

  clutter_actor_set_size (second_actor, 200, 200);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  clutter_actor_show (stage);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action_full (stage, "", CLUTTER_PHASE_CAPTURE, CLUTTER_ACTION (gesture_2));

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_RECOGNIZING);

  emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);

  g_assert_true (clutter_gesture_get_state (gesture_1) == CLUTTER_GESTURE_STATE_WAITING);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_destroy (second_actor);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}
/*
static void
gesture_event_order (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
ClutterGesture *gesture_1 = clutter_gesture_new ();
  ClutterGesture *gesture_2 = clutter_gesture_new ();
  ClutterGesture *gesture_3 = clutter_gesture_new ();

  gboolean was_presented = FALSE;

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  ClutterActor *second_actor = clutter_actor_new ();

  clutter_actor_set_size (second_actor, 200, 200);
  clutter_actor_set_reactive (second_actor, true);
  clutter_actor_add_child (stage, second_actor);

  clutter_actor_show (stage);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_add_action (second_actor, CLUTTER_ACTION (gesture_3));

 // emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_PRESS, 15, 15);

 // emit_event_and_wait (CLUTTER_STAGE (stage), &was_presented, CLUTTER_BUTTON_RELEASE, 15, 15);

  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_1));
  clutter_actor_remove_action (stage, CLUTTER_ACTION (gesture_2));
  clutter_actor_remove_action (second_actor, CLUTTER_ACTION (gesture_3));

  clutter_actor_destroy (second_actor);

  g_signal_handlers_disconnect_by_func (stage, on_presented, &was_presented);
}*/

CLUTTER_TEST_SUITE (
  CLUTTER_TEST_UNIT ("/gesture/disposed-while-active", gesture_disposed_while_active);
  CLUTTER_TEST_UNIT ("/gesture/state-machine-move-to-waiting", gesture_state_machine_move_to_waiting);
  CLUTTER_TEST_UNIT ("/gesture/multiple-mouse-buttons", gesture_multiple_mouse_buttons);
  CLUTTER_TEST_UNIT ("/gesture/event-order", gesture_event_order);
  CLUTTER_TEST_UNIT ("/gesture/event-order-2", gesture_event_order_2);
)
