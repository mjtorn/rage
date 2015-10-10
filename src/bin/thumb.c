#include <Elementary.h>
#include <Emotion.h>
#include "sha1.h"
#include "albumart.h"

static Evas_Object *win = NULL, *subwin = NULL, *image = NULL;
static Evas_Object *vidimage = NULL;
static int iw, ih;
static unsigned char sum[20];
static Eet_File *ef = NULL;
static Ecore_Timer *vid_timeout = NULL;

static void
_cb_fetched(void *data EINA_UNUSED)
{
   elm_exit();
}

static void
_cb_loaded(void *data, Evas_Object *obj, void *info EINA_UNUSED)
{
   const char *file, *title, *artist, *album;

   file = data;
   title = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_TITLE);
   artist = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_ARTIST);
   album = emotion_object_meta_info_get(obj, EMOTION_META_INFO_TRACK_ALBUM);

   albumart_find(file, title, artist, album, _cb_fetched, (void *)file);
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
   Eina_Bool is_audio = EINA_FALSE;

   if (argc < 3) exit(1);
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

   const char *extn = strchr(argv[1], '.');
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
        Evas_Object *vid;

        vid = emotion_object_add(evas_object_evas_get(win));
        if (emotion_object_init(vid, NULL))
          {
             evas_object_smart_callback_add(vid, "open_done",
                                            _cb_loaded, argv[1]);
             emotion_object_file_set(vid, argv[1]);
             vid_timeout = ecore_timer_add(2.0, _cb_timeout, NULL);
             elm_run();
          }
     }
   else
     {
        char buf_base[PATH_MAX];
        char buf_file[PATH_MAX];
        unsigned int pos, incr;

        vidimage = evas_object_image_filled_add(evas_object_evas_get(subwin));
        evas_object_show(vidimage);

        evas_object_image_file_set(vidimage, argv[1], NULL);
        evas_object_image_size_get(vidimage, &iw, &ih);
        if (!sha1((unsigned char *)argv[1], strlen(argv[1]), sum)) exit(2);
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
        incr = atoi(argv[2]);
        for (pos = 0; ; pos += incr)
          {
             int w, h;
             int *pixels;
             char key[128];

             snprintf(key, sizeof(key), "%i", pos);
             evas_object_image_file_set(vidimage, argv[1], key);
             evas_object_image_size_get(vidimage, &iw, &ih);
             if ((iw <= 0) || (ih <= 0)) break;
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
               exit(6);
             evas_object_image_data_set(image, pixels);
          }
        eet_close(ef);
     }
   elm_shutdown();
   return 0;
}
ELM_MAIN()
