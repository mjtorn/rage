#ifndef _VIDEO_H__
#define _VIDEO_H__ 1

#include <Emotion.h>

Evas_Object *video_add(Evas_Object *parent);
void video_file_set(Evas_Object *obj, const char *file);
void video_mute_set(Evas_Object *obj, Eina_Bool mute);
Eina_Bool video_mute_get(Evas_Object *obj);
void video_play_set(Evas_Object *obj, Eina_Bool play);
Eina_Bool video_play_get(Evas_Object *obj);
void video_loop_set(Evas_Object *obj, Eina_Bool loop);
Eina_Bool video_loop_get(Evas_Object *obj);
void video_fill_set(Evas_Object *obj, Eina_Bool fill);
Eina_Bool video_fill_get(Evas_Object *obj);
void video_position_set(Evas_Object *obj, double pos);
double video_position_get(Evas_Object *obj);
double video_length_get(Evas_Object *obj);
void video_stop(Evas_Object *obj);
void video_ratio_size_get(Evas_Object *obj, int *w, int *h);
void video_eject(Evas_Object *obj);
int video_chapter_count(Evas_Object *obj);
void video_chapter_set(Evas_Object *obj, int chapter);
int video_chapter_get(Evas_Object *obj);
const char * video_chapter_name_get(Evas_Object *obj, int chapter);
void video_volume_set(Evas_Object *obj, double vol);
double video_volume_get(Evas_Object *obj);
Eina_Bool video_has_video_get(Evas_Object *obj);
Eina_Bool video_has_audio_get(Evas_Object *obj);
const char *video_title_get(Evas_Object *obj);
int video_audio_channel_count(Evas_Object *obj);
void video_audio_channel_set(Evas_Object *obj, int chan);
int video_audio_channel_get(Evas_Object *obj);
const char *video_audio_channel_name_get(Evas_Object *obj, int chan);
int video_video_channel_count(Evas_Object *obj);
void video_video_channel_set(Evas_Object *obj, int chan);
int video_video_channel_get(Evas_Object *obj);
const char *video_video_channel_name_get(Evas_Object *obj, int chan);
int video_spu_channel_count(Evas_Object *obj);
void video_spu_channel_set(Evas_Object *obj, int chan);
int video_spu_channel_get(Evas_Object *obj);
const char *video_spu_channel_name_get(Evas_Object *obj, int chan);
int video_spu_button_count(Evas_Object *obj);
int video_spu_button_get(Evas_Object *obj);
void video_event_send(Evas_Object *obj, Emotion_Event ev);
void video_lowquality_set(Evas_Object *obj, Eina_Bool lowq);
Eina_Bool video_lowquality_get(Evas_Object *obj);

#endif
