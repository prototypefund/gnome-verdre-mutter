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

  clutter_gesture_set_state (gesture_2, CLUTTER_GESTURE_STATE_POSSIBLE);
  g_assert_true (clutter_gesture_get_state (gesture_2) == CLUTTER_GESTURE_STATE_WAITING);

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

  g_object_unref (gesture_1);
  g_object_unref (gesture_2);
}

CLUTTER_TEST_SUITE (
  CLUTTER_TEST_UNIT ("/gesture/relationship/simple", gesture_relationship_simple);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-inhibit-move-to-possible", gesture_relationship_global_inhibit_move_to_possible);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-cancel-on-recognize", gesture_relationship_global_cancel_on_recognize);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-recognize-independently", gesture_relationship_global_recognize_independently);
  CLUTTER_TEST_UNIT ("/gesture/relationship/global-recognize-independently-2", gesture_relationship_global_recognize_independently_2);
)
