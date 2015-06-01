#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "video.h"
#include "dnd.h"
#include "key.h"
#include "controls.h"
#include "gesture.h"
#include "albumart.h"

static void
_cb_fullscreen(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   elm_layout_signal_emit(inf->lay, "state,win,fullscreen", "rage");
   elm_win_noblank_set(obj, EINA_TRUE);
   evas_object_show(inf->event2);
}

static void
_cb_unfullscreen(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   elm_layout_signal_emit(inf->lay, "state,win,normal", "rage");
   elm_win_noblank_set(obj, EINA_FALSE);
   evas_object_hide(inf->event2);
}

static void
_cb_win_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *ev EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   Winvid_Entry *vid;

   if (inf->next_job) ecore_job_del(inf->next_job);
   if (inf->show_timeout) ecore_timer_del(inf->show_timeout);
   if (inf->drag_anim) ecore_animator_del(inf->drag_anim);
   if (inf->mouse_idle_timeout) ecore_timer_del(inf->mouse_idle_timeout);
   if (inf->albumart_timeout) ecore_timer_del(inf->albumart_timeout);
   EINA_LIST_FREE(inf->file_list, vid)
     {
        if (vid->file) eina_stringshare_del(vid->file);
        if (vid->sub) eina_stringshare_del(vid->sub);
        if (vid->uri) efreet_uri_free(vid->uri);
        free(vid);
     }
   evas_object_data_del(obj, "inf");
   free(inf);
   dnd_shutdown();
}

static void
_cb_key_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   key_handle(data, event_info);
}

static Eina_Bool
_cb_mouse_idle(void *data)
{
   Inf *inf = evas_object_data_get(data, "inf");
   inf->mouse_idle_timeout = NULL;
   if (elm_win_fullscreen_get(data)) evas_object_show(inf->event2);
   return EINA_FALSE;
}

static void
_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (inf->mouse_idle_timeout) ecore_timer_del(inf->mouse_idle_timeout);
   inf->mouse_idle_timeout = ecore_timer_add(5.0, _cb_mouse_idle, data);
   evas_object_hide(inf->event2);
}

static void
_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *m_info = event_info;

   if (m_info->flags & EVAS_BUTTON_DOUBLE_CLICK)
     elm_win_fullscreen_set(data, !elm_win_fullscreen_get(data));
}

static Eina_Bool
_cb_albumart_delay(void *data)
{
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");

   if (!inf) return EINA_FALSE;
   inf->albumart_timeout = NULL;
   if (!inf->vid) return EINA_FALSE;

   if ((!video_has_video_get(inf->vid)) && (video_has_audio_get(inf->vid)))
     albumart_find(win, inf->vid);
   else albumart_find(win, NULL);
   return EINA_FALSE;
}

void
win_do_play(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, EINA_TRUE);
   elm_layout_signal_emit(inf->lay, "action,play", "rage");
}

void
win_do_pause(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, EINA_FALSE);
   elm_layout_signal_emit(inf->lay, "action,pause", "rage");
}

void
win_do_play_pause(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, !video_play_get(inf->vid));
   if (video_play_get(inf->vid))
     elm_layout_signal_emit(inf->lay, "action,play", "rage");
   else
     elm_layout_signal_emit(inf->lay, "action,pause", "rage");
}

void
win_do_prev(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (video_spu_button_count(inf->vid) > 0)
     video_event_send(inf->vid, EMOTION_EVENT_PREV);
   else
     {
        // if we have chapters - send prev event to jump chapters
        if (video_chapter_count(inf->vid) > 0)
          video_event_send(inf->vid, EMOTION_EVENT_PREV);
        else
          {
             double pos = video_position_get(inf->vid);

             if ((pos < 5.0) && win_video_have_prev(win)) win_video_prev(win);
             else video_position_set(inf->vid, 0.0);
             elm_layout_signal_emit(inf->lay, "action,prev", "rage");
          }
     }
}

void
win_do_next(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (video_spu_button_count(inf->vid) > 0)
     video_event_send(inf->vid, EMOTION_EVENT_NEXT);
   else
     {
        // if we have chapters - send next event to jump chapters
        if (video_chapter_count(inf->vid) > 0)
          video_event_send(inf->vid, EMOTION_EVENT_NEXT);
        else
          {
             if (!win_video_have_next(win))
               {
                  double pos = video_position_get(inf->vid);
                  double len = video_length_get(inf->vid);

                  if ((len - pos) > 5.0)
                    {
                       video_position_set(inf->vid, len - 5.0);
                       elm_layout_signal_emit(inf->lay, "action,next", "rage");
                    }
               }
             else
               {
                  win_video_next(win);
                  elm_layout_signal_emit(inf->lay, "action,next", "rage");
               }
          }
     }
}

static void
_restart_vid(Evas_Object *win, Evas_Object *lay, Evas_Object *vid, const char *file, const char *sub)
{
   video_position_set(vid, 0.0);
   video_play_set(vid, EINA_FALSE);
   video_file_autosub_set(vid, file, sub);
   video_position_set(vid, 0.0);
   video_play_set(vid, EINA_TRUE);
   elm_layout_signal_emit(lay, "action,newvid", "rage");
   win_aspect_adjust(win);
   win_title_update(win);
}

void
win_video_restart(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Winvid_Entry *vid = inf->file_cur->data;
   _restart_vid(win, inf->lay, inf->vid, vid->file, vid->sub);
}

void
win_video_next(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Winvid_Entry *vid;

   if (!inf->file_list) return;
   if (!inf->file_cur) l = inf->file_list;
   else l = inf->file_cur->next;
   if (!l)
     {
        elm_exit();
        return;
     }
   inf->file_cur = l;
   vid = l->data;
   _restart_vid(win, inf->lay, inf->vid, vid->file, vid->sub);
   win_list_sel_update(win);
}

void
win_video_prev(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Winvid_Entry *vid;

   if (!inf->file_list) return;
   if (!inf->file_cur) l = eina_list_last(inf->file_list);
   else l = inf->file_cur->prev;
   if (!l) return;
   inf->file_cur = l;
   vid = l->data;
   _restart_vid(win, inf->lay, inf->vid, vid->file, vid->sub);
   win_list_sel_update(win);
}

void
win_video_first(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Winvid_Entry *vid;

   if (!inf->file_list) return;
   l = inf->file_list;
   inf->file_cur = l;
   vid = l->data;
   _restart_vid(win, inf->lay, inf->vid, vid->file, vid->sub);
   win_list_sel_update(win);
}

void
win_video_last(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   Winvid_Entry *vid;

   if (!inf->file_list) return;
   l = eina_list_last(inf->file_list);
   if (!l) return;
   inf->file_cur = l;
   vid = l->data;
   _restart_vid(win, inf->lay, inf->vid, vid->file, vid->sub);
   win_list_sel_update(win);
}

void
win_video_delete(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l, *l_next;
   Winvid_Entry *vid;

   int direction = 0; // -1 prev, 1 next

   if (!inf->file_list) return;

   // If we're able to delete something, do it,
   // and decide on next/prev
   if (win_video_have_next(win) || win_video_have_prev(win))
     {
        EINA_LIST_FOREACH_SAFE(inf->file_list, l, l_next, vid)
          {
             if (l == inf->file_cur)
               {
                  if (vid->file) eina_stringshare_del(vid->file);
                  if (vid->sub) eina_stringshare_del(vid->sub);
                  if (vid->uri) efreet_uri_free(vid->uri);
                  free(vid);
                  inf->file_list = eina_list_remove_list(inf->file_list, l);
                  direction = (l_next == 0 ? -1 : 1);
                  break;
               }
          }
     }

   // Move to a direction and update playlist, which is confused
   if (direction == -1) win_do_prev(win);
   else if (direction == 1) win_do_next(win);
   win_list_content_update(win);
}

Eina_Bool
win_video_have_next(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;

   if (!inf->file_list) return EINA_FALSE;
   if (!inf->file_cur) return EINA_FALSE;
   else l = inf->file_cur->next;
   if (!l) return EINA_FALSE;
   return EINA_TRUE;
}

Eina_Bool
win_video_have_prev(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;

   if (!inf->file_list) return EINA_FALSE;
   if (!inf->file_cur) return EINA_FALSE;
   else l = inf->file_cur->prev;
   if (!l) return EINA_FALSE;
   return EINA_TRUE;
}

Evas_Object *
win_add(void)
{
   Evas_Object *win, *o;
   char buf[4096];
   Inf *inf = calloc(1, sizeof(Inf));

   if (!inf) return NULL;

   win = elm_win_add(NULL, "Rage", ELM_WIN_BASIC);
   if (!win)
     {
        free(inf);
        return NULL;
     }

   elm_win_title_set(win, "Rage");
   elm_win_autodel_set(win, EINA_TRUE);

   evas_object_data_set(win, "inf", inf);
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _cb_win_del, NULL);
   evas_object_smart_callback_add(win, "fullscreen", _cb_fullscreen, NULL);
   evas_object_smart_callback_add(win, "unfullscreen", _cb_unfullscreen, NULL);
   evas_object_smart_callback_add(win, "normal", _cb_unfullscreen, NULL);

   o = evas_object_image_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/rage.png", elm_app_data_dir_get());
   evas_object_image_file_set(o, buf, NULL);
   elm_win_icon_object_set(win, o);

   o = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
   elm_layout_file_set(o, buf, "rage/core");
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, o);
   evas_object_show(o);
   inf->lay = o;
   controls_init(win, o);

   o = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, o);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, EINA_TRUE);
   evas_object_show(o);
   inf->event = o;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                  _cb_mouse_move, win);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,
                                  _cb_mouse_move, win);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_mouse_down, win);
   elm_object_part_content_set(inf->lay, "rage.gesture", o);
   gesture_init(win, o);
   dnd_init(win, o);

   o = elm_button_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, o);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, EINA_TRUE);
   elm_object_cursor_set(o, "blank");
   elm_object_cursor_theme_search_enabled_set(o, EINA_TRUE);
   inf->event2 = o;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                  _cb_mouse_move, win);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT,
                                  _cb_mouse_move, win);

   // a dummy button to collect key events and have focus
   o = elm_button_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, o);
   evas_object_lower(o);
   evas_object_show(o);
   elm_object_focus_set(o, EINA_TRUE);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN,
                                  _cb_key_down, win);
   return win;
}

void
win_title_update(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   const char *file, *s;
   char buf[4096];
   Winvid_Entry *vid;

   if (!inf->file_cur)
     {
        elm_win_title_set(win, "Rage");
        return;
     }
   vid = inf->file_cur->data;
   file = vid->file;
   if (!file)
     {
        elm_win_title_set(win, "Rage");
        return;
     }
   s = video_title_get(inf->vid);
   if (s)
     {
        snprintf(buf, sizeof(buf), "Rage: %s", s);
        elm_win_title_set(win, buf);
     }
   else
     {
        s = ecore_file_file_get(vid->uri ? vid->uri->path : file);
        if ((s) && (s[0] != 0))
          snprintf(buf, sizeof(buf), "Rage: %s", s);
        else
          snprintf(buf, sizeof(buf), "Rage: %s", file);
        elm_win_title_set(win, buf);
     }
}

void
win_art(Evas_Object *win, const char *path)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (!path)
     {
        elm_layout_signal_emit(inf->lay, "state,noart", "rage");
        if (inf->artimg)
          {
             evas_object_del(inf->artimg);
             inf->artimg = NULL;
          }
     }
   else
     {
        int iw, ih;

        if (inf->artimg)
          {
             evas_object_del(inf->artimg);
             inf->artimg = NULL;
          }
        inf->artimg = evas_object_image_filled_add(evas_object_evas_get(win));
        evas_object_image_file_set(inf->artimg, path, NULL);
        evas_object_image_size_get(inf->artimg, &iw, &ih);
        if ((iw > 0) && (ih > 0))
          {
             evas_object_size_hint_aspect_set(inf->artimg,
                                              EVAS_ASPECT_CONTROL_NEITHER,
                                              iw, ih);
             elm_object_part_content_set(inf->lay, "rage.art", inf->artimg);
             elm_layout_signal_emit(inf->lay, "state,art", "rage");
          }
        else
          {
             evas_object_del(inf->artimg);
             inf->artimg = NULL;
          }
     }
}

void
win_show(Evas_Object *win, int w, int h)
{
   Inf *inf = evas_object_data_get(win, "inf");
   if (!inf->sized)
     {
        inf->sized = EINA_TRUE;
        evas_object_resize(win, w, h);
        if (inf->show_timeout)
          {
             ecore_timer_del(inf->show_timeout);
             inf->show_timeout = NULL;
          }
        evas_object_show(win);
     }
   if (inf->albumart_timeout) ecore_timer_del(inf->albumart_timeout);
   inf->albumart_timeout = ecore_timer_add(0.2, _cb_albumart_delay, win);

   if (!video_has_video_get(inf->vid))
     elm_layout_signal_emit(inf->lay, "state,novideo", "rage");
   else
     elm_layout_signal_emit(inf->lay, "state,video", "rage");
   if (!video_has_audio_get(inf->vid))
     elm_layout_signal_emit(inf->lay, "state,noaudio", "rage");
   else
     elm_layout_signal_emit(inf->lay, "state,audio", "rage");
}

void
win_aspect_adjust(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   int w = 1, h = 1;

   video_ratio_size_get(inf->vid, &w, &h);
   if (inf->zoom_mode == 1)
     evas_object_size_hint_aspect_set(inf->vid, EVAS_ASPECT_CONTROL_NEITHER,
                                      w, h);
   else
     evas_object_size_hint_aspect_set(inf->vid, EVAS_ASPECT_CONTROL_BOTH,
                                      w, h);
   if (((w > 1) && (h > 1)) ||
       ((!video_has_video_get(inf->vid)) && (video_has_audio_get(inf->vid))))
     {
        if ((!video_has_video_get(inf->vid)) &&
            (video_has_audio_get(inf->vid)))
          {
             Evas_Coord mw = 1, mh = 1;

             elm_layout_signal_emit(inf->lay, "pref,size,on", "rage");
             edje_object_message_signal_process(elm_layout_edje_get(inf->lay));
             edje_object_size_min_calc(elm_layout_edje_get(inf->lay), &mw, &mh);
             elm_layout_signal_emit(inf->lay, "pref,size,off", "rage");
             w = mw;
             h = mh;
          }
        win_show(win, w, h);
     }
}

void
win_frame_decode(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   controls_update(inf->lay, inf->vid);
}
