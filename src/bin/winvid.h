#ifndef _WINVID_H__
#define _WINVID_H__ 1

void win_video_init(Evas_Object *win);
void win_video_file_list_set(Evas_Object *win, Eina_List *list);
void win_video_insert(Evas_Object *win, const char *file);
void win_video_goto(Evas_Object *win, Eina_List *l);

#endif
