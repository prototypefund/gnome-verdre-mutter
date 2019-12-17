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

#include "clutter-actor-grab.h"

#include "clutter-private.h"
#include "clutter-debug.h"
#include "clutter-event-private.h"
#include "clutter-input-device.h"
#include "clutter-actor-private.h"

typedef struct _ClutterActorGrabPrivate
{
  ClutterActor *grab_actor;
} ClutterActorGrabPrivate;

enum
{
  PROP_0,

  PROP_GRAB_ACTOR,

  PROP_LAST
};

static GParamSpec *obj_props[PROP_LAST] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (ClutterActorGrab, clutter_actor_grab, CLUTTER_TYPE_GRAB);

static void
focus_event (ClutterGrab          *grab,
             ClutterInputDevice   *device,
             ClutterEventSequence *sequence,
             ClutterActor         *old_actor,
             ClutterActor         *new_actor,
             ClutterCrossingMode   mode)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  ClutterActor *topmost_actor = NULL;
  ClutterActor *bottommost_actor = NULL;

  if (priv->grab_actor != NULL)
    {
      if (mode == CLUTTER_CROSSING_GRAB)
        if (clutter_actor_contains (priv->grab_actor, old_actor))
          bottommost_actor = priv->grab_actor;

      if (mode == CLUTTER_CROSSING_UNGRAB)
        if (clutter_actor_contains (priv->grab_actor, new_actor))
          bottommost_actor = priv->grab_actor;
    }

  if (old_actor != NULL && new_actor != NULL)
    {
      if (priv->grab_actor != NULL)
        {
          topmost_actor = priv->grab_actor;

          gboolean grab_contains_old_actor =
            clutter_actor_contains (priv->grab_actor, old_actor);

          gboolean grab_contains_new_actor =
            clutter_actor_contains (priv->grab_actor, new_actor);

          if (!grab_contains_old_actor &&
              !grab_contains_new_actor)
            return;

          if (grab_contains_old_actor &&
              !grab_contains_new_actor)
            new_actor = NULL;

          if (!grab_contains_old_actor &&
              grab_contains_new_actor)
            old_actor = NULL;
        }
      else
        {
          /* We emit leave-events from the just left actor up to the common ancestor
           * and enter-events down to the just entered actor again. */
          for (topmost_actor = old_actor;
               topmost_actor != NULL;
               topmost_actor = clutter_actor_get_parent (topmost_actor))
            {
              if (clutter_actor_contains (topmost_actor, new_actor))
                break;
            }
        }
    }

  clutter_emit_crossing_event (device, sequence, old_actor, new_actor,
                               topmost_actor, bottommost_actor, mode);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->focus_event (grab, device,
    sequence, old_actor, new_actor, mode);
}

static void
key_event (ClutterGrab *grab,
             const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->key_event (grab, event);
}

static void
motion_event (ClutterGrab *grab,
                const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->motion_event (grab, event);
}

static void
button_event (ClutterGrab *grab,
                const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->motion_event (grab, event);
}

static void
scroll_event (ClutterGrab *grab,
                const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->scroll_event (grab, event);
}

static void
touchpad_gesture_event (ClutterGrab *grab,
                          const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->touchpad_gesture_event (grab, event);
}

static void
touch_event (ClutterGrab *grab,
             const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));
  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->touch_event (grab, event);
}

static void
pad_event (ClutterGrab *grab,
             const ClutterEvent     *event)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  clutter_emit_event (event, priv->grab_actor);

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->pad_event (grab, event);
}

/* Return TRUE here if this grab should be used again after the newer grabs
 * are stopped again. Return FALSE to abort this grab. */
static gboolean
cancel (ClutterGrab *grab)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (CLUTTER_ACTOR_GRAB (grab));

  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->cancel (grab);

  /* If this is an implicit grab, we always want to cancel, otherwise we might
   * regain control after the button was already released.
   * In case this grab is attached to a touch sequence, the default grab is
   * implicit and the input device won't cancel the default grab, so it's safe
   * to return FALSE here. */
/*  if (priv->bottommost_actor != NULL)
    return FALSE;
*/
  return TRUE;
}

/*
static void
clutter_actor_grab_dispose (GObject *gobject)
{
  CLUTTER_GRAB_CLASS (clutter_actor_grab_parent_class)->dispose (gobject);
}*/

static void
clutter_actor_grab_set_property (GObject      *gobject,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClutterActorGrab *self = CLUTTER_GRAB (gobject);
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_GRAB_ACTOR:
      priv->grab_actor = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_grab_get_property (GObject    *gobject,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClutterActorGrab *self = CLUTTER_GRAB (gobject);
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_GRAB_ACTOR:
      g_value_set_object (value, priv->grab_actor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_grab_class_init (ClutterActorGrabClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterGrabClass *grab_class = CLUTTER_GRAB_CLASS (klass);

  gobject_class->set_property = clutter_actor_grab_set_property;
  gobject_class->get_property = clutter_actor_grab_get_property;

  grab_class->focus_event = focus_event;
  grab_class->key_event = key_event;
  grab_class->motion_event = motion_event;
  grab_class->button_event = button_event;
  grab_class->scroll_event = scroll_event;
  grab_class->touchpad_gesture_event = touchpad_gesture_event;
  grab_class->touch_event = touch_event;
  grab_class->pad_event = pad_event;
  grab_class->cancel = cancel;

  obj_props[PROP_GRAB_ACTOR] =
    g_param_spec_object ("grab-actor",
                         P_("Grab actor"),
                         P_("The grab actor"),
                         CLUTTER_TYPE_ACTOR,
                         CLUTTER_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class, PROP_LAST, obj_props);
}

static void
clutter_actor_grab_init (ClutterActorGrab *self)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (self);

  priv->grab_actor = NULL;
}

ClutterActorGrab *
clutter_actor_grab_new (ClutterActor *grab_actor)
{
  return g_object_new (CLUTTER_TYPE_ACTOR_GRAB,
                       "grab-actor", grab_actor,
                       NULL);
}

/**
 * clutter_actor_grab_get_grab_actor:
 * @grab: a #ClutterActorGrab
 *
 * Gets the grab actor that's set for this grab.
 *
 * Returns: (transfer none): The grabbed #ClutterActor, or %NULL
 **/
ClutterActor *
clutter_actor_grab_get_grab_actor (ClutterActorGrab *grab)
{
  ClutterActorGrabPrivate *priv =
    clutter_actor_grab_get_instance_private (grab);

  return priv->grab_actor;
}
