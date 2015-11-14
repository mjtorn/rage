#include <Elementary.h>
#include <Emotion.h>
#include "sha1.h"
#include "albumart.h"


static Evas_Object *win = NULL, *subwin = NULL, *image = NULL, *vid = NULL;
static Evas_Object *vidimage = NULL;
static Eet_File *ef = NULL;
static Ecore_Timer *vid_timeout = NULL;
static Eina_Bool is_audio = EINA_FALSE;
static Eina_Bool is_movie = EINA_FALSE;
static int iw, ih, incr = 0;
static Eina_Bool poster = 0;
static unsigned char sum[20];
static const char *file = NULL;

static void
_cb_fetched(void *data EINA_UNUSED)
{
   char *path = albumart_file_get(file);
   if (path)
     {
        Evas_Object *im = evas_object_image_add(evas_object_evas_get(win));
        int w, h;
        evas_object_image_file_set(im, path, NULL);
        evas_object_image_size_get(im, &w, &h);
        if ((w < 1) || (h < 0)) ecore_file_unlink(path);
        free(path);
     }
   elm_exit();
}

static void
_cb_loaded(void *data, Evas_Object *obj, void *info EINA_UNUSED)
{
   const char *file, *title, *artist, *album;

   if (vid_timeout) ecore_timer_del(vid_timeout);
   vid_timeout = NULL;
   
   file = data;
   title = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_TITLE);
   artist = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_ARTIST);
   album = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_ALBUM);

   if ((!emotion_object_video_handled_get(obj)) &&
       (emotion_object_audio_handled_get(obj)))
     is_audio = EINA_TRUE;

   if (is_audio)
     {
        albumart_find(file, title, artist, album, NULL,
                      _cb_fetched, (void *)file);
        return;
     }
   else if (poster)
     {
        double len = emotion_object_play_length_get(obj);
        double ratio = emotion_object_ratio_get(obj);
        int iw, ih;

        emotion_object_size_get(obj, &iw, &ih);
        if (ratio > 0.0) iw = (ih * ratio);
        else ratio = iw / ih;
        if ((ratio >= (4.0 / 3.0)) &&
            (ratio <= (3.0 / 1.0)) &&
            (len >= (75.0 * 60.0)) &&
            (len <= (5.0 * 60.0 * 60.0)))
          is_movie = EINA_TRUE;
     }

   if (is_movie)
     {
        albumart_find(file, NULL, NULL, NULL, "film poster",
                      _cb_fetched, (void *)file);
        return;
     }
   else
     {
        char buf_base[PATH_MAX];
        char buf_file[PATH_MAX];
        unsigned int pos;

        vidimage = evas_object_image_filled_add(evas_object_evas_get(subwin));
        evas_object_show(vidimage);

        evas_object_image_file_set(vidimage, file, NULL);
        evas_object_image_size_get(vidimage, &iw, &ih);
        if (!sha1((unsigned char *)file, strlen(file), sum)) exit(2);
        if (!efreet_cache_home_get()) exit(3);
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
        if (!ecore_file_mkpath(buf_base)) exit(4);
        ef = eet_open(buf_file, EET_FILE_MODE_WRITE);
        if (!ef) exit(5);
        pos = 0;
        for (pos = 0; ; pos += incr)
          {
             int w, h;
             int *pixels;
             char key[128];

             snprintf(key, sizeof(key), "%i", pos);
             evas_object_image_file_set(vidimage, file, key);
             evas_object_image_size_get(vidimage, &iw, &ih);
             if ((iw <= 0) || (ih <= 0))
               {
                  eet_close(ef);
                  exit(6);
               }
             w = 160;
             h = (ih * 160) / iw;
             if (h < 1) h = 1;
             evas_object_resize(vidimage, w, h);
             evas_object_resize(subwin, w, h);
             elm_win_render(subwin);
             pixels = evas_object_image_data_get(image, EINA_FALSE);
             if (pixels)
               eet_data_image_write(ef, key, pixels, w, h,
                                    0, 0, 70, EET_IMAGE_JPEG);
             else
               {
                  eet_close(ef);
                  exit(6);
               }
             evas_object_image_data_set(image, pixels);
          }
        eet_close(ef);
     }
}

static Eina_Bool
_cb_timeout(void *data EINA_UNUSED)
{
   vid_timeout = NULL;
   elm_exit();
   return EINA_FALSE;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   if (argc < 4) exit(1);
   elm_need_efreet();

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "rage", "themes/default.edj");

   elm_config_preferred_engine_set("buffer");
   win = elm_win_add(NULL, "Rage", ELM_WIN_BASIC);
   subwin = elm_win_add(win, "inlined", ELM_WIN_INLINED_IMAGE);
   image = elm_win_inlined_image_object_get(subwin);

   evas_object_show(subwin);
   evas_object_show(win);

   elm_win_norender_push(subwin);
   elm_win_norender_push(win);

   vid = emotion_object_add(evas_object_evas_get(win));
   file = argv[1];
   incr = atoi(argv[2]);
   poster = atoi(argv[3]);

   const char *extn = strchr(file, '.');
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
   if (emotion_object_init(vid, NULL))
     {
        evas_object_smart_callback_add(vid, "open_done", _cb_loaded, file);
        emotion_object_file_set(vid, file);
        vid_timeout = ecore_timer_add(20.0, _cb_timeout, NULL);
        elm_run();
     }
   elm_shutdown();
   return 0;
}
ELM_MAIN()
