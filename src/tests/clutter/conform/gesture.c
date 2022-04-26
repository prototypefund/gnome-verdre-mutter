#define CLUTTER_DISABLE_DEPRECATION_WARNINGS
#include <clutter/clutter.h>

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
emit_event (ClutterStage     *stage,
            ClutterEventType  type,
            float             x,
            float             y)
{
  ClutterSeat *seat =
    clutter_backend_get_default_seat (clutter_get_default_backend ());
  ClutterInputDevice *pointer = clutter_seat_get_pointer (seat);
  ClutterEvent *event = clutter_event_new (type);

  clutter_event_set_coords (event, x, y);
  clutter_event_set_device (event, pointer);
  clutter_event_set_stage (event, stage);

  clutter_event_put (event);
  clutter_event_free (event);
}

static void
gesture_state_machine (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  ClutterGesture *gesture = CLUTTER_GESTURE (clutter_gesture_new ());
  gboolean was_presented;

  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);

  clutter_actor_add_action (stage, CLUTTER_ACTION (gesture));

  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);

  g_signal_connect (stage, "presented", G_CALLBACK (on_presented),
                    &was_presented);

  clutter_actor_show (stage);

  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);

  emit_event (CLUTTER_STAGE (stage), CLUTTER_BUTTON_PRESS, 15, 15);
  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_POSSIBLE);

  clutter_gesture_set_state (gesture, CLUTTER_GESTURE_STATE_CANCELLED);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_CANCELLED);

  emit_event (CLUTTER_STAGE (stage), CLUTTER_BUTTON_RELEASE, 15, 15);
  was_presented = FALSE;
  while (!was_presented)
    g_main_context_iteration (NULL, FALSE);
  g_assert_true (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_WAITING);
}

CLUTTER_TEST_SUITE (
  CLUTTER_TEST_UNIT ("/gesture/state-machine", gesture_state_machine);
)
