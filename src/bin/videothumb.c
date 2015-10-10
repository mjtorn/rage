#include <Elementary.h>
#include "videothumb.h"
#include "sha1.h"
#include "albumart.h"

typedef struct _Videothumb Videothumb;

struct _Videothumb
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Evas_Object *o_img, *o_img2;
   Ecore_Exe *thumb_exe;
   Ecore_Event_Handler *exe_handler;
   Ecore_Timer *cycle_timer;
   Ecore_Timer *launch_timer;
   const char *file;
   const char *realfile;
   char *realpath;
   double pos;
   unsigned int realpos;
   int iw, ih;
   Evas_Coord w, h;
   Eina_Bool seen : 1;
};

static Evas_Smart *_smart = NULL;
static Evas_Smart_Class _parent_sc = EVAS_SMART_CLASS_INIT_NULL;

static Eina_List *busy_thumbs = NULL;
static Eina_List *vidthumbs = NULL;

static int        _thumb_running = 0;

static Eina_Bool _cb_thumb_exe(void *data, int type EINA_UNUSED, void *event);
static void _videothumb_eval(Evas_Object *obj, Eina_Bool force);
static void _smart_calculate(Evas_Object *obj);

static Eina_Bool
_busy_add(const char *file)
{
   Eina_List *l;
   const char *s;

   EINA_LIST_FOREACH(busy_thumbs, l, s)
     {
        if (!strcmp(file, s)) return EINA_FALSE;
     }
   busy_thumbs = eina_list_append(busy_thumbs, eina_stringshare_add(file));
   return EINA_TRUE;
}

static Eina_Bool
_busy_del(const char *file)
{
   Eina_List *l;
   const char *s;

   EINA_LIST_FOREACH(busy_thumbs, l, s)
     {
        if (!strcmp(file, s))
          {
             eina_stringshare_del(s);
             busy_thumbs = eina_list_remove_list(busy_thumbs, l);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static void
_thumb_update(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   char buf[PATH_MAX];

   if (!sd) return;
   snprintf(buf, sizeof(buf), "%u", sd->realpos);
   evas_object_image_file_set(sd->o_img2, NULL, NULL);
   evas_object_image_file_set(sd->o_img2, sd->realfile, buf);
   evas_object_image_size_get(sd->o_img2, &(sd->iw), &(sd->ih));
   if ((sd->iw <= 0) || (sd->ih <= 0))
     {
        if (sd->cycle_timer)
          {
             sd->pos = 0.0;
             if (!sd->thumb_exe)
               {
                  _videothumb_eval(obj, EINA_TRUE);
               }
          }
        else
          {
             evas_object_del(sd->o_img2);
             sd->o_img2 = NULL;
             evas_object_smart_callback_call(obj, "failed", NULL);
          }
     }
   else
     {
        evas_object_image_preload(sd->o_img, EINA_FALSE);
        evas_object_smart_callback_call(obj, "loaded", NULL);
     }
}

static void
_thumb_match_update(Evas_Object *obj, const char *file)
{
   Videothumb *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   if (!sd->realpath) return;
   if (!strcmp(sd->realpath, file)) _thumb_update(obj);
}

static void
_videothumb_launch_do(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   char buf[PATH_MAX];
   const char *libdir;
   char *s;

   if (!sd) return;
   ecore_exe_run_priority_set(10);
   if (sd->thumb_exe)
     {
        _busy_del(sd->realpath);
        ecore_exe_kill(sd->thumb_exe);
        ecore_exe_free(sd->thumb_exe);
        sd->thumb_exe = NULL;
        _thumb_running--;
     }
   s = ecore_file_escape_name(sd->realpath);
   if (s)
     {
        libdir = elm_app_lib_dir_get();
        if (libdir)
          {
             if (_busy_add(sd->realpath))
               {
                  if (!sd->exe_handler)
                    sd->exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                              _cb_thumb_exe, obj);
                  snprintf(buf, sizeof(buf),
                           "%s/rage/utils/rage_thumb %s 10000 >& /dev/null",
                           libdir, s);
                  sd->thumb_exe = ecore_exe_pipe_run(buf,
                                                     ECORE_EXE_TERM_WITH_PARENT |
                                                     ECORE_EXE_NOT_LEADER,
                                                     obj);
                  _thumb_running++;
               }
             else return;
          }
        free(s);
     }
}

static Eina_Bool
_cb_videothumb_delay(void *data)
{
   Evas_Object *obj = data;
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;
   if (_thumb_running < (eina_cpu_count() + 1))
     {
        sd->launch_timer = NULL;
        _videothumb_launch_do(obj);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void
_videothumb_launch(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);

   if (!sd) return;
   if (sd->launch_timer) return;
   sd->launch_timer = ecore_timer_add(1.0, _cb_videothumb_delay, obj);
}

static Eina_Bool
_cb_thumb_exe(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Videothumb *sd = evas_object_smart_data_get(data);

   if (!sd) return EINA_TRUE;
   ev = event;
   if (ev->exe == sd->thumb_exe)
     {
        Eina_List *l;
        Evas_Object *o;

        _busy_del(sd->realpath);
        sd->thumb_exe = NULL;
        _thumb_running--;
        EINA_LIST_FOREACH(vidthumbs, l, o)
          {
             _thumb_match_update(o, sd->realpath);
          }
        if ((sd->iw <= 0) || (sd->ih <= 0))
          {
             if (sd->exe_handler)
               {
                  ecore_event_handler_del(sd->exe_handler);
                  sd->exe_handler = NULL;
               }
          }
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void
_cb_preload(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   Videothumb *sd = evas_object_smart_data_get(data);

   if (!sd) return;
   if (sd->o_img) evas_object_del(sd->o_img);
   sd->o_img = sd->o_img2;
   sd->o_img2 = NULL;
   evas_object_show(sd->o_img);
   evas_object_smart_callback_call(data, "data", NULL);
}

static void
_videothumb_image_load(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   char buf_base[PATH_MAX];
   char buf_file[PATH_MAX];
   char buf[PATH_MAX];
   unsigned char sum[20];
   Eina_Bool is_audio = EINA_FALSE;

   if (!sd) return;
   if (!sd->file) return;
   sd->o_img2 = evas_object_image_filled_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->o_img2, obj);
   const char *extn = strchr(sd->realpath, '.');
   if (extn)
     {
        if ((!strcasecmp(extn, ".mp3")) ||
            (!strcasecmp(extn, ".m4a")) ||
            (!strcasecmp(extn, ".oga")) ||
            (!strcasecmp(extn, ".aac")) ||
            (!strcasecmp(extn, ".flac")) ||
            (!strcasecmp(extn, ".wav")))
          {
             is_audio = EINA_TRUE;
          }
     }
   if (is_audio)
     {
        char *artfile = albumart_file_get(sd->realpath);
        if (artfile)
          {
             sd->realfile = eina_stringshare_add(artfile);
             free(artfile);
          }
     }
   else
     {
        if (!sha1((unsigned char *)sd->realpath, strlen(sd->realpath), sum))
          return;
        snprintf(buf_base, sizeof(buf_base), "%s/rage/thumb/%02x",
                 efreet_cache_home_get(), sum[0]);
        snprintf(buf_file, sizeof(buf_base),
                 "%s/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
                 "%02x%02x%02x%02x%02x%02x%02x%02x.eet",
                 buf_base,
                 sum[1], sum[2], sum[3],
                 sum[4], sum[5], sum[6], sum[7],
                 sum[8], sum[9], sum[10], sum[11],
                 sum[12], sum[13], sum[14], sum[15],
                 sum[16], sum[17], sum[18], sum[19]);
        if (sd->realfile) eina_stringshare_del(sd->realfile);
        sd->realfile = eina_stringshare_add(buf_file);
        sd->realpos = (((unsigned int)(sd->pos * 1000.0)) / 10000) * 10000;
        snprintf(buf, sizeof(buf), "%u", sd->realpos);
     }
   evas_object_event_callback_add(sd->o_img2,
                                  EVAS_CALLBACK_IMAGE_PRELOADED,
                                  _cb_preload, obj);
   evas_object_image_file_set(sd->o_img2, sd->realfile, buf);
   evas_object_image_size_get(sd->o_img2, &(sd->iw), &(sd->ih));
   _smart_calculate(obj);
   if (sd->iw > 0)
     {
        Eina_Bool ok = EINA_FALSE;
        struct stat st1, st2;

        if (stat(sd->realpath, &st1) == 0)
          {
             if (stat(sd->realfile, &st2) == 0)
               {
                  if (st1.st_mtime < st2.st_mtime) ok = EINA_TRUE;
               }
          }
        if (ok)
          {
             evas_object_image_preload(sd->o_img2, EINA_FALSE);
             return;
          }
     }
   if (sd->iw <= 0)
     {
        Eina_Bool ok = EINA_FALSE;
        struct stat st1, st2;

        if (stat(sd->realpath, &st1) == 0)
          {
             if (stat(sd->realfile, &st2) == 0)
               {
                  if (st1.st_mtime < st2.st_mtime)
                    {
                       ok = EINA_TRUE;
                    }
               }
          }
        if (!ok)
          {
             _videothumb_launch(obj);
          }
     }
}

static void
_videothumb_eval(Evas_Object *obj, Eina_Bool force)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh, vw, vh;
   Eina_Bool seen = EINA_FALSE;

   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(obj), NULL, NULL, &vw, &vh);
   // XXX: not listening to canvas resizes! :(
   if (ELM_RECTS_INTERSECT(ox, oy, ow, oh, 0, 0, vw, vh)) seen = EINA_TRUE;
   // XXX: fix - wrokaround and always visible
   seen = EINA_TRUE;
   if (force)
     {
        sd->seen = seen;
        if (sd->o_img2)
          {
             evas_object_del(sd->o_img2);
             sd->o_img2 = NULL;
          }
        _videothumb_image_load(obj);
        evas_object_move(sd->o_img2, ox, oy);
        evas_object_resize(sd->o_img2, ow, oh);
     }
   else
     {
        if (seen != sd->seen)
          {
             sd->seen = seen;
             if (sd->seen)
               {
                  if (sd->o_img2)
                    {
                       evas_object_del(sd->o_img2);
                       sd->o_img2 = NULL;
                    }
                  _videothumb_image_load(obj);
                  evas_object_move(sd->o_img2, ox, oy);
                  evas_object_resize(sd->o_img2, ow, oh);
               }
             else
               {
                  if (sd->o_img2)
                    {
                       evas_object_del(sd->o_img2);
                       sd->o_img2 = NULL;
                    }
               }
          }
     }
}

static void
_smart_add(Evas_Object *obj)
{
   Videothumb *sd;

   sd = calloc(1, sizeof(Videothumb));
   EINA_SAFETY_ON_NULL_RETURN(sd);
   evas_object_smart_data_set(obj, sd);
   _parent_sc.add(obj);
}

static void
_smart_del(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   vidthumbs = eina_list_remove(vidthumbs, obj);
   if (sd->thumb_exe)
     {
        if (sd->realpath) _busy_del(sd->realpath);
        _thumb_running--;
        ecore_exe_kill(sd->thumb_exe);
        ecore_exe_free(sd->thumb_exe);
     }
   if (sd->launch_timer)
     {
        ecore_timer_del(sd->launch_timer);
     }
   if (sd->file) eina_stringshare_del(sd->file);
   if (sd->realfile) eina_stringshare_del(sd->realfile);
   if (sd->realpath) free(sd->realpath);
   if (sd->o_img) evas_object_del(sd->o_img);
   if (sd->o_img2) evas_object_del(sd->o_img2);
   if (sd->exe_handler) ecore_event_handler_del(sd->exe_handler);
   if (sd->cycle_timer) ecore_timer_del(sd->cycle_timer);
   _parent_sc.del(obj);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;
   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   evas_object_smart_changed(obj);
}

static void
_smart_calculate(Evas_Object *obj)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   Evas_Coord ox, oy, ow, oh;

   if (!sd) return;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   sd->w = ow;
   sd->h = oh;
   _videothumb_eval(obj, EINA_FALSE);
   if (sd->o_img)
     {
        evas_object_move(sd->o_img, ox, oy);
        evas_object_resize(sd->o_img, ow, oh);
     }
   if (sd->o_img2)
     {
        evas_object_move(sd->o_img2, ox, oy);
        evas_object_resize(sd->o_img2, ow, oh);
     }
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_smart_changed(obj);
}

static void
_smart_init(void)
{
   static Evas_Smart_Class sc;

   evas_object_smart_clipped_smart_set(&_parent_sc);
   sc           = _parent_sc;
   sc.name      = "videothumb";
   sc.version   = EVAS_SMART_CLASS_VERSION;
   sc.add       = _smart_add;
   sc.del       = _smart_del;
   sc.resize    = _smart_resize;
   sc.move      = _smart_move;
   sc.calculate = _smart_calculate;
   _smart = evas_smart_class_new(&sc);
}

Evas_Object *
videothumb_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   if (!_smart) _smart_init();
   obj = evas_object_smart_add(e, _smart);
   vidthumbs = eina_list_prepend(vidthumbs, obj);
   return obj;
}

void
videothumb_file_set(Evas_Object *obj, const char *file, double pos)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->file == file) && (sd->pos == pos)) return;
   if (sd->file) eina_stringshare_del(sd->file);
   sd->file = eina_stringshare_add(file);
   if (!strncasecmp(sd->file, "file:/", 6))
     {
        Efreet_Uri *uri = efreet_uri_decode(sd->file);
        if (uri)
          {
             sd->realpath = ecore_file_realpath(uri->path);
             efreet_uri_free(uri);
          }
     }
   else if ((!strncasecmp(sd->file, "http:/", 6)) ||
            (!strncasecmp(sd->file, "https:/", 7)))
     sd->realpath = strdup(sd->file);
   else
     sd->realpath = ecore_file_realpath(sd->file);
   sd->pos = pos;
   _videothumb_eval(obj, EINA_TRUE);
}

static Eina_Bool
_cb_cycle(void *data)
{
   Evas_Object *obj = data;
   Videothumb *sd = evas_object_smart_data_get(obj);
   sd->pos += 10.0;
   if (!sd->thumb_exe)
     {
        _videothumb_eval(obj, EINA_TRUE);
        if (sd->iw <= 0)
          {
             sd->pos = 0;
             _videothumb_eval(obj, EINA_TRUE);
          }
     }
   return EINA_TRUE;
}

void
videothumb_autocycle_set(Evas_Object *obj, Eina_Bool enabled)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!enabled)
     {
        if (sd->cycle_timer) ecore_timer_del(sd->cycle_timer);
        sd->cycle_timer = NULL;
        return;
     }
   if (sd->cycle_timer) return;
   sd->cycle_timer = ecore_timer_add(0.5, _cb_cycle, obj);
}

void
videothumb_size_get(Evas_Object *obj, int *w, int *h)
{
   Videothumb *sd = evas_object_smart_data_get(obj);
   *w = 1;
   *h = 1;
   if (!sd) return;
   *w = sd->iw;
   *h = sd->ih;
}
