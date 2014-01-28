#ifndef _WIN_H__
#define _WIN_H__ 1

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

// ui high level controls
void win_do_play(Evas_Object *win);
void win_do_pause(Evas_Object *win);
void win_do_play_pause(Evas_Object *win);
void win_do_prev(Evas_Object *win);
void win_do_next(Evas_Object *win);

// lower level controls
void win_video_next(Evas_Object *win);
void win_video_prev(Evas_Object *win);
void win_video_first(Evas_Object *win);
void win_video_last(Evas_Object *win);
Eina_Bool win_video_have_next(Evas_Object *win);
Eina_Bool win_video_have_prev(Evas_Object *win);
Evas_Object *win_add(void);
void win_title_update(Evas_Object *win);
void win_show(Evas_Object *win, int w, int h);
void win_aspect_adjust(Evas_Object *win);
void win_frame_decode(Evas_Object *win);

#endif
