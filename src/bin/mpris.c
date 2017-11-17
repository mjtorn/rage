#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "browser.h"
#include "video.h"
#include "winvid.h"
#include "winlist.h"
#include "config.h"
#include "mpris.h"

static Evas_Object *mainwin = NULL;
static Eldbus_Connection *conn = NULL;
static Eldbus_Service_Interface *iface = NULL;
static Eldbus_Service_Interface *iface_player = NULL;

#define SERVICE "org.mpris.MediaPlayer2.rage"
#define PATH    "/org/mpris/MediaPlayer2"

static void
_cb_name_request(void *data EINA_UNUSED, const Eldbus_Message *msg,
                 Eldbus_Pending *pending EINA_UNUSED)
{
   unsigned int flag;

   if (eldbus_message_error_get(msg, NULL, NULL))
     {
        fprintf(stderr, "Could not request bus name");
        return;
     }
   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        fprintf(stderr, "Could not get arguments on on_name_request");
        return;
     }
   if (!(flag & ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER))
     fprintf(stderr, "Name already in use\n");
}

/*
https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html
 */

#define API(fn) \
static Eldbus_Message * \
_cb_api_##fn(const Eldbus_Service_Interface *iface EINA_UNUSED, \
              const Eldbus_Message *msg)
#define GETTER(fn) \
static Eina_Bool \
_cb_prop_##fn##_get(const Eldbus_Service_Interface *iface EINA_UNUSED, \
                        const char *propname EINA_UNUSED, \
                        Eldbus_Message_Iter *iter, \
                        const Eldbus_Message *request_msg EINA_UNUSED, \
                        Eldbus_Message **error EINA_UNUSED)

#define SETTER(fn) \
static Eldbus_Message * \
_cb_prop_##fn##_set(const Eldbus_Service_Interface *iface EINA_UNUSED, \
                        const char *propname EINA_UNUSED, \
                        Eldbus_Message_Iter *iter, \
                        const Eldbus_Message *msg)
#define PROP_RO(str, type, prop) \
   { str, type, _cb_prop_##prop##_get, NULL, 0 }
#define PROP_RW(str, type, prop) \
   { str, type, _cb_prop_##prop##_get, _cb_prop_##prop##_set, 0 }
#define METHOD(str, type, fn) \
   { str,  NULL, type, _cb_api_##fn, 0 }





/////////////////////////////////////////////////////////////////////////////

GETTER(can_quit)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(fullscreen)
{
   Eina_Bool fs = elm_win_fullscreen_get(mainwin);
   eldbus_message_iter_arguments_append(iter, "b", fs);
   return EINA_TRUE;
}

SETTER(fullscreen)
{
   Eina_Bool fs = EINA_FALSE;
   eldbus_message_iter_arguments_get(iter, "d", &fs);
   elm_win_fullscreen_set(mainwin, fs);
   return eldbus_message_method_return_new(msg);
}

GETTER(can_set_fullscreen)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_raise)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(has_track_list)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_FALSE);
   return EINA_TRUE;
}

GETTER(identity)
{
   char buf[] = "Rage";

   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(desktop_entry)
{
   char buf[] = "rage";
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(supported_mime_types)
{
   Eldbus_Message_Iter *array = NULL;

   eldbus_message_iter_arguments_append(iter, "as", &array);
   eldbus_message_iter_arguments_append(array, "s", "application/ogg");
   eldbus_message_iter_arguments_append(array, "s", "video/mpeg");
   eldbus_message_iter_arguments_append(array, "s", "audio/*");
   eldbus_message_iter_arguments_append(array, "s", "video/*");
   eldbus_message_iter_arguments_append(array, "s", "item3");
   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

GETTER(supported_uri_schemes)
{
   Eldbus_Message_Iter *array = NULL;

   eldbus_message_iter_arguments_append(iter, "as", &array);
   eldbus_message_iter_arguments_append(array, "s", "file");
   eldbus_message_iter_arguments_append(array, "s", "http");
   eldbus_message_iter_arguments_append(array, "s", "https");
   eldbus_message_iter_arguments_append(array, "s", "rtsp");
   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

static const Eldbus_Property properties[] =
{
   PROP_RO("CanQuit",             "b",  can_quit),
   PROP_RW("Fullscreen",          "b",  fullscreen),
   PROP_RO("CanSetFullscreen",    "b",  can_set_fullscreen),
   PROP_RO("CanRaise",            "b",  can_raise),
   PROP_RO("HasTrackList",        "b",  has_track_list),
   PROP_RO("Identity",            "s",  identity),
   PROP_RO("DesktopEntry",        "s",  desktop_entry),
   PROP_RO("SupportedMimeTypes",  "as", supported_mime_types),
   PROP_RO("SupportedUriSchemes", "as", supported_uri_schemes),
   { 0 }
};

API(quit)
{
   evas_object_del(mainwin);
   return eldbus_message_method_return_new(msg);
}

API(raize)
{
   elm_win_raise(mainwin);
   return eldbus_message_method_return_new(msg);
}

static const Eldbus_Method methods[] =
{
   METHOD("Quit",  NULL, quit),
   METHOD("Raise", NULL, raize),
   { 0 }
};

static const Eldbus_Service_Interface_Desc desc = {
   "org.mpris.MediaPlayer2", methods, NULL, properties, NULL, NULL
};





/////////////////////////////////////////////////////////////////////////////

GETTER(playback_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = "Stopped";

   if (!inf) goto end;
   if (browser_visible()) goto end;
   if (video_play_get(inf->vid)) buf = "Playing";
   else buf = "Paused";
end:
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

GETTER(loop_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = "None";

   if (!inf) goto end;
   if (browser_visible()) goto end;
   if (video_loop_get(inf->vid)) buf = "Track";
   else buf = "None";
//   "Playlist";
end:
   eldbus_message_iter_arguments_append(iter, "s", buf);
   return EINA_TRUE;
}

SETTER(loop_status)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   const char *buf = NULL;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   eldbus_message_iter_arguments_get(iter, "s", &buf);
   if (buf)
     {
        if (!strcmp(buf, "None"))
          {
             video_loop_set(inf->vid, EINA_FALSE);
             mpris_loop_status_change();
          }
        else if (!strcmp(buf, "Track"))
          {
             video_loop_set(inf->vid, EINA_TRUE);
             mpris_loop_status_change();
          }
        else if (!strcmp(buf, "Playlist"))
          {
             fprintf(stderr, "Cannot do playlist loop yet.\n");
          }
        if (video_loop_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,loop", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,sequential", "rage");
     }
end:
   return eldbus_message_method_return_new(msg);
}

GETTER(rate)
{
   double rate = 1.0;
   eldbus_message_iter_arguments_append(iter, "d", rate);
   return EINA_TRUE;
}

SETTER(rate)
{
   double rate = 1.0;
   eldbus_message_iter_arguments_get(iter, "d", &rate);
   return eldbus_message_method_return_new(msg);
}

GETTER(shuffle)
{
   Eina_Bool shuffle = EINA_FALSE;
   eldbus_message_iter_arguments_append(iter, "b", shuffle);
   return EINA_TRUE;
}

SETTER(shuffle)
{
   Eina_Bool shuffle = EINA_FALSE;
   fprintf(stderr, "Cannot set shuffle mode yet.\n");
   eldbus_message_iter_arguments_get(iter, "b", &shuffle);
   return eldbus_message_method_return_new(msg);
}

/*
GETTER(metadata)
{
   // XXX: return metadata
   eldbus_message_iter_arguments_append(iter, "a{sv}", NULL);
   return EINA_TRUE;
}
*/

GETTER(volume)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   double vol = 1.0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   vol = video_volume_get(inf->vid);
end:
   eldbus_message_iter_arguments_append(iter, "d", vol);
   return EINA_TRUE;
}

SETTER(volume)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   double vol = 1.0; // max

   if (!inf) goto end;
   if (browser_visible()) goto end;
   eldbus_message_iter_arguments_get(iter, "d", &vol);
   if (vol < 0.0) vol = 0.0;
   if (vol > 1.0) vol = 1.0;
   video_volume_set(inf->vid, vol);
   mpris_volume_change();
end:
   return eldbus_message_method_return_new(msg);
}

GETTER(position)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   uint64_t pos = 0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   pos = video_position_get(inf->vid) * 1000000.0;
end:
   eldbus_message_iter_arguments_append(iter, "x", pos);
   return EINA_TRUE;
}

GETTER(minimum_rate)
{
   eldbus_message_iter_arguments_append(iter, "d", 1.0);
   return EINA_TRUE;
}

GETTER(maximum_rate)
{
   eldbus_message_iter_arguments_append(iter, "d", 1.0);
   return EINA_TRUE;
}

GETTER(can_go_next)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_go_previous)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_play)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_pause)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_seek)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

GETTER(can_control)
{
   eldbus_message_iter_arguments_append(iter, "b", EINA_TRUE);
   return EINA_TRUE;
}

static const Eldbus_Property properties_player[] =
{
   PROP_RO("PlaybackStatus", "s", playback_status),
   PROP_RW("LoopStatus",     "s", loop_status),
   PROP_RW("Rate",           "d", rate),
   PROP_RW("Shuffle",        "b", shuffle),
//   PROP_RO("Metadata",       "a{sv}", metadata),
   PROP_RW("Volume",         "d", volume),
   PROP_RO("Position",       "x", position),
   PROP_RO("MinimumRate",    "d", minimum_rate),
   PROP_RO("MaximumRate",    "d", maximum_rate),
   PROP_RO("CanGoNext",      "b", can_go_next),
   PROP_RO("CanGoPrevious",  "b", can_go_previous),
   PROP_RO("CanPlay",        "b", can_play),
   PROP_RO("CanPause",       "b", can_pause),
   PROP_RO("CanSeek",        "b", can_seek),
   PROP_RO("CanControl",     "b", can_control),
   { 0 }
};

API(next)
{
   if (browser_visible()) goto end;
   win_do_next(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(previous)
{
   if (browser_visible()) goto end;
   win_do_prev(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(pause)
{
   if (browser_visible()) goto end;
   win_do_pause(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(play_pause)
{
   if (browser_visible()) goto end;
   win_do_play_pause(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(stop)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");

   if (!inf) goto end;
   if (browser_visible()) goto end;
   video_stop(inf->vid);
   elm_layout_signal_emit(inf->lay, "action,stop", "rage");
   elm_layout_signal_emit(inf->lay, "state,default", "rage");
   if (inf->browse_mode) browser_show(mainwin);
   else evas_object_del(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(play)
{
   if (browser_visible()) goto end;
   win_do_play(mainwin);
end:
   return eldbus_message_method_return_new(msg);
}

API(seek)
{
   Inf *inf = evas_object_data_get(mainwin, "inf");
   uint64_t pos = 0;

   if (!inf) goto end;
   if (browser_visible()) goto end;
   // XXX: seek to pos in usec
   if (!eldbus_message_arguments_get(msg, "x", &pos))
     return eldbus_message_error_new(msg, "Invalid arguments", "Error getting position");
   video_position_set(inf->vid, (double)pos / 1000000.0);
end:
   return eldbus_message_method_return_new(msg);
}

API(set_position)
{
   fprintf(stderr, "Cannot do set_position yet.\n");
   // XXX: get track + position
   return eldbus_message_method_return_new(msg);
}

API(open_uri)
{
   char *uri = NULL;

   // XXX: handle uri open
   if (!eldbus_message_arguments_get(msg, "s", &uri))
     return eldbus_message_error_new(msg, "Invalid arguments", "Error getting URI string");
   win_video_insert(mainwin, uri);
   win_video_next(mainwin);
   win_list_content_update(mainwin);
   browser_hide(mainwin);
   return eldbus_message_method_return_new(msg);
}

static const Eldbus_Method methods_player[] =
{
   METHOD("Next", NULL, next),
   METHOD("Previous", NULL, previous),
   METHOD("Pause", NULL, pause),
   METHOD("PlayPause", NULL, play_pause),
   METHOD("Stop", NULL, stop),
   METHOD("Play", NULL, play),
   METHOD("Seek", ELDBUS_ARGS({"x", "Offset"}), seek),
   METHOD("SetPosition", ELDBUS_ARGS({ "ox", "Path,Position"}), set_position),
   METHOD("OpenUri", ELDBUS_ARGS({"s", "Uri"}), open_uri),
   { 0 }
};

static const Eldbus_Signal signals_player[] =
{
   [ 0 ] = { "Seeked", ELDBUS_ARGS({ "x", "Position" }), 0 },
   { 0 }
};
// XXX: Signal: Seeked (x: Position)

static const Eldbus_Service_Interface_Desc desc_player = {
   "org.mpris.MediaPlayer2.Player", methods_player, signals_player, properties_player, NULL, NULL
};

void
mpris_fullscreen_change(void)
{
   if (!iface) return;
   eldbus_service_property_changed(iface, "Fullscreen");
}

void
mpris_volume_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "Volume");
}

void
mpris_loop_status_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "LoopStatus");
}

void
mpris_playback_status_change(void)
{
   if (!iface_player) return;
   eldbus_service_property_changed(iface_player, "PlaybackStatus");
}

void
mpris_position_change(double pos)
{
   uint64_t p = pos * 1000000.0;

   if (!iface_player) return;
   eldbus_service_signal_emit(iface_player, 0, p);
   eldbus_service_property_changed(iface_player, "Position");
}

void
mpris_init(Evas_Object *win)
{
   elm_need_eldbus();
   conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!conn) return;
   mainwin = win;
   eldbus_name_request(conn, SERVICE, 0, _cb_name_request, NULL);
   iface = eldbus_service_interface_register(conn, PATH, &desc);
   iface_player = eldbus_service_interface_register(conn, PATH, &desc_player);
}

void
mpris_shutdown(void)
{
   if (!conn) return;
   mainwin = NULL;
//   if (iface_player) eldbus_service_object_unregister(iface_player);
   iface_player = NULL;
//   if (iface) eldbus_service_object_unregister(iface);
   iface = NULL;
//   eldbus_name_release(conn, SERVICE, NULL, NULL);
//   eldbus_connection_unref(conn);
   conn = NULL;
}
