/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright © 2009, 2010, 2011  Intel Corp.
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
 *
 * Author: Jonas Dreßler <verdre@v0yd.nl>
 */

#ifndef __clutter_grab_H__
#define __clutter_grab_H__

#if !defined(__CLUTTER_H_INSIDE__) && !defined(CLUTTER_COMPILATION)
#error "Only <clutter/clutter.h> can be included directly."
#endif

#include <clutter/clutter-types.h>

#define CLUTTER_TYPE_GRAB (clutter_grab_get_type ())

CLUTTER_EXPORT
G_DECLARE_DERIVABLE_TYPE (ClutterGrab, clutter_grab,
                          CLUTTER, GRAB, GObject);

/**
 * ClutterGrabClass:
 * @focus_event: virtual function called when emitting crossing events
 * @key_event: virtual function called when emitting key events
 * @motion_event: virtual function called when emitting motion events
 * @button_event: virtual function called when emitting button events
 * @scroll_event: virtual function called when emitting scroll events
 * @touchpad_gesture_event: virtual function called when emitting touchpad
 *   gesture events
 * @touch_event: virtual function called when emitting touch events
 * @pad_event: virtual function called when emitting pad events
 * @cancel: virtual function called when emitting pad events
 *
 * The #ClutterGrabClass structure contains
 * only private data
 */
typedef struct _ClutterGrabClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/

  /**
   * ClutterGrabClass::focus_event:
   * @grab: a #ClutterGrab
   * @device: the #ClutterInputDevice the event belongs to
   * @sequence: (allow-none): the #ClutterEventSequence sequence of the touch, or %NULL
   * @old_actor: (allow-none): the #ClutterActor that has been left by the pointer, or %NULL
   * @new_actor: (allow-none): the #ClutterActor that has been entered by the pointer, or %NULL
   * @mode: the #ClutterCrossingMode of the event
   *
   * Virtual function, called when emitting a crossing event.
   */
  void (*focus_event) (ClutterGrab *grab,
                 ClutterInputDevice     *device,
                 ClutterEventSequence   *sequence,
                 ClutterActor           *old_actor,
                 ClutterActor           *new_actor,
                ClutterCrossingMode mode);

  /**
   * ClutterGrabClass::key_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a key event.
   */
  void (*key_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::motion_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a motion event.
   */
  void (*motion_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::button_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a button event.
   */
  void (*button_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::scroll_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a scroll event.
   */
  void (*scroll_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::touchpad_gesture_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a touchpad gesture event.
   */
  void (*touchpad_gesture_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::touch_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a touch event.
   */
  void (*touch_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::pad_event:
   * @grab: a #ClutterGrab
   * @event: a #ClutterEvent
   *
   * Virtual function, called when emitting a pad event.
   */
  void (*pad_event) (ClutterGrab *grab,
                  const ClutterEvent     *event);

  /**
   * ClutterGrabClass::cancel:
   * @grab: a #ClutterGrab
   *
   * Virtual function, called when the grab is cancelled because another
   * grab superseeded it.
   *
   * Returns: %TRUE if the grab should be removed, %FALSE if the grab should
   *   be put in place again after the superseeding grab ended.
   */
  gboolean (*cancel) (ClutterGrab *grab);
} ClutterGrabClass;

void clutter_grab_emit_focus (ClutterGrab *grab,
                                           ClutterInputDevice     *device,
                                           ClutterEventSequence   *sequence,
                                           ClutterActor           *old_actor,
                                           ClutterActor           *new_actor,
ClutterCrossingMode mode);

void clutter_grab_emit_key (ClutterGrab *grab,
                                         const ClutterEvent     *event);

void clutter_grab_emit_motion (ClutterGrab *grab,
                                            const ClutterEvent     *event);

void clutter_grab_emit_button (ClutterGrab *grab,
                                            const ClutterEvent     *event);

void clutter_grab_emit_scroll (ClutterGrab *grab,
                                            const ClutterEvent     *event);

void clutter_grab_emit_touchpad_gesture (ClutterGrab *grab,
                                                      const ClutterEvent     *event);

void clutter_grab_emit_touch (ClutterGrab *grab,
                                           const ClutterEvent     *event);

void clutter_grab_emit_pad (ClutterGrab *grab,
                                         const ClutterEvent     *event);

gboolean clutter_grab_emit_cancel (ClutterGrab *grab);

#endif /* __CLUTTER_INPUT_DEVICE_H__ */

