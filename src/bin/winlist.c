#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "video.h"

static Evas_Object *tb = NULL;
static Evas_Object *sc, *rc, *bx;
static Ecore_Timer *bring_timer = NULL;

static void
_ready(Evas_Object *obj)
{
   int w = 1, h = 1;
   
   if (evas_object_data_get(obj, "ready")) return;
   evas_object_data_set(obj, "ready", obj);
   video_position_set(obj, video_length_get(obj) / 2.0);
   video_ratio_size_get(obj, &w, &h);
   evas_object_size_hint_aspect_set(obj, EVAS_ASPECT_CONTROL_NEITHER, w, h);
}

static void
_bring(Evas_Object *obj)
{
   Evas_Coord x, y, w, h, px, py;
   
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_geometry_get(bx, &px, &py, NULL, NULL);
   elm_scroller_region_bring_in(sc, x - px, y - py, w, h);
}

static Eina_Bool
_cb_bring_in(void *data)
{
   _bring(data);
   bring_timer = NULL;
   return EINA_FALSE;
}

static void
_cb_opened(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   _ready(obj);
}

static void
_cb_length(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   _ready(obj);
}

static Eina_Bool
_cb_pos_eval(void *data)
{
   Evas_Object *obj = data;
   Evas_Coord x, y, w, h, vx, vy, vw, vh;
   
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_output_viewport_get(evas_object_evas_get(obj), &vx, &vy, &vw, &vh);
   if (ELM_RECTS_INTERSECT(x, y, w, h, vx, vy, vw, vh))
     {
        if (!evas_object_data_get(obj, "active"))
          {
             const char *f;
             
             f = evas_object_data_get(obj, "file");
             video_play_set(obj, EINA_TRUE);
             video_loop_set(obj, EINA_TRUE);
             video_file_set(obj, f);
             evas_object_data_set(obj, "active", obj);
          }
     }
   else
     {
        if (evas_object_data_get(obj, "active"))
          {
             video_play_set(obj, EINA_FALSE);
             video_file_set(obj, NULL);
             video_position_set(obj, 0.0);
             evas_object_data_del(obj, "active");
             evas_object_data_del(obj, "ready");
          }
     }
   evas_object_data_del(obj, "timer");
   return EINA_FALSE;
}

static void
_cb_vid_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *tim = evas_object_data_get(obj, "timer");
   if (tim) ecore_timer_del(tim);
   tim = ecore_timer_add(0.1, _cb_pos_eval, obj);
   evas_object_data_set(obj, "timer", tim);
}

static void
_cb_vid_resize(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *tim = evas_object_data_get(obj, "timer");
   if (tim) ecore_timer_del(tim);
   tim = ecore_timer_add(0.1, _cb_pos_eval, obj);
   evas_object_data_set(obj, "timer", tim);
}

static void
_cb_vid_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *tim = evas_object_data_get(obj, "timer");
   if (tim) ecore_timer_del(tim);
   evas_object_data_del(obj, "timer");
   if (bring_timer)
     {
        ecore_timer_del(bring_timer);
        bring_timer = NULL;
     }
}

static void
_sel(Evas_Object *obj)
{
   Eina_List *items = elm_box_children_get(bx);
   Eina_List *l;
   Evas_Object *o;
   
   EINA_LIST_FOREACH(items, l, o)
     {
        if (evas_object_data_get(o, "selected"))
          {
             evas_object_data_del(o, "selected");
             elm_layout_signal_emit(o, "rage,state,unselected", "rage");
          }
     }
   elm_layout_signal_emit(obj, "rage,state,selected", "rage");
   evas_object_data_set(obj, "selected", obj);
}

static void
_cb_selected(void *data, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   if (evas_object_data_get(obj, "selected")) return;
   _sel(obj);
   win_video_goto(data, evas_object_data_get(obj, "list"));
}

static void
_fill_box(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *l;
   const char *f, *s;
   Evas_Object *o, *base, *rect;
   char buf[4096];
   Evas_Coord w, h, sz = 0;
   
   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   evas_object_geometry_get(win, NULL, NULL, &w, &h);
   w = w / 8;
   h = h / 8;
   if (w < sz) w = sz;
   if (h < sz) h = sz;
   EINA_LIST_FOREACH(inf->file_list, l, f)
     {
        base = o = elm_layout_add(win);
        elm_object_focus_allow_set(o, EINA_FALSE);
        snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
        elm_layout_file_set(o, buf, "rage/list/item");
        evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(o, -1.0, 0.5);
        elm_layout_signal_callback_add(o, "rage,selected", "rage", _cb_selected, win);
        evas_object_data_set(o, "list", l);
        
        s = ecore_file_file_get(f);
        if ((s) && (s[0] != 0))
          elm_object_part_text_set(o, "rage.title", s);
        else
          elm_object_part_text_set(o, "rage.title", f);
        
        rect = o = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_color_set(rect, 0, 0, 0, 0);
        elm_object_part_content_set(base, "rage.sizer", o);
        evas_object_data_set(base, "sizer", o);
        evas_object_size_hint_min_set(o, w, h);
        
        o = video_add(win);
        evas_object_data_set(o, "base", base);
        evas_object_data_set(base, "video", o);
        video_mute_set(o, EINA_TRUE);
        video_fill_set(o, EINA_TRUE);
        video_lowquality_set(o, EINA_TRUE);
        evas_object_smart_callback_add(o, "opened", _cb_opened, win);
        evas_object_smart_callback_add(o, "length", _cb_length, win);
        
        evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _cb_vid_move, win);
        evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _cb_vid_resize, win);
        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL, _cb_vid_del, win);
        
        evas_object_data_set(o, "file", f);
        elm_object_part_content_set(base, "rage.content", o);
        evas_object_show(o);
        
        elm_box_pack_end(bx, base);
        evas_object_show(base);
        
        if (l == inf->file_cur)
          {
             evas_object_data_set(base, "selected", base);
             elm_layout_signal_emit(base, "rage,state,selected", "rage");
             if (bring_timer) ecore_timer_del(bring_timer);
             bring_timer = ecore_timer_add(0.3, _cb_bring_in, base);
          }
     }
}

void
win_list_show(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
 
   if (tb) return;
   tb = elm_table_add(win);
   evas_object_show(tb);

   rc = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_weight_set(rc, 1.0, 1.0);
   evas_object_color_set(rc, 0, 0, 0, 0);
   elm_table_pack(tb, rc, 0, 0, 1, 1);

   sc = elm_scroller_add(win);
   elm_object_style_set(sc, "noclip");
   elm_object_focus_allow_set(sc, EINA_FALSE);
   evas_object_size_hint_weight_set(sc, 1.0, 1.0);
   evas_object_size_hint_align_set(sc, -1.0, -1.0);
   elm_scroller_content_min_limit(sc, EINA_TRUE, EINA_FALSE);
   elm_table_pack(tb, sc, 0, 0, 1, 1);
   evas_object_show(sc);
   
   bx = elm_box_add(win);
   elm_object_focus_allow_set(bx, EINA_FALSE);
   evas_object_size_hint_weight_set(bx, 1.0, 0.0);
   evas_object_size_hint_align_set(bx, -1.0, 0.0);
   elm_box_homogeneous_set(bx, EINA_TRUE);
   
   _fill_box(win);
   
   elm_object_content_set(sc, bx);
   evas_object_show(bx);
   
   elm_object_part_content_set(inf->lay, "rage.list", tb);
   
   elm_layout_signal_emit(inf->lay, "list,state,visible", "rage");
}

static void
_cb_hidden(void *data EINA_UNUSED, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   elm_layout_signal_callback_del(obj, "list,state,hidden,finished", "rage",
                                  _cb_hidden);
   evas_object_del(tb);
   tb = NULL;
}

void
win_list_hide(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");
 
   if (!tb) return;
   if (bring_timer) ecore_timer_del(bring_timer);
   bring_timer = NULL;
   elm_layout_signal_callback_add(inf->lay, "list,state,hidden,finished", "rage",
                                  _cb_hidden, win);
   elm_layout_signal_emit(inf->lay, "list,state,hidden", "rage");
}

void
win_list_toggle(Evas_Object *win)
{
   if (tb) win_list_hide(win);
   else win_list_show(win);
}

void
win_list_sel_update(Evas_Object *win)
{
   if (!tb) return;
   Inf *inf = evas_object_data_get(win, "inf");
   Eina_List *items = elm_box_children_get(bx);
   Eina_List *l;
   Evas_Object *o;
   
   EINA_LIST_FOREACH(items, l, o)
     {
        if (inf->file_cur == evas_object_data_get(o, "list"))
          {
             if (evas_object_data_get(o, "selected")) return;
             _sel(o);
             _bring(o);
             return;
          }
     }
}

void
win_list_size_update(Evas_Object *win)
{
   if (!tb) return;
   Eina_List *items = elm_box_children_get(bx);
   Eina_List *l;
   Evas_Object *o;
   Evas_Coord w, h, sz = 0;
   
   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   evas_object_geometry_get(win, NULL, NULL, &w, &h);
   w = w / 8;
   h = h / 8;
   if (w < sz) w = sz;
   if (h < sz) h = sz;
   EINA_LIST_FOREACH(items, l, o)
     {
        Evas_Object *sizer = evas_object_data_get(o, "sizer");
        evas_object_size_hint_min_set(sizer, w, h);
     }
}

void
win_list_content_update(Evas_Object *win)
{
   Eina_List *list;

   if (!tb) return;
   while ((list = elm_box_children_get(bx)))
     {
        evas_object_del(list->data);
     }
   _fill_box(win);
}
