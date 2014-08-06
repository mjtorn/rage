#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "config.h"

static Eina_Bool
_cb_show_timeout(void *data)
{
   Evas_Object *win = data;
   Inf *inf = evas_object_data_get(win, "inf");

   inf->show_timeout = NULL;
   evas_object_show(win);
   return EINA_FALSE;
}

static void
_cb_resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   win_list_size_update(obj);
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
   Config *config;

   elm_need_efreet();
   config_init();
   config = config_get();
   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf("Usage: rage [OPTIONS] [file1] [file2] [...]\n"
                    "  Where OPTIONS can ben"
                    "    -e ENGINE\n"
                    "      ENGINE is one of gstreamer1, xine or vlc\n"
                    "      The default is gstreamer1\n"
                    "    -h | -help | --help\n"
                    "      This help\n");
             exit(0);
          }
        else if (!strcmp(argv[i], "-e"))
          {
             if (i < (argc - 1))
               {
                  i++;
                  eina_stringshare_del(config->emotion_engine);
                  config->emotion_engine = eina_stringshare_add(argv[i]);
               }
          }
        else
          list = eina_list_append(list, eina_stringshare_add(argv[i]));
     }

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "rage", "themes/default.edj");

#if (ELM_VERSION_MAJOR > 1) || (ELM_VERSION_MINOR >= 10)
   elm_config_accel_preference_set("accel");
#endif

   snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
   elm_theme_overlay_add(NULL, buf);

   win = win_add();
   if (!win)
     {
        printf("ERROR - cannto create window!\n");
        goto end;
     }

   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _cb_resize, NULL);
   evas_object_resize(win, 320, 200);

   win_video_init(win);
   win_video_file_list_set(win, list);
   EINA_LIST_FREE(list, f) eina_stringshare_del(f);

   inf = evas_object_data_get(win, "inf");
   if (argc <= 1)
     {
        elm_layout_signal_emit(inf->lay, "about,show", "rage");
        elm_layout_signal_emit(inf->lay, "state,novideo", "rage");
        evas_object_show(win);
     }
   else
     {
        inf->show_timeout = ecore_timer_add(10.0, _cb_show_timeout, win);
     }

   elm_run();

end:
   config_shutdown();
   elm_shutdown();
   return 0;
}
ELM_MAIN()
