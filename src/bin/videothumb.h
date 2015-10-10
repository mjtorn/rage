#ifndef _VIDEOTHUMB_H__
#define _VIDEOTHUMB_H__ 1

Evas_Object *videothumb_add(Evas_Object *parent);
void videothumb_file_set(Evas_Object *obj, const char *file, double pos);
void videothumb_autocycle_set(Evas_Object *obj, Eina_Bool enabled);
void videothumb_size_get(Evas_Object *obj, int *w, int *h);

#endif
