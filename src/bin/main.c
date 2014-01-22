#include <Elementary.h>
#include "main.h"
#include "video.h"

typedef struct _Inf Inf;

struct _Inf
{
   Evas_Object *vid, *lay, *event;
   Eina_List *file_list, *file_cur;
   Ecore_Job *next_job;
   Ecore_Timer *show_timeout;
   double last_action;
   double jump;
   int zoom_mode;
   Eina_Bool sized : 1;
   Eina_Bool last_action_rwind : 1;
   Eina_Bool playing : 1;
};

static void win_video_next(Evas_Object *win);
static void win_video_insert(Evas_Object *win, const char *file);
static Eina_Bool win_video_have_next(Evas_Object *win);
static Eina_Bool win_video_have_prev(Evas_Object *win);
static void win_video_prev(Evas_Object *win);

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
_cb_drag_enter(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
   printf("dnd enter\n");
}

static void
_cb_drag_leave(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
   printf("dnd leave\n");
}

static void
_cb_drag_pos(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action)
{
   printf("dnd at %i %i act:%i\n", x, y, action);
}

static Eina_Bool
_cb_drop(void *data, Evas_Object *o EINA_UNUSED, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   Eina_Bool inserted = EINA_FALSE;

   if (!ev->data) return EINA_TRUE;
   if (strchr(ev->data, '\n'))
     {
        char *p, *p2, *p3, *tb;
        
        tb = malloc(strlen(ev->data) + 1);
        if (tb)
          {
             for (p = ev->data; p;)
               {
                  p2 = strchr(p, '\n');
                  p3 = strchr(p, '\r');
                  if (p2 && p3)
                    {
                       if (p3 < p2) p2 = p3;
                    }
                  else if (!p2) p3 = p2;
                  if (p2)
                    {
                       strncpy(tb, p, p2 - p);
                       tb[p2 - p] = 0;
                       p = p2;
                       while ((*p) && (isspace(*p))) p++;
                       if (strlen(tb) > 0)
                         {
                            win_video_insert(win, tb);
                            inserted = EINA_TRUE;
                         }
                    }
                  else
                    {
                       strcpy(tb, p);
                       if (strlen(tb) > 0)
                         {
                            win_video_insert(win, tb);
                            inserted = EINA_TRUE;
                         }
                       break;
                    }
               }
             free(tb);
          }
     }
   else
     {
        win_video_insert(win, ev->data);
        inserted = EINA_TRUE;
     }
   if (inserted) win_video_next(win);
   return EINA_TRUE;
}

static void
_cb_win_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *ev EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(obj, "inf");
   const char *f;
   
   if (inf->next_job) ecore_job_del(inf->next_job);
   if (inf->show_timeout) ecore_timer_del(inf->show_timeout);
   EINA_LIST_FREE(inf->file_list, f) eina_stringshare_del(f);
   free(inf);
   evas_object_data_del(obj, "inf");
}

static void
win_do_play(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, EINA_TRUE);
   elm_layout_signal_emit(inf->lay, "action,play", "rage");
}

static void
win_do_pause(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, EINA_FALSE);
   elm_layout_signal_emit(inf->lay, "action,pause", "rage");
}

static void
win_do_play_pause(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   video_play_set(inf->vid, !video_play_get(inf->vid));
   if (video_play_get(inf->vid))
     elm_layout_signal_emit(inf->lay, "action,play", "rage");
   else
     elm_layout_signal_emit(inf->lay, "action,pause", "rage");
}

static void
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

static void
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
_cb_drag(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   double pos = 0.0;
   
   edje_object_part_drag_value_get(elm_layout_edje_get(inf->lay),
                                   "rage.dragable.pos", &pos, NULL);
   video_position_set(inf->vid, pos * video_length_get(inf->vid));
}

static void
_cb_pos_prev(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_prev(data);
}

static void
_cb_pos_next(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_next(data);
}

static void
_cb_pos_play(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_play(data);
}

static void
_cb_pos_pause(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   win_do_pause(data);
}

static void
_cb_options(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
//   win_do_options(data);
}

static Evas_Object *
win_add(void)
{
   Evas_Object *win, *o;
   char buf[4096];
   Evas_Coord sz;
   Inf *inf = calloc(1, sizeof(Inf));
   
   win = elm_win_add(NULL, "Rage", ELM_WIN_BASIC);
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
   
   elm_layout_signal_callback_add(o, "drag", "rage.dragable.pos",
                                  _cb_drag, win);
   elm_layout_signal_callback_add(o, "pos,action,prev", "rage",
                                  _cb_pos_prev, win);
   elm_layout_signal_callback_add(o, "pos,action,next", "rage",
                                  _cb_pos_next, win);
   elm_layout_signal_callback_add(o, "pos,action,play", "rage",
                                  _cb_pos_play, win);
   elm_layout_signal_callback_add(o, "pos,action,pause", "rage",
                                  _cb_pos_pause, win);
   elm_layout_signal_callback_add(o, "pos,action,options", "rage",
                                  _cb_options, win);

   sz = 0;
   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   
#define FINGER_SIZE(_nam) \
   o = evas_object_rectangle_add(evas_object_evas_get(win)); \
   evas_object_color_set(o, 0, 0, 0, 0); \
   evas_object_pass_events_set(o, EINA_TRUE); \
   evas_object_size_hint_min_set(o, sz, sz); \
   elm_object_part_content_set(inf->lay, _nam, o)
   
   FINGER_SIZE("rage.pos.swallow");
   FINGER_SIZE("rage.vol.swallow");
   FINGER_SIZE("rage.options.swallow");
   FINGER_SIZE("rage.pos.prev.swallow");
   FINGER_SIZE("rage.pos.play.swallow");
   FINGER_SIZE("rage.pos.next.swallow");
   
   o = evas_object_rectangle_add(evas_object_evas_get(win));
   elm_win_resize_object_add(win, o);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, EINA_TRUE);
   evas_object_show(o);
   inf->event = o;

   elm_drop_target_add(o, ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE,
                       _cb_drag_enter, win,
                       _cb_drag_leave, win,
                       _cb_drag_pos, win,
                       _cb_drop, win);
   return win;
}

static void
win_title_update(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   const char *file;
   
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
   if (video_title_get(inf->vid))
     elm_win_title_set(win, video_title_get(inf->vid));
   else
     elm_win_title_set(win, ecore_file_file_get(file));
}

static void
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

static void
win_aspect_adjust(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   int w = 1, h = 1;
   
   video_ratio_size_get(inf->vid, &w, &h);
   if (inf->zoom_mode == 1)
     evas_object_size_hint_aspect_set(inf->vid, EVAS_ASPECT_CONTROL_NEITHER, w, h);
   else
     evas_object_size_hint_aspect_set(inf->vid, EVAS_ASPECT_CONTROL_BOTH, w, h);
   if (((w > 1) && (h > 1)) ||
       ((!video_has_video_get(inf->vid)) && (video_has_audio_get(inf->vid))))
     {
        if ((!video_has_video_get(inf->vid)) && (video_has_audio_get(inf->vid)))
          w = h = 160;
        win_show(win, w, h);
     }
}

static void
_time_print(double t, double max, int size, char *buf, Eina_Bool subsec)
{
   double tsf;
   long long tl;
   int ts, tm, th;

   tl = t;
   ts = tl % 60;
   tm = (tl / 60) % 60;
   th = (tl / (60 * 60));
   tsf = fmod(t, 60.0);
   if (subsec)
     {
        if (tsf < 10.0)
          {
             if (max >= (60 * 60))
               snprintf(buf, size, "%i:%02i:0%1.02f", th, tm, tsf);
             else if (max >= 60)
               snprintf(buf, size, "%i:0%1.02f", tm, tsf);
             else
               snprintf(buf, size, "%1.02f", tsf);
          }
        else
          {
             if (max >= (60 * 60))
               snprintf(buf, size, "%i:%02i:%2.02f", th, tm, tsf);
             else if (max >= 60)
               snprintf(buf, size, "%i:%2.02f", tm, tsf);
             else
               snprintf(buf, size, "%2.02f", tsf);
          }
     }
   else
     {
        if (max >= (60 * 60))
          snprintf(buf, size, "%i:%02i:%02i", th, tm, ts);
        else if (max >= 60)
          snprintf(buf, size, "%i:%02i", tm, ts);
        else
          snprintf(buf, size, "%i", ts);
     }
}

static void
win_frame_decode(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   char buf[256];
   double p;

   _time_print(video_position_get(inf->vid), video_length_get(inf->vid),
               sizeof(buf), buf, EINA_TRUE);
   elm_object_part_text_set(inf->lay, "rage.pos", buf);
   _time_print(video_length_get(inf->vid), video_length_get(inf->vid),
               sizeof(buf), buf, EINA_FALSE);
   elm_object_part_text_set(inf->lay, "rage.length", buf);
   elm_layout_signal_emit(inf->lay, "action,frame", "rage");
   if (video_length_get(inf->vid) > 0.0)
     p = video_position_get(inf->vid) / video_length_get(inf->vid);
   else p = 0.0;
   edje_object_part_drag_value_set(elm_layout_edje_get(inf->lay),
                                   "rage.dragable.pos", p, 0.0);
}

static void
_cb_resize(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_aspect_adjust(data);
   win_title_update(data);
}

static void
_cb_decode(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_frame_decode(data);
}

static void
win_video_next(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   const char *file;

   if (!inf->file_list) return;
   if (!inf->file_cur) l = inf->file_list;
   else l = inf->file_cur->next;
   if (!l)
     {
        elm_exit();
        return;
     }
   file = l->data;
   inf->file_cur = l;
   video_position_set(inf->vid, 0.0);
   video_play_set(inf->vid, EINA_FALSE);
   video_file_set(inf->vid, file);
   video_position_set(inf->vid, 0.0);
   video_play_set(inf->vid, EINA_TRUE);
   elm_layout_signal_emit(inf->lay, "action,newvid", "rage");
   win_aspect_adjust(win);
   win_title_update(win);
}

static void
win_video_prev(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   const char *file;
   
   if (!inf->file_list) return;
   if (!inf->file_cur) l = eina_list_last(inf->file_list);
   else l = inf->file_cur->prev;
   if (!l) return;
   file = l->data;
   inf->file_cur = l;
   video_position_set(inf->vid, 0.0);
   video_play_set(inf->vid, EINA_FALSE);
   video_file_set(inf->vid, file);
   video_position_set(inf->vid, 0.0);
   video_play_set(inf->vid, EINA_TRUE);
   elm_layout_signal_emit(inf->lay, "action,newvid", "rage");
   win_aspect_adjust(win);
   win_title_update(win);
}

static Eina_Bool
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

static Eina_Bool
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

static void
win_video_insert(Evas_Object *win, const char *file)
{
   Inf *inf = evas_object_data_get(win, "inf");

   inf->file_list = eina_list_append_relative_list
     (inf->file_list, eina_stringshare_add(file), inf->file_cur);
   evas_object_data_set(win, "file_list", inf->file_list);
}

static void
_cb_key_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");
   
   printf("%s | %s\n", ev->key, ev->keyname);
   if ((!strcmp(ev->key, "Left")) ||
       (!strcmp(ev->key, "bracketleft")))
     {
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Left")))
          video_event_send(inf->vid, EMOTION_EVENT_LEFT);
        else
          {
             double pos = video_position_get(inf->vid);
             
             if (!inf->last_action_rwind)
               {
                  double t = ecore_time_get();
                  if ((t - inf->last_action) < 1.0) inf->jump *= 1.2;
                  else inf->jump = -5.0;
                  inf->last_action = t;
               }
             else inf->jump = -5.0;
             if (inf->jump < -300.0) inf->jump = -300.0;
             inf->last_action_rwind = EINA_FALSE;
             pos += inf->jump;
             if (pos < 0.0) pos = 0.0;
             video_position_set(inf->vid, pos);
             elm_layout_signal_emit(inf->lay, "action,rewind", "rage");
          }
     }
   else if ((!strcmp(ev->key, "Right")) ||
            (!strcmp(ev->key, "bracketright")))
     {
        double pos = video_position_get(inf->vid);
        double len = video_length_get(inf->vid);

        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Right")))
          video_event_send(inf->vid, EMOTION_EVENT_RIGHT);
        else
          {
             if (inf->last_action_rwind)
               {
                  double t = ecore_time_get();
                  if ((t - inf->last_action) < 1.0) inf->jump *= 1.2;
                  else inf->jump = 5.0;
                  inf->last_action = t;
               }
             else inf->jump = 5.0;
             if (inf->jump > 300.0) inf->jump = 300.0;
             inf->last_action_rwind = EINA_TRUE;
             pos += inf->jump;
             if (pos < (len - 1.0)) video_position_set(inf->vid, pos);
               elm_layout_signal_emit(inf->lay, "action,forward", "rage");
          }
     }
   else if ((!strcmp(ev->key, "Up")) ||
            (!strcmp(ev->key, "plus")) ||
            (!strcmp(ev->key, "equal")))
     {
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Up")))
          video_event_send(inf->vid, EMOTION_EVENT_UP);
        else
          {
             double vol = video_volume_get(inf->vid) + 0.05;
             if (vol > 1.0) vol = 1.0;
             video_volume_set(inf->vid, vol);
             elm_layout_signal_emit(inf->lay, "action,volume_up", "rage");
          }
     }
   else if ((!strcmp(ev->key, "Down")) ||
            (!strcmp(ev->key, "minus")))
     {
        if ((video_spu_button_count(inf->vid) > 0) &&
            (!strcmp(ev->key, "Down")))
          video_event_send(inf->vid, EMOTION_EVENT_DOWN);
        else
          {
             double vol = video_volume_get(inf->vid) - 0.05;
             if (vol < 0.0) vol = 0.0;
             video_volume_set(inf->vid, vol);
             elm_layout_signal_emit(inf->lay, "action,volume_down", "rage");
          }
     }
   else if ((!strcmp(ev->key, "space")) ||
            (!strcmp(ev->keyname, "p")) ||
            (!strcmp(ev->key, "XF86AudioPlay")))
     {
        printf("ch: %i/%i [%s]\n", video_chapter_get(inf->vid), video_chapter_count(inf->vid), video_chapter_name_get(inf->vid, video_chapter_get(inf->vid)));
        printf("ti: %s\n", video_title_get(inf->vid));
        printf("au: %i/%i [%s]\n", video_audio_channel_get(inf->vid), video_audio_channel_count(inf->vid), video_audio_channel_name_get(inf->vid, video_audio_channel_get(inf->vid)));
        printf("vi: %i/%i [%s]\n", video_video_channel_get(inf->vid), video_video_channel_count(inf->vid), video_video_channel_name_get(inf->vid, video_video_channel_get(inf->vid)));
        printf("sp: %i/%i [%s]\n", video_spu_channel_get(inf->vid), video_spu_channel_count(inf->vid), video_spu_channel_name_get(inf->vid, video_spu_channel_get(inf->vid)));

        win_do_play_pause(win);
     }
   else if ((!strcmp(ev->keyname, "s")) ||
            (!strcmp(ev->key, "XF86AudioStop")))
     {
        video_stop(inf->vid);
        elm_layout_signal_emit(inf->lay, "action,stop", "rage");
     }
   else if ((!strcmp(ev->key, "Prior")) ||
            (!strcmp(ev->key, "XF86AudioPrev")))
     {
        win_do_prev(win);
     }
   else if ((!strcmp(ev->key, "Next")) ||
            (!strcmp(ev->key, "XF86AudioNext")))
     {
        win_do_next(win);
     }
   else if ((!strcmp(ev->keyname, "m")) ||
            (!strcmp(ev->key, "XF86AudioMute")))
     {
        video_mute_set(inf->vid, !video_mute_get(inf->vid));
        if (video_mute_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,mute", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,unmute", "rage");
     }
   else if (!strcmp(ev->keyname, "l"))
     {
        video_loop_set(inf->vid, !video_loop_get(inf->vid));
        if (video_loop_get(inf->vid))
          elm_layout_signal_emit(inf->lay, "action,loop", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,sequential", "rage");
     }
   else if ((!strcmp(ev->keyname, "q")) ||
            (!strcmp(ev->key, "Escape")))
     {
        elm_exit();
     }
   else if ((!strcmp(ev->keyname, "f")) ||
            (!strcmp(ev->key, "F11")))
     {
        elm_win_fullscreen_set(win, !elm_win_fullscreen_get(win));
     }
   else if (!strcmp(ev->keyname, "n"))
     {
        int w, h;
        
        video_ratio_size_get(inf->vid, &w, &h);
        if ((w > 1) && (h > 1)) evas_object_resize(win, w, h);
     }
   else if (!strcmp(ev->keyname, "z"))
     {
        if (inf->zoom_mode == 0) inf->zoom_mode = 1;
        else inf->zoom_mode = 0;
        win_aspect_adjust(win);
        if (inf->zoom_mode == 1)
          elm_layout_signal_emit(inf->lay, "action,zoom_fill", "rage");
        else
          elm_layout_signal_emit(inf->lay, "action,zoom_fit", "rage");
     }
   else if (!strcmp(ev->keyname, "e"))
     {
        video_eject(inf->vid);
        elm_layout_signal_emit(inf->lay, "action,eject", "rage");
     }
   else if ((!strcmp(ev->key, "Return")) ||
            (!strcmp(ev->key, "KP_Enter")))
     {
        video_event_send(inf->vid, EMOTION_EVENT_SELECT);
     }
   else if ((!strcmp(ev->key, "comman")) ||
            (!strcmp(ev->key, "less")))
     {
        video_event_send(inf->vid, EMOTION_EVENT_ANGLE_PREV);
     }
   else if ((!strcmp(ev->key, "period")) ||
            (!strcmp(ev->key, "greater")))
     {
        video_event_send(inf->vid, EMOTION_EVENT_ANGLE_NEXT);
     }
   else if (!strcmp(ev->key, "Tab"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_FORCE);
     }
   else if (!strcmp(ev->key, "0"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_0);
     }
   else if (!strcmp(ev->key, "1"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_1);
     }
   else if (!strcmp(ev->key, "2"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_2);
     }
   else if (!strcmp(ev->key, "3"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_3);
     }
   else if (!strcmp(ev->key, "4"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_4);
     }
   else if (!strcmp(ev->key, "5"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_5);
     }
   else if (!strcmp(ev->key, "6"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_6);
     }
   else if (!strcmp(ev->key, "7"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_7);
     }
   else if (!strcmp(ev->key, "8"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_8);
     }
   else if (!strcmp(ev->key, "9"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_9);
     }
   else if ((!strcmp(ev->key, "grave")) ||
            (!strcmp(ev->key, "asciitilde")))
     {
        video_event_send(inf->vid, EMOTION_EVENT_10);
     }
   else if (!strcmp(ev->key, "F1"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU1);
     }
   else if (!strcmp(ev->key, "F2"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU2);
     }
   else if (!strcmp(ev->key, "F3"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU3);
     }
   else if (!strcmp(ev->key, "F4"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU4);
     }
   else if (!strcmp(ev->key, "F5"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU5);
     }
   else if (!strcmp(ev->key, "F6"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU6);
     }
   else if (!strcmp(ev->key, "F7"))
     {
        video_event_send(inf->vid, EMOTION_EVENT_MENU7);
     }
}

static void
_cb_stop_next(void *data)
{
   Inf *inf = evas_object_data_get(data, "inf");
   inf->next_job = NULL;
   win_video_next(data);
}

static void
_cb_stop(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   printf("stop\n");
   if (win_video_have_next(data))
     {
        if (inf->next_job) ecore_job_del(inf->next_job);
        inf->next_job = ecore_job_add(_cb_stop_next, data);
     }
   else elm_exit();
}

static void
_cb_opened(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("opened\n");
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_length(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("len\n");
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_title(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("title\n");
   win_title_update(data);
}

static void
_cb_audio(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("audio\n");
   win_title_update(data);
}

static void
_cb_channels(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("channels\n");
   win_title_update(data);
}

static void
_cb_play_start(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   printf("play start\n");
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
   inf->playing = EINA_TRUE;
}

static void
_cb_play_finish(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   printf("play finish\n");
   if (!inf->playing) win_show(data, 160, 160);
   inf->playing = EINA_FALSE;
}

static void
_cb_button_num(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("button num\n");
   win_title_update(data);
}

static void
_cb_button(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("button\n");
   win_title_update(data);
}

static void
win_video_init(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Evas_Object *o;
   
   o = video_add(win);
   video_fill_set(o, EINA_TRUE);
   inf->vid = o;
   evas_object_smart_callback_add(o, "frame_resize", _cb_resize, win);
   evas_object_smart_callback_add(o, "frame_decode", _cb_decode, win);
   evas_object_smart_callback_add(o, "stop", _cb_stop, win);
   evas_object_smart_callback_add(o, "opened", _cb_opened, win);
   evas_object_smart_callback_add(o, "length", _cb_length, win);
   evas_object_smart_callback_add(o, "title", _cb_title, win);
   evas_object_smart_callback_add(o, "audio", _cb_audio, win);
   evas_object_smart_callback_add(o, "channels", _cb_channels, win);
   evas_object_smart_callback_add(o, "play_start", _cb_play_start, win);
   evas_object_smart_callback_add(o, "play_finish", _cb_play_finish, win);
   evas_object_smart_callback_add(o, "button_num", _cb_button_num, win);
   evas_object_smart_callback_add(o, "button", _cb_button, win);
   elm_object_part_content_set(inf->lay, "rage.content", o);
   evas_object_show(o);
   
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, _cb_key_down, win);
}

static void
win_video_file_list_set(Evas_Object *win, Eina_List *list)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l, *list2 = NULL;
   const char *f;
   
   EINA_LIST_FOREACH(list, l, f)
     {
        list2 = eina_list_append(list2, eina_stringshare_add(f));
     }
   inf->file_list = list2;
   win_video_next(win);
}

static Eina_Bool
_cb_show_timeout(void *data)
{
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");

   inf->show_timeout = NULL;
   evas_object_show(win);
   return EINA_FALSE;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win;
   char buf[4096];
   const char *f;
   Eina_List *list = NULL;
   int i;
   Inf *inf;
   
   if (argc <= 1)
     {
        printf("Usage: rage {file-name}\n");
        goto end;
     }

   for (i = 1; i < argc; i++)
     {
        list = eina_list_append(list, eina_stringshare_add(argv[i]));
     }
   
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "rage", "themes/default.edj");
   
   snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
   elm_theme_overlay_add(NULL, buf);

   win = win_add();
   evas_object_resize(win, 320, 200);
   
   win_video_init(win);
   win_video_file_list_set(win, list);
   EINA_LIST_FREE(list, f) eina_stringshare_del(f);
   
   inf = evas_object_data_get(win, "inf");
   inf->show_timeout = ecore_timer_add(10.0, _cb_show_timeout, win);
                        
   elm_run();

end:
   elm_shutdown();
   return 0;
}
ELM_MAIN()
