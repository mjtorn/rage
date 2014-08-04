#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "video.h"
#include "winlist.h"
#include "winvid.h"
#include "videothumb.h"

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
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_length(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_frame_decode(data);
   win_title_update(data);
}

static void
_cb_title(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_audio(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_channels(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_play_start(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   win_aspect_adjust(data);
   win_frame_decode(data);
   win_title_update(data);
   inf->playing = EINA_TRUE;
}

static void
_cb_play_finish(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");
   if (!inf->playing) win_show(data, 160, 160);
   inf->playing = EINA_FALSE;
}

static void
_cb_button_num(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_button(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   win_title_update(data);
}

static void
_cb_vidthumb_loaded(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
}

static void
_cb_vidthumb_failed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
}

static void
_cb_vidthumb_data(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   int w, h;
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
   videothumb_size_get(inf->vidthumb, &w, &h);
   w = ((double)w * elm_config_scale_get()) / 2.0;
   h = ((double)h * elm_config_scale_get()) / 2.0;
   evas_object_size_hint_min_set(inf->vidthumb, w, h);
}

static void
_cb_layout_message(void *data, Evas_Object *obj EINA_UNUSED, Edje_Message_Type type, int id, void *msg)
{
   Inf *inf = evas_object_data_get(data, "inf");

   if (!inf) return;
   if (type == EDJE_MESSAGE_FLOAT)
     {
        if (id == 10)
          {
             Edje_Message_Float *m = msg;
             double len = video_length_get(inf->vid);
             video_position_set(inf->vid, len * m->val);
          }
        else if (id == 13)
          {
             Edje_Message_Float *m = msg;
             double len = video_length_get(inf->vid);
             videothumb_file_set(inf->vidthumb, video_file_get(inf->vid),
                                 len * m->val);
          }
     }
}

void
win_video_init(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Evas_Object *o;
   Evas_Coord sz;

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

   o = videothumb_add(win);
   inf->vidthumb = o;
   evas_object_smart_callback_add(o, "loaded", _cb_vidthumb_loaded, win);
   evas_object_smart_callback_add(o, "failed", _cb_vidthumb_failed, win);
   evas_object_smart_callback_add(o, "data", _cb_vidthumb_data, win);
   sz = 80;
   sz = (double)sz * elm_config_scale_get();
   evas_object_size_hint_min_set(o, sz, sz);
   elm_object_part_content_set(inf->lay, "rage.dragable.content", o);
   evas_object_show(o);
   edje_object_message_handler_set(elm_layout_edje_get(inf->lay),
                                   _cb_layout_message, win);
}

void
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

void
win_video_insert(Evas_Object *win, const char *file)
{
   Inf *inf = evas_object_data_get(win, "inf");

   inf->file_list = eina_list_append_relative_list
     (inf->file_list, eina_stringshare_add(file), inf->file_cur);
   evas_object_data_set(win, "file_list", inf->file_list);
}

void
win_video_goto(Evas_Object *win, Eina_List *l)
{
   Inf *inf = evas_object_data_get(win, "inf");

   inf->file_cur = l;
   win_video_restart(win);
   win_list_sel_update(win);
}
