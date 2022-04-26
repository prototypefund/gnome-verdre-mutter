#define CLUTTER_DISABLE_DEPRECATION_WARNINGS
#include <clutter/clutter.h>

#include "tests/clutter-test-utils.h"
#if 0
static void
create_touchscreen (void)
{
  ClutterVirtualInputDevice *touchscreen;
  ClutterSeat *seat;
  guint notify_id;

  seat = clutter_backend_get_default_seat (clutter_get_default_backend ());
  touchscreen = clutter_seat_create_virtual_device (seat, CLUTTER_TOUCHSCREEN_DEVICE);

  clutter_virtual_input_device_notify_touch_down (touchscreen, 0, 0, 15, 15);

  /*notify_id = g_signal_connect (actor, "notify::has-pointer",
                                G_CALLBACK (has_pointer_cb), NULL);
  clutter_test_main ();

  g_signal_handler_disconnect (actor, notify_id);
*/
  g_object_unref (touchscreen);
}
#endif

static void
button_release_cb (ClutterActor *actor)
{
    clutter_test_quit ();
}

static void
create_pointer (ClutterActor *actor)
{
  ClutterVirtualInputDevice *pointer;
  ClutterSeat *seat;
  guint notify_id;

  seat = clutter_backend_get_default_seat (clutter_get_default_backend ());
  pointer = clutter_seat_create_virtual_device (seat, CLUTTER_POINTER_DEVICE);

  clutter_virtual_input_device_notify_absolute_motion (pointer,
                                                       0,
                                                       15, 15);

  clutter_virtual_input_device_notify_absolute_motion (pointer,
                                                       0,
                                                       15, 16);

  clutter_virtual_input_device_notify_button (pointer, 10, 1, CLUTTER_BUTTON_STATE_PRESSED);
  clutter_virtual_input_device_notify_button (pointer, 33, CLUTTER_BUTTON_PRIMARY, CLUTTER_BUTTON_STATE_RELEASED);

//  notify_id = g_signal_connect (actor, "event",
  //                              G_CALLBACK (button_release_cb), NULL);

  clutter_test_main ();

  g_signal_handler_disconnect (actor, notify_id);

  g_object_unref (pointer);
}

static void
check_was_cancelled (ClutterGesture *gesture,
                      GParamSpec         *pspec,
                      gboolean            *was_cancelled)
{
  if (clutter_gesture_get_state (gesture) == CLUTTER_GESTURE_STATE_CANCELLED)
    *was_cancelled = TRUE;
}

static void
gesture_relationship_weak_refs (void)
{
  ClutterActor *actor = clutter_test_get_stage ();
  ClutterGesture *action_1 = clutter_gesture_new ();
  ClutterGesture *action_2 = clutter_gesture_new ();

  g_object_add_weak_pointer (G_OBJECT (action_1), (gpointer *) &action_1);
  g_object_add_weak_pointer (G_OBJECT (action_2), (gpointer *) &action_2);

  clutter_gesture_can_not_cancel (action_1, action_2);

  g_object_unref (action_2);
  g_assert_null (action_2);

  g_object_unref (action_1);
  g_assert_null (action_1);

  clutter_actor_set_reactive (actor, TRUE);

  action_1 = clutter_gesture_new ();
  action_2 = clutter_gesture_new ();

  clutter_actor_add_action (actor, action_1);
  clutter_actor_add_action (actor, action_2);

  g_object_add_weak_pointer (G_OBJECT (action_1), (gpointer *) &action_1);
  g_object_add_weak_pointer (G_OBJECT (action_2), (gpointer *) &action_2);

//  clutter_gesture_action_new_require_failure_of (action_1, action_2);

  g_object_unref (action_2);
  g_assert_null (action_2);

  //create_pointer (actor);
}

static void
setup_relationships (unsigned int n_gestures,
                     ...)
{
  va_list valist, valist_2;
  unsigned int i;

  ClutterEvent *event = clutter_event_new (CLUTTER_TOUCH_BEGIN);
  event->touch.sequence = 0x1;
  event->touch.device = 0x1;

  va_start (valist, n_gestures);
  for (i = 0; i < n_gestures; i++)
    {
      ClutterGesture *gesture = va_arg (valist, ClutterGesture *);
      ClutterActionClass *action_class = CLUTTER_ACTION_GET_CLASS (gesture);
      unsigned int j;

      va_start (valist_2, n_gestures);
      for (j = 0; j < n_gestures; j++)
        {
          ClutterGesture *other_gesture = va_arg (valist_2, ClutterGesture *);
          if (j <= i)
            continue;

          action_class->setup_sequence_relationship (gesture, other_gesture, event);
        }
      va_end (valist_2);
    }
  va_end (valist);
}

static void
gesture_relationship_simple (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  gboolean action_1_was_cancelled, action_2_was_cancelled;

  ClutterGesture *action_1 = clutter_gesture_new ();
  ClutterGesture *action_2 = clutter_gesture_new ();

  action_1_was_cancelled = action_2_was_cancelled = FALSE;
  g_signal_connect (action_1, "notify::state",
                    G_CALLBACK (check_was_cancelled),
                    &action_1_was_cancelled);
  g_signal_connect (action_2, "notify::state",
                    G_CALLBACK (check_was_cancelled),
                    &action_2_was_cancelled);
}

static void
gesture_relationship_circular_dependency (void)
{
  ClutterActor *stage = clutter_test_get_stage ();
  gboolean action_1_was_cancelled, action_2_was_cancelled;
  gboolean action_3_was_cancelled;

  ClutterGesture *action_1 =
    g_object_new (CLUTTER_TYPE_GESTURE, "name", "action-1", NULL);
  ClutterGesture *action_2 =
    g_object_new (CLUTTER_TYPE_GESTURE, "name", "action-2", NULL);
  ClutterGesture *action_3 =
    g_object_new (CLUTTER_TYPE_GESTURE, "name", "action-3", NULL);

  action_1_was_cancelled = action_2_was_cancelled = FALSE;
  action_3_was_cancelled = FALSE;
  g_signal_connect (action_1, "notify::state",
                    G_CALLBACK (check_was_cancelled),
                    &action_1_was_cancelled);
  g_signal_connect (action_2, "notify::state",
                    G_CALLBACK (check_was_cancelled),
                    &action_2_was_cancelled);
  g_signal_connect (action_3, "notify::state",
                    G_CALLBACK (check_was_cancelled),
                    &action_3_was_cancelled);

}

CLUTTER_TEST_SUITE (
  CLUTTER_TEST_UNIT ("/gesture/relationship/weak-refs", gesture_relationship_weak_refs);
  CLUTTER_TEST_UNIT ("/gesture/relationship/simple", gesture_relationship_simple);
  CLUTTER_TEST_UNIT ("/gesture/relationship/circular-dependency", gesture_relationship_circular_dependency);
)
