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

#include "clutter-build-config.h"

#include "clutter-grab.h"

#include "clutter-private.h"
#include "clutter-debug.h"
#include "clutter-event-private.h"
#include "clutter-input-device.h"
#include "clutter-actor-private.h"

typedef struct _ClutterGrabPrivate
{
} ClutterGrabPrivate;

G_DEFINE_TYPE (ClutterGrab, clutter_grab, G_TYPE_OBJECT);

static void
default_focus (ClutterGrab *grab,
               ClutterInputDevice     *device,
               ClutterEventSequence   *sequence,
               ClutterActor           *old_actor,
               ClutterActor           *new_actor,
                ClutterCrossingMode mode)
{
}

static void
default_key (ClutterGrab *grab,
             const ClutterEvent     *event)
{
}

static void
default_motion (ClutterGrab *grab,
                const ClutterEvent     *event)
{
}

static void
default_button (ClutterGrab *grab,
                const ClutterEvent     *event)
{
}

static void
default_scroll (ClutterGrab *grab,
                const ClutterEvent     *event)
{
}

static void
default_touchpad_gesture (ClutterGrab *grab,
                          const ClutterEvent     *event)
{
}

static void
default_touch (ClutterGrab *grab,
               const ClutterEvent     *event)
{
}

static void
default_pad (ClutterGrab *grab,
             const ClutterEvent     *event)
{
}

static gboolean
default_cancel (ClutterGrab *grab)
{
  return FALSE;
}


static void
clutter_grab_class_init (ClutterGrabClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->focus_event = default_focus;
  klass->key_event = default_key;
  klass->motion_event = default_motion;
  klass->button_event = default_button;
  klass->scroll_event = default_scroll;
  klass->touchpad_gesture_event = default_touchpad_gesture;
  klass->touch_event = default_touch;
  klass->pad_event = default_pad;
  klass->cancel = default_cancel;
}

static void
clutter_grab_init (ClutterGrab *self)
{
}

void
clutter_grab_emit_focus (ClutterGrab *grab,
                                      ClutterInputDevice     *device,
                                      ClutterEventSequence   *sequence,
                                      ClutterActor           *old_actor,
                                      ClutterActor           *new_actor,
                          ClutterCrossingMode mode)
{
  CLUTTER_GRAB_GET_CLASS (grab)->focus_event (grab, device, sequence,
                                                     old_actor, new_actor, mode);
}

void
clutter_grab_emit_key (ClutterGrab *grab,
                                    const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->key_event (grab, event);
}

void
clutter_grab_emit_motion (ClutterGrab *grab,
                                       const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->motion_event (grab, event);
}

void
clutter_grab_emit_button (ClutterGrab *grab,
                                       const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->button_event (grab, event);
}

void
clutter_grab_emit_scroll (ClutterGrab *grab,
                                       const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->scroll_event (grab, event);
}

void
clutter_grab_emit_touchpad_gesture (ClutterGrab *grab,
                                                 const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->touchpad_gesture_event (grab, event);
}

void
clutter_grab_emit_touch (ClutterGrab *grab,
                                      const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->touch_event (grab, event);
}

void
clutter_grab_emit_pad (ClutterGrab *grab,
                                    const ClutterEvent *event)
{
  CLUTTER_GRAB_GET_CLASS (grab)->pad_event (grab, event);
}

gboolean
clutter_grab_emit_cancel (ClutterGrab *grab)
{
  return CLUTTER_GRAB_GET_CLASS (grab)->cancel (grab);
}

