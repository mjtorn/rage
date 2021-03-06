#ifndef _BROWSER_H__
#define _BROWSER_H__ 1

Eina_Bool browser_visible(void);
void browser_show(Evas_Object *win);
void browser_hide(Evas_Object *win);
void browser_toggle(Evas_Object *win);
void browser_size_update(Evas_Object *win);
void browser_fullscreen(Evas_Object *win, Eina_Bool enabled);

#endif
