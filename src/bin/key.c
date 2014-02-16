#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "video.h"
#include "key.h"
#include "winlist.h"

void
key_handle(Evas_Object *win, Evas_Event_Key_Down *ev)
{
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
            (!strcmp(ev->key, "Pause")) ||
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
   else if (!strcmp(ev->key, "Home"))
     {
        if (win_video_have_prev(win)) win_video_first(win);
        else win_video_prev(win);
     }
   else if (!strcmp(ev->key, "End"))
     {
        if (win_video_have_next(win)) win_video_last(win);
        else win_video_next(win);
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
   else if (!strcmp(ev->keyname, "backslash"))
     {
        win_list_toggle(win);
     }
   else if (!strcmp(ev->keyname, "y"))
     {
        video_lowquality_set(inf->vid, !video_lowquality_get(inf->vid));
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
