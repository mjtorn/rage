#include <Elementary.h>
#include <Emotion.h>
#include "video.h"
#include "rage_config.h"

typedef struct _Video Video;

struct _Video
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Evas_Object *clip, *o_vid, *o_event;
   Ecore_Timer *smooth_timer;
   Ecore_Job *restart_job;
   const char *file;
   int w, h;
   int iw, ih, piw, pih;
   int resizes;
   struct {
      Evas_Coord x, y;
      Eina_Bool down : 1;
   } down;
   Eina_Bool nosmooth : 1;
   Eina_Bool lowqual : 1;
   Eina_Bool loop : 1;
   Eina_Bool fill : 1;
};

static Evas_Smart *_smart = NULL;
static Evas_Smart_Class _parent_sc = EVAS_SMART_CLASS_INIT_NULL;

static void _ob_resize(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);

static void
_cb_vid_frame(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return;
   evas_object_geometry_get(data, &ox, &oy, &ow, &oh);
   evas_object_show(sd->o_vid);
   evas_object_show(sd->clip);
   _ob_resize(data, ox, oy, ow, oh);
   evas_object_smart_callback_call(data, "frame_decode", NULL);
}

static void
_cb_vid_resize(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return;
   evas_object_geometry_get(data, &ox, &oy, &ow, &oh);
   _ob_resize(data, ox, oy, ow, oh);
}

static void
_cb_vid_len_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
}

static void
_cb_restart(void *data)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->restart_job = NULL;
   emotion_object_position_set(sd->o_vid, 0.0);
   emotion_object_play_set(sd->o_vid, EINA_TRUE);
}

static void
_cb_vid_stop(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   if (sd->restart_job) ecore_job_del(sd->restart_job);
   if (sd->loop)
     {
        sd->restart_job = ecore_job_add(_cb_restart, data);
        evas_object_smart_callback_call(data, "loop", NULL);
     }
   else
     {
        sd->restart_job = NULL;
        evas_object_smart_callback_call(data, "stop", NULL);
     }
}

static void
_cb_vid_progress(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "frame_decode", NULL);
}

static void
_cb_vid_ref(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "frame_decode", NULL);
}

static void
_cb_open_done(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "opened", NULL);
}

static void
_cb_position_update(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "frame_decode", NULL);
}

static void
_cb_length_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "length", NULL);
}

static void
_cb_title_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "title", NULL);
}

static void
_cb_audio_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "audio", NULL);
}

static void
_cb_channels_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "channels", NULL);
}

static void
_cb_play_start(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "play_start", NULL);
}

static void
_cb_play_finish(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "play_finish", NULL);
}

static void
_cb_button_num_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "button_num", NULL);
}

static void
_cb_button_change(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   evas_object_smart_callback_call(data, "button", NULL);
}

static void
_ob_resize(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   emotion_object_size_get(sd->o_vid, &(sd->iw), &(sd->ih));
   if (!sd->fill)
     {
        if ((w <= 0) || (h <= 0) || (sd->iw <= 0) || (sd->ih <= 0))
          {
             w = 1;
             h = 1;
          }
        else
          {
             int iw = 1, ih = 1;
             double ratio;
             
             ratio = emotion_object_ratio_get(sd->o_vid);
             if (ratio > 0.0) sd->iw = (sd->ih * ratio);
             else ratio = (double)sd->iw / (double)sd->ih;
             
             iw = w;
             ih = ((double)w + 1.0) / ratio;
             if (ih > h)
               {
                  ih = h;
                  iw = h * ratio;
                  if (iw > w) iw = w;
               }
             x += ((w - iw) / 2);
             y += ((h - ih) / 2);
             w = iw;
             h = ih;
          }
     }
   evas_object_move(sd->o_vid, x, y);
   evas_object_resize(sd->o_vid, w, h);
   if ((sd->piw != sd->iw) || (sd->pih != sd->ih))
     {
        sd->piw = sd->iw;
        sd->pih = sd->ih;
        evas_object_smart_callback_call(obj, "frame_resize", NULL);
     }
}

static void _smart_calculate(Evas_Object *obj);

static void
_smart_add(Evas_Object *obj)
{
   Video *sd;
   Evas_Object *o;

   sd = calloc(1, sizeof(Video));
   EINA_SAFETY_ON_NULL_RETURN(sd);
   evas_object_smart_data_set(obj, sd);

   _parent_sc.add(obj);

   o = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(o, obj);
   sd->clip = o;
   evas_object_color_set(o, 255, 255, 255, 255);
}

static void
_smart_del(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->file) eina_stringshare_del(sd->file);
   if (sd->clip) evas_object_del(sd->clip);
   if (sd->o_vid) evas_object_del(sd->o_vid);
   if (sd->o_event) evas_object_del(sd->o_event);
   if (sd->smooth_timer) sd->smooth_timer = ecore_timer_del(sd->smooth_timer);
   if (sd->restart_job) ecore_job_del(sd->restart_job);

   _parent_sc.del(obj);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Video *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;
   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   evas_object_smart_changed(obj);
   evas_object_resize(sd->clip, ow, oh);
}

static Eina_Bool
_unsmooth_timeout(void *data)
{
   Video *sd = evas_object_smart_data_get(data);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return EINA_FALSE;
   evas_object_geometry_get(data, &ox, &oy, &ow, &oh);
   sd->smooth_timer = NULL;
   sd->nosmooth = EINA_FALSE;
   emotion_object_smooth_scale_set(sd->o_vid,
                                   (!sd->nosmooth) & (!sd->lowqual));
   return EINA_FALSE;
}

static void
_smooth_handler(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   double interval;
   
   if (!sd) return;
   interval = ecore_animator_frametime_get();
   if (interval <= 0.0) interval = 1.0/60.0;
   if (!sd->nosmooth)
     {
        if (sd->resizes >= 2)
          {
             sd->nosmooth = EINA_TRUE;
             sd->resizes = 0;
             emotion_object_smooth_scale_set(sd->o_vid,
                                             (!sd->nosmooth) & (!sd->lowqual));
             if (sd->smooth_timer)
               sd->smooth_timer = ecore_timer_del(sd->smooth_timer);
             sd->smooth_timer = ecore_timer_add(interval * 10,
                                                _unsmooth_timeout, obj);
          }
     }
   else
     {
        if (sd->smooth_timer)
          sd->smooth_timer = ecore_timer_del(sd->smooth_timer);
        sd->smooth_timer = ecore_timer_add(interval * 10,
                                           _unsmooth_timeout, obj);
     }
}

static void
_smart_calculate(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   if ((ow != sd->w) || (oh != sd->h)) sd->resizes++;
   else sd->resizes = 0;
   _smooth_handler(obj);
   sd->w = ow;
   sd->h = oh;
   _ob_resize(obj, ox, oy, ow, oh);
   evas_object_move(sd->clip, ox, oy);
   evas_object_resize(sd->clip, ow, oh);
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_smart_changed(obj);
}

static void
_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Event_Mouse_Down *ev = event;
   Video *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   
   if (sd->down.down) return;
   if (ev->button != 1) return;
   sd->down.x = ev->canvas.x;
   sd->down.y = ev->canvas.y;
   sd->down.down = EINA_TRUE;
}

static void
_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Event_Mouse_Down *ev = event;
   Video *sd = evas_object_smart_data_get(data);
   Evas_Coord dx, dy;
   if (!sd) return;
   
   if (!sd->down.down) return;
   sd->down.down = EINA_FALSE;
   dx = abs(ev->canvas.x - sd->down.x);
   dy = abs(ev->canvas.y - sd->down.y);
   if ((dx <= elm_config_finger_size_get()) &&
       (dy <= elm_config_finger_size_get()))
     evas_object_smart_callback_call(data, "clicked", NULL);
}

static void
_smart_init(void)
{
   static Evas_Smart_Class sc;

   evas_object_smart_clipped_smart_set(&_parent_sc);
   sc           = _parent_sc;
   sc.name      = "video";
   sc.version   = EVAS_SMART_CLASS_VERSION;
   sc.add       = _smart_add;
   sc.del       = _smart_del;
   sc.resize    = _smart_resize;
   sc.move      = _smart_move;
   sc.calculate = _smart_calculate;
   _smart = evas_smart_class_new(&sc);
}

Evas_Object *
video_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj, *o;
   Video *sd;
   char *modules[] =
     {
        NULL,
        "gstreamer",
        "xine",
        "vlc",
        "gstreamer1"
     };
   char *mod = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;

   if (!_smart) _smart_init();
   obj = evas_object_smart_add(e, _smart);
   sd = evas_object_smart_data_get(obj);
   if (!sd) return obj;

   emotion_init();
   o = sd->o_vid = emotion_object_add(evas_object_evas_get(obj));
   emotion_object_keep_aspect_set(o, EMOTION_ASPECT_KEEP_NONE);
   mod = modules[4];
   if (!emotion_object_init(o, mod))
     {
        evas_object_del(sd->o_vid);
        sd->o_vid = NULL;
        return obj;
     }
   evas_object_smart_callback_add(o, "frame_decode", _cb_vid_frame, obj);
   evas_object_smart_callback_add(o, "frame_resize", _cb_vid_resize, obj);
   evas_object_smart_callback_add(o, "length_change", _cb_vid_len_change, obj);
   evas_object_smart_callback_add(o, "decode_stop", _cb_vid_stop, obj);
   evas_object_smart_callback_add(o, "progress_change", _cb_vid_progress, obj);
   evas_object_smart_callback_add(o, "ref_change", _cb_vid_ref, obj);
   evas_object_smart_callback_add(o, "open_done", _cb_open_done, obj);
   evas_object_smart_callback_add(o, "position_update", _cb_position_update, obj);
   evas_object_smart_callback_add(o, "length_change", _cb_length_change, obj);
   evas_object_smart_callback_add(o, "title_change", _cb_title_change, obj);
   evas_object_smart_callback_add(o, "audio_level_change", _cb_audio_change, obj);
   evas_object_smart_callback_add(o, "channels_change", _cb_channels_change, obj);
   evas_object_smart_callback_add(o, "playback_started", _cb_play_start, obj);
   evas_object_smart_callback_add(o, "playback_finished", _cb_play_finish, obj);
   evas_object_smart_callback_add(o, "button_num_change", _cb_button_num_change, obj);
   evas_object_smart_callback_add(o, "button_change", _cb_button_change, obj);
   evas_object_smart_member_add(o, obj);
   evas_object_clip_set(o, sd->clip);
   evas_object_raise(sd->o_event);

   sd->o_event = evas_object_rectangle_add(e);
   evas_object_color_set(sd->o_event, 0, 0, 0, 0);
   evas_object_repeat_events_set(sd->o_event, EINA_TRUE);
   evas_object_smart_member_add(sd->o_event, obj);
   evas_object_clip_set(sd->o_event, sd->clip);
   evas_object_show(sd->o_event);
   evas_object_event_callback_add(sd->o_event, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, obj);
   evas_object_event_callback_add(sd->o_event, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, obj);
   return obj;
}

void
video_file_set(Evas_Object *obj, const char *file)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->o_vid);
   evas_object_hide(sd->clip);
   eina_stringshare_replace(&(sd->file), file);
   emotion_object_file_set(sd->o_vid, sd->file);
   video_position_set(obj, 0.0);
}

void
video_mute_set(Evas_Object *obj, Eina_Bool mute)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_audio_mute_set(sd->o_vid, mute);
}

Eina_Bool
video_mute_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return emotion_object_audio_mute_get(sd->o_vid);
}

void
video_play_set(Evas_Object *obj, Eina_Bool play)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_play_set(sd->o_vid, play);
}

Eina_Bool
video_play_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return emotion_object_play_get(sd->o_vid);
}

void
video_loop_set(Evas_Object *obj, Eina_Bool loop)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->loop = loop;
}

Eina_Bool
video_loop_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return sd->loop;
}

void
video_fill_set(Evas_Object *obj, Eina_Bool fill)
{
   Evas_Coord ox, oy, ow, oh;
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->fill = fill;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   _ob_resize(obj, ox, oy, ow, oh);
}

Eina_Bool
video_fill_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return sd->fill;
}

void
video_stop(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->o_vid);
   evas_object_hide(sd->clip);
   emotion_object_play_set(sd->o_vid, EINA_FALSE);
   emotion_object_position_set(sd->o_vid, 0.0);
}

void
video_position_set(Evas_Object *obj, double pos)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_position_set(sd->o_vid, pos);
}

double
video_position_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0.0;
   return emotion_object_position_get(sd->o_vid);
}

double
video_length_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0.0;
   return emotion_object_play_length_get(sd->o_vid);
}

void
video_ratio_size_get(Evas_Object *obj, int *w, int *h)
{
   Video *sd = evas_object_smart_data_get(obj);
   *w = 1;
   *h = 1;
   if (!sd) return;
   emotion_object_size_get(sd->o_vid, &(sd->iw), &(sd->ih));
   if ((sd->iw <= 0) || (sd->ih <= 0))
     {
        *w = 1;
        *h = 1;
     }
   else
     {
        double ratio;
        
        ratio = emotion_object_ratio_get(sd->o_vid);
        if (ratio > 0.0) sd->iw = (sd->ih * ratio) + 0.5;
        else ratio = (double)sd->iw / (double)sd->ih;
        *w = sd->iw;
        *h = sd->ih;
     }
}

void
video_eject(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_eject(sd->o_vid);
}

int
video_chapter_count(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_chapter_count(sd->o_vid);
}

void
video_chapter_set(Evas_Object *obj, int chapter)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_chapter_set(sd->o_vid, chapter);
}

int
video_chapter_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_chapter_get(sd->o_vid);
}

const char *
video_chapter_name_get(Evas_Object *obj, int chapter)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return emotion_object_chapter_name_get(obj, chapter);
}

void
video_volume_set(Evas_Object *obj, double vol)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_audio_volume_set(sd->o_vid, vol);
}

double
video_volume_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0.0;
   return emotion_object_audio_volume_get(sd->o_vid);
}

Eina_Bool
video_has_video_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return emotion_object_video_handled_get(sd->o_vid);
}

Eina_Bool
video_has_audio_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return emotion_object_audio_handled_get(sd->o_vid);
}

const char *
video_title_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return emotion_object_title_get(obj);
}

int
video_audio_channel_count(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_audio_channel_count(sd->o_vid);
}

void
video_audio_channel_set(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_audio_channel_set(sd->o_vid, chan);
}

int
video_audio_channel_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_audio_channel_get(sd->o_vid);
}

const char *
video_audio_channel_name_get(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return emotion_object_audio_channel_name_get(obj, chan);
}

int
video_video_channel_count(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_video_channel_count(sd->o_vid);
}

void
video_video_channel_set(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_video_channel_set(sd->o_vid, chan);
}

int
video_video_channel_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_video_channel_get(sd->o_vid);
}

const char *
video_video_channel_name_get(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return emotion_object_video_channel_name_get(obj, chan);
}

int
video_spu_channel_count(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_spu_channel_count(sd->o_vid);
}

void
video_spu_channel_set(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_spu_channel_set(sd->o_vid, chan);
}

int
video_spu_channel_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_spu_channel_get(sd->o_vid);
}

const char *
video_spu_channel_name_get(Evas_Object *obj, int chan)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   return emotion_object_spu_channel_name_get(obj, chan);
}

int
video_spu_button_count(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_spu_button_count_get(sd->o_vid);
}

int
video_spu_button_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return emotion_object_spu_button_get(sd->o_vid);
}

void
video_event_send(Evas_Object *obj, Emotion_Event ev)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   emotion_object_event_simple_send(sd->o_vid, ev);
}

void
video_lowquality_set(Evas_Object *obj, Eina_Bool lowq)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->lowqual = lowq;
   emotion_object_smooth_scale_set(sd->o_vid, 
                                   (!sd->nosmooth) & (!sd->lowqual));
}

Eina_Bool
video_lowquality_get(Evas_Object *obj)
{
   Video *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   return sd->lowqual;
}


// emotion_object_seekable_get
// emotion_object_play_speed_set
// emotion_object_play_speed_get
// emotion_object_meta_info_get
// emotion_object_buffer_size_get
// emotion_object_video_mute_set
// emotion_object_video_mute_get
// emotion_object_video_subtitle_file_set
// emotion_object_spu_mute_set
// emotion_object_spu_mute_get
// emotion_object_progress_info_get
// emotion_object_progress_status_get
// emotion_object_ref_file_get
