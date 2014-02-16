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

static void
_cb_fullscreen(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   elm_layout_signal_emit(inf->lay, "state,win,fullscreen", "rage");
}

static void
_cb_unfullscreen(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   elm_layout_signal_emit(inf->lay, "state,win,normal", "rage");
}

static void
_cb_win_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *ev EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   const char *f;
   
   if (inf->next_job) ecore_job_del(inf->next_job);
   if (inf->show_timeout) ecore_timer_del(inf->show_timeout);
   if (inf->drag_anim) ecore_animator_del(inf->drag_anim);
   EINA_LIST_FREE(inf->file_list, f) eina_stringshare_del(f);
   evas_object_data_del(obj, "inf");
   free(inf);
}

static void
_cb_key_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   key_handle(data, event_info);
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
_restart_vid(Evas_Object *win, Evas_Object *lay, Evas_Object *vid, const char *file)
{
   video_position_set(vid, 0.0);
   video_play_set(vid, EINA_FALSE);
   video_file_set(vid, file);
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
   _restart_vid(win, inf->lay, inf->vid, inf->file_cur->data);
}

void
win_video_next(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;

   if (!inf->file_list) return;
   if (!inf->file_cur) l = inf->file_list;
   else l = inf->file_cur->next;
   if (!l)
     {
        elm_exit();
        return;
     }
   inf->file_cur = l;
   _restart_vid(win, inf->lay, inf->vid, l->data);
   win_list_sel_update(win);
}

void
win_video_prev(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   
   if (!inf->file_list) return;
   if (!inf->file_cur) l = eina_list_last(inf->file_list);
   else l = inf->file_cur->prev;
   if (!l) return;
   inf->file_cur = l;
   _restart_vid(win, inf->lay, inf->vid, l->data);
   win_list_sel_update(win);
}

void
win_video_first(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   
   if (!inf->file_list) return;
   l = inf->file_list;
   inf->file_cur = l;
   _restart_vid(win, inf->lay, inf->vid, l->data);
   win_list_sel_update(win);
}

void
win_video_last(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   
   if (!inf->file_list) return;
   l = eina_list_last(inf->file_list);
   if (!l) return;
   inf->file_cur = l;
   _restart_vid(win, inf->lay, inf->vid, l->data);
   win_list_sel_update(win);
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
   if (!win) return NULL;
   
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
   elm_win_resize_object_add(win, o);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, EINA_TRUE);
   evas_object_show(o);
   inf->event = o;
   dnd_init(win, o);
   gesture_init(win, o);

   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN,
                                  _cb_key_down, win);
   return win;
}

void
win_title_update(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   const char *file, *s;
   char buf[4096];
   
   if (!inf->file_cur)
     {
        elm_win_title_set(win, "Rage");
        return;
     }
   file = inf->file_cur->data;
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
        s = ecore_file_file_get(file);
        if ((s) && (s[0] != 0))
          snprintf(buf, sizeof(buf), "Rage: %s", s);
        else
          snprintf(buf, sizeof(buf), "Rage: %s", file);
        elm_win_title_set(win, buf);
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
          w = h = 160;
        win_show(win, w, h);
     }
}

void
win_frame_decode(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   
   controls_update(inf->lay, inf->vid);
}
