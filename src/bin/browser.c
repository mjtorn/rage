#include <Elementary.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "browser.h"
#include "videothumb.h"
#include "key.h"

typedef struct _Message Message;
typedef struct _Entry Entry;

typedef enum _Type
{
   TYPE_NEW,
   TYPE_UPDATE,
   TYPE_FINISH
} Type;

struct _Message
{
   Type type;
   Entry *entry;
};

struct _Entry
{
   Eina_Lock lock;
   Entry *parent; // what is above
   Eina_Stringshare *path; // full path
   Eina_List *dirs; // entries fo subdir entires
   Eina_List *files; // strings of just filenames in path dir
   Eina_List *sels;
   Evas_Object *base;
   Evas_Object *box;
   Evas_Object *table;
   Evas_Object *sizer;
   Evas_Coord iw, ih;
   int cols, rows;
   int sel_x, sel_y;
   Eina_Bool sel : 1;
};

static char *selfile = NULL;
static Entry *selentry = NULL;
static int seli = 0;

static Evas_Object *bx = NULL;
static Evas_Object *sc, *bt;
static Ecore_Thread *fill_thread = NULL;
static Entry *dir_entry = NULL;

static void _sel_go(Evas_Object *win EINA_UNUSED, Entry *base_entry, int x, int y);

static void
_item_size_get(Evas_Object *win, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord sz = 0;

   elm_coords_finger_size_adjust(1, &sz, 1, &sz);
   evas_object_geometry_get(win, NULL, NULL, w, h);
   *w = *w / 5;
   *h = *h / 5;
   if (*w < sz) *w = sz;
   if (*h < sz) *h = sz;
}

static Eina_Bool
_video_ok(const char *path)
{
   const char *exts[] =
     {
        ".asf", ".avi", ".bdm", ".bdmv", ".clpi", ".cpi", ".dv", ".fla",
        ".flv", ".m1v", ".m2t", ".m2v", ".m4v", ".mkv", ".mov", ".mp2",
        ".mp2ts", ".mp4", ".mpe", ".mpeg", ".mpg", ".mpl", ".mpls", ".mts",
        ".mxf", ".nut", ".nuv", ".ogg", ".ogm", ".ogv", ".qt", ".rm", ".rmj",
        ".rmm", ".rms", ".rmvb", ".rmx", ".rv", ".swf", ".ts", ".weba",
        ".webm", ".wmv", ".3g2", ".3gp", ".3gp2", ".3gpp", ".3gpp2", ".3p2",
        ".264",
        NULL
     };
   int i;
   const char *ext = strrchr(path, '.');
   if (!ext) return EINA_FALSE;
   for (i = 0; exts[i]; i++)
     {
        if (!strcasecmp(ext, exts[i])) return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
_audio_ok(const char *path)
{
   const char *exts[] =
     {
        ".mp3", ".m4a", ".oga", ".aac", ".flac", ".wav",
        NULL
     };
   int i;
   const char *ext = strrchr(path, '.');
   if (!ext) return EINA_FALSE;
   for (i = 0; exts[i]; i++)
     {
        if (!strcasecmp(ext, exts[i])) return EINA_TRUE;
     }
   return EINA_FALSE;
}

static void
_fill_message(Ecore_Thread *th, Type type, Entry *entry)
{
   Message *message = calloc(1, sizeof(Message));
   if (!message) return;
   message->type = type;
   message->entry = entry;
   ecore_thread_feedback(th, message);
}

static Entry *
_fill_scan(Ecore_Thread *th, Entry *parent, const char *dir)
{
   Eina_List *files;
   char *file;
   Entry *entry = NULL;

   entry = calloc(1, sizeof(Entry));
   if (!entry) return NULL;
   files = ecore_file_ls(dir);
   if (!files)
     {
        if (entry == selentry) selentry = NULL;
        free(entry);
        return NULL;
     }
   eina_lock_new(&(entry->lock));
   entry->parent = parent;
   entry->path = eina_stringshare_add(dir);
   if (parent)
     {
        eina_lock_take(&(parent->lock));
        parent->dirs = eina_list_append(parent->dirs, entry);
        eina_lock_release(&(parent->lock));
     }
   _fill_message(th, TYPE_NEW, entry);
   EINA_LIST_FREE(files, file)
     {
        if (!ecore_thread_check(th))
          {
             if (file[0] != '.')
               {
                  char buf[PATH_MAX];

                  snprintf(buf, sizeof(buf), "%s/%s", dir, file);
                  if (ecore_file_is_dir(buf))
                    {
                       _fill_scan(th, entry, buf);
                       _fill_message(th, TYPE_UPDATE, entry);
                    }
                  else
                    {
                       if (_video_ok(file) || _audio_ok(file))
                         {
                            eina_lock_take(&(entry->lock));
                            entry->files = eina_list_append
                              (entry->files, eina_stringshare_add(file));
                            eina_lock_release(&(entry->lock));
                            _fill_message(th, TYPE_UPDATE, entry);
                         }
                    }
               }
          }
        free(file);
     }
   _fill_message(th, TYPE_FINISH, entry);
   return entry;
}

static void
_fill_thread(void *data EINA_UNUSED, Ecore_Thread *th)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/Videos", eina_environment_home_get());
   _fill_scan(th, NULL, buf);
}

static void
_cb_vidthumb_data(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   int w, h;

   videothumb_size_get(obj, &w, &h);
   evas_object_size_hint_aspect_set(obj, EVAS_ASPECT_CONTROL_BOTH, w, h);
}

static void
_activate(Evas_Object *win, Entry *entry, const char *file)
{
   Eina_List *list = NULL;
   Winvid_Entry *vid;
   char buf[PATH_MAX];

   vid = calloc(1, sizeof(Winvid_Entry));
   if (vid)
     {
        snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
        vid->file = eina_stringshare_add(buf);
        list = eina_list_append(list, vid);
     }
   win_video_file_list_set(win, list);
   EINA_LIST_FREE(list, vid)
     {
        if (vid->file) eina_stringshare_del(vid->file);
        if (vid->sub) eina_stringshare_del(vid->sub);
        if (vid->uri) efreet_uri_free(vid->uri);
        free(vid);
     }
   browser_hide(win);
}

static void
_cb_file_selected(void *data, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   Evas_Object *win = data;
   Entry *entry = evas_object_data_get(obj, "entry");
   char buf[PATH_MAX];
   const char *file = evas_object_data_get(obj, "file");

   elm_layout_signal_emit(obj, "rage,state,selected", "rage");
   _activate(win, entry, file);
   snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
   if (selfile) free(selfile);
   selfile = strdup(buf);
}

static void
_entry_files_pop(Evas_Object *win, Entry *entry)
{
   Evas_Object *o, *base;
   int i = 0, j = 0;
   Eina_List *l;
   const char *file;
   char buf[PATH_MAX], *p;

   if (evas_object_data_get(entry->table, "populated")) return;
   evas_object_data_set(entry->table, "populated", entry->table);

   EINA_LIST_FOREACH(entry->files, l, file)
     {
        base = o = elm_layout_add(win);
        entry->sels = eina_list_append(entry->sels, o);
        evas_object_data_set(o, "entry", entry);
        evas_object_data_set(o, "file", file);
        elm_object_focus_allow_set(o, EINA_FALSE);
        snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
        elm_layout_file_set(o, buf, "rage/browser/item");
        snprintf(buf, sizeof(buf), "%s", file);
        p = strrchr(buf, '.');
        if (p) *p = 0;
        elm_object_part_text_set(o, "rage.title", buf);
        evas_object_size_hint_weight_set(o, 0.0, 0.0);
        evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(entry->table, o, i, j, 1, 1);
        evas_object_show(o);

        elm_layout_signal_callback_add(o, "rage,selected", "rage", _cb_file_selected, win);

        o = videothumb_add(win);
        evas_object_smart_callback_add(o, "data", _cb_vidthumb_data, base);
        evas_object_data_set(o, "entry", entry);
        evas_object_data_set(o, "file", file);
        snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
        videothumb_file_set(o, buf, 0.0);
        videothumb_autocycle_set(o, EINA_TRUE);
        elm_object_part_content_set(base, "rage.content", o);
        evas_object_show(o);

        if ((entry->sel) && (entry->sel_x == i) && (entry->sel_y == j))
          elm_layout_signal_emit(base, "rage,state,selected", "rage");

        i++;
        if (i == entry->cols)
          {
             i = 0;
             j++;
          }
     }
  if ((entry->cols > 0) && (entry->rows > 0))
     elm_table_pack(entry->table, entry->sizer, 0, 0, entry->cols, entry->rows);
   else
     elm_table_pack(entry->table, entry->sizer, 0, 0, 1, 1);
}

static void
_entry_files_unpop(Evas_Object *win EINA_UNUSED, Entry *entry)
{
   evas_object_size_hint_min_set(entry->sizer,
                                 entry->cols * entry->iw,
                                 entry->rows * entry->ih);
   if (!evas_object_data_get(entry->table, "populated")) return;
   entry->sels = eina_list_free(entry->sels);
   evas_object_data_del(entry->table, "populated");
   elm_table_unpack(entry->table, entry->sizer);
   elm_table_clear(entry->table, EINA_TRUE);
  if ((entry->cols > 0) && (entry->rows > 0))
     elm_table_pack(entry->table, entry->sizer, 0, 0, entry->cols, entry->rows);
   else
     elm_table_pack(entry->table, entry->sizer, 0, 0, 1, 1);
}

static void
_cb_sel_job(void *data)
{
   Evas_Object *win = data;
   Entry *entry = selentry;
   if ((!dir_entry) || (!entry)) return;
   eina_lock_take(&(entry->lock));
   entry->sel = EINA_TRUE;
   if (entry->cols > 0) entry->sel_y = seli / entry->cols;
   entry->sel_x = seli - (entry->sel_y * entry->cols);
   eina_lock_release(&(entry->lock));
   _sel_go(win, dir_entry, 0, 0);
}

static void
_entry_files_redo(Evas_Object *win, Entry *entry)
{
   Evas_Coord x, y,w, h, iw = 1, ih = 1, ww, wh;
   int num, cols, rows;
   Eina_Rectangle r1, r2;

   eina_lock_take(&(entry->lock));

   num = eina_list_count(entry->files);
   evas_object_geometry_get(win, NULL, NULL, &ww, &wh);
   evas_object_geometry_get(entry->table, &x, &y, &w, &h);
   if (w < 40) goto done;

   if (w > (ww - 20)) w = (ww - 20);

   _item_size_get(win, &iw, &ih);
   cols = w / iw;
   if (cols < 1) cols = 1;
   rows = (num + (cols - 1)) / cols;

   entry->iw = iw;
   entry->ih = ih;
   entry->cols = cols;
   entry->rows = rows;

   r1.x = 0; r1.y = 0; r1.w = ww; r1.h = wh;
   r2.x = x; r2.y = y; r2.w = w; r2.h = h;

   _entry_files_unpop(win, entry);
   if (eina_rectangles_intersect(&r1, &r2)) _entry_files_pop(win, entry);

   if (selfile)
     {
        Eina_List *l;
        const char *file;
        char buf[PATH_MAX];
        int i;

        i = 0;
        EINA_LIST_FOREACH(entry->files, l, file)
          {
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (!strcmp(buf, selfile))
               {
                  selentry = entry;
                  seli = i;
                  ecore_job_add(_cb_sel_job, win);
                  break;
               }
             i++;
          }
     }
done:
   eina_lock_release(&(entry->lock));
}

static void
_cb_entry_table_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   Entry *entry = data;
   Evas_Object *win = evas_object_data_get(obj, "win");
   Evas_Coord x, y,w, h, ww, wh;
   Eina_Rectangle r1, r2;

   eina_lock_take(&(entry->lock));

   evas_object_geometry_get(win, NULL, NULL, &ww, &wh);
   evas_object_geometry_get(entry->table, &x, &y, &w, &h);
   if (w < 40) goto done;

   if (w > (ww - 20)) w = (ww - 20);

   r1.x = 0; r1.y = 0; r1.w = ww; r1.h = wh;
   r2.x = x; r2.y = y; r2.w = w; r2.h = h;

   if (eina_rectangles_intersect(&r1, &r2)) _entry_files_pop(win, entry);
   else _entry_files_unpop(win, entry);

done:
   eina_lock_release(&(entry->lock));
}

static void
_cb_entry_table_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   Entry *entry = data;
   Evas_Object *win = evas_object_data_get(obj, "win");
   _entry_files_redo(win, entry);
}

static void
_fill_feedback(void *data, Ecore_Thread *th, void *msg)
{
   Evas_Object *win = data;
   Message *message = msg;
   Evas_Object *o;
   char buf[PATH_MAX];

   if ((th == fill_thread) && (bx))
     {
        Entry *entry = message->entry;

        if ((message->type == TYPE_NEW) && (!dir_entry)) dir_entry = entry;

        if (message->type == TYPE_NEW)
          {
             eina_lock_take(&(entry->lock));
             if ((entry->dirs) || (entry->files))
               {
                  if (!entry->base)
                    {
                       const char *file;

                       entry->base = o = elm_layout_add(win);
                       elm_object_focus_allow_set(o, EINA_FALSE);
                       snprintf(buf, sizeof(buf), "%s/themes/default.edj", elm_app_data_dir_get());
                       elm_layout_file_set(o, buf, "rage/browser/entry");
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);

                       if (entry->parent)
                         {
                            file = entry->path + strlen(dir_entry->path) + 1;
                            elm_object_part_text_set(o, "rage.title", file);
                         }

                       entry->box = o = elm_box_add(win);
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
                       elm_object_part_content_set(entry->base, "rage.content", o);
                       evas_object_show(o);

                       entry->table = o = elm_table_add(win);
                       elm_table_homogeneous_set(o, EINA_TRUE);
                       evas_object_data_set(o, "win", win);
                       evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _cb_entry_table_move, entry);
                       evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _cb_entry_table_resize, entry);
                       evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
                       evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
                       elm_box_pack_end(entry->box, o);
                       evas_object_show(o);

                       entry->sizer = o = evas_object_rectangle_add(evas_object_evas_get(win));
                       evas_object_color_set(o, 0, 0, 0, 0);
                       evas_object_size_hint_min_set(o, 10, 10);
                       elm_table_pack(entry->table, o, 0, 0, 1, 1);
                       evas_object_show(o);

                       if (!entry->parent)
                         elm_box_pack_end(bx, entry->base);
                       else
                         elm_box_pack_end(entry->parent->box, entry->base);
                       evas_object_show(entry->base);
                    }
               }
             eina_lock_release(&(entry->lock));
          }
        else if ((message->type == TYPE_FINISH) && (entry->parent))
          {
             _entry_files_redo(win, entry);
          }
     }
   free(msg);
}

static void
_fill_end(void *data EINA_UNUSED, Ecore_Thread *th)
{
   if (th == fill_thread) fill_thread = NULL;
}

static void
_fill_cancel(void *data EINA_UNUSED, Ecore_Thread *th)
{
   if (th == fill_thread) fill_thread = NULL;
}

static void
_entry_free(Entry *entry)
{
   Entry *subentry;
   Eina_Stringshare *str;
   if (!entry) return;
   eina_lock_take(&(entry->lock));
   entry->sels = eina_list_free(entry->sels);
   EINA_LIST_FREE(entry->files, str) eina_stringshare_del(str);
   EINA_LIST_FREE(entry->dirs, subentry) _entry_free(subentry);
   if (entry->base) evas_object_del(entry->base);
   eina_stringshare_del(entry->path);
   eina_lock_release(&(entry->lock));
   eina_lock_free(&(entry->lock));
   if (entry == selentry) selentry = NULL;
   free(entry);
}

static void
_fill(Evas_Object *win)
{
   if (fill_thread)
     {
        ecore_thread_cancel(fill_thread);
        ecore_thread_wait(fill_thread, 10.0);
     }
   _entry_free(dir_entry);
   dir_entry = NULL;
   fill_thread = ecore_thread_feedback_run(_fill_thread, _fill_feedback,
                                           _fill_end, _fill_cancel,
                                           win, EINA_TRUE);
}

static Entry *
_sel_find(Entry *entry)
{
   Eina_List *l;
   Entry *subentry, *tmpentry;

   eina_lock_take(&(entry->lock));
   if (entry->sel)
     {
        eina_lock_release(&(entry->lock));
        return entry;
     }
   EINA_LIST_FOREACH(entry->dirs, l, subentry)
     {
        tmpentry = _sel_find(subentry);
        if (tmpentry)
          {
             eina_lock_release(&(entry->lock));
             return tmpentry;
          }
     }
   eina_lock_release(&(entry->lock));
   return NULL;
}

static Evas_Object *
_sel_object_find(Entry *entry)
{
   int num = (entry->sel_y * entry->cols) + entry->sel_x;
   Evas_Object *o = eina_list_nth(entry->sels, num);
   return o;
}

static const char *
_sel_file_find(Entry *entry)
{
   int num = (entry->sel_y * entry->cols) + entry->sel_x;
   const char *file = eina_list_nth(entry->files, num);
   return file;
}

static Eina_List *
_entry_list_flatten(Eina_List *list, Entry *entry)
{
   Eina_List *l;
   Entry *subentry;

   eina_lock_take(&(entry->lock));
   if (entry->files) list = eina_list_append(list, entry);
   EINA_LIST_FOREACH(entry->dirs, l, subentry)
     {
        list = _entry_list_flatten(list, subentry);
     }
   eina_lock_release(&(entry->lock));
   return list;
}

static void
_sel_go(Evas_Object *win EINA_UNUSED, Entry *base_entry, int x, int y)
{
   Evas_Object *o;
   Evas_Coord bxx, bxy, tbx, tby;
   const char *file;

   if (!base_entry) return;
   evas_object_geometry_get(bx, &bxx, &bxy, NULL, NULL);
   Entry *entry = _sel_find(base_entry);
   if (!entry)
     {
        Eina_List *flatlist = _entry_list_flatten(NULL, base_entry);

        if (flatlist)
          {
             entry = flatlist->data;
             eina_lock_take(&(entry->lock));
             entry->sel = EINA_TRUE;
             entry->sel_x = 0;
             entry->sel_y = 0;
             o = _sel_object_find(entry);
             elm_layout_signal_emit(o, "rage,state,selected", "rage");
             eina_lock_release(&(entry->lock));
             eina_list_free(flatlist);
          }
     }
   else
     {
        int sel_x, sel_y, num;
        Entry *subentry;
        Eina_List *l;
        Eina_List *flatlist = _entry_list_flatten(NULL, base_entry);

        eina_lock_take(&(entry->lock));
        sel_x = entry->sel_x + x;
        sel_y = entry->sel_y + y;
        if (sel_x >= entry->cols)
          {
             sel_y++;
             sel_x = 0;
          }
        else if (sel_x < 0)
          {
             sel_y--;
             sel_x = entry->cols - 1;
          }
        num = (sel_y * entry->cols) + sel_x;
        if (num < 0)
          {
             EINA_LIST_FOREACH(flatlist, l, subentry)
               {
                  if (subentry == entry)
                    {
                       if (l->prev)
                         {
                            subentry = l->prev->data;
                            entry->sel = EINA_FALSE;
                            subentry->sel = EINA_TRUE;
                            if (sel_x < subentry->cols)
                              subentry->sel_x = sel_x;
                            else
                              subentry->sel_x = subentry->cols - 1;
                            subentry->sel_y = subentry->rows - 1;
                            num = eina_list_count(subentry->files) - 1 -
                              ((subentry->sel_y * subentry->cols) +
                               subentry->sel_x);
                            if (num < 0) subentry->sel_x += num;
                            evas_object_geometry_get(subentry->table, &tbx, &tby, NULL, NULL);
                            o = _sel_object_find(entry);
                            if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
                            o = _sel_object_find(subentry);
                            if (o) elm_layout_signal_emit(o, "rage,state,selected", "rage");
                            elm_scroller_region_bring_in
                              (sc,
                               (tbx - bxx) + (subentry->sel_x * subentry->iw),
                               (tby - bxy) + (subentry->sel_y * subentry->ih),
                               entry->iw, entry->ih);
                         }
                       break;
                    }
               }
          }
        else if (num >= (int)eina_list_count(entry->files))
          {
             EINA_LIST_FOREACH(flatlist, l, subentry)
               {
                  if (subentry == entry)
                    {
                       if (l->next)
                         {
                            subentry = l->next->data;
                            entry->sel = EINA_FALSE;
                            subentry->sel = EINA_TRUE;
                            if (sel_x < subentry->cols)
                              subentry->sel_x = sel_x;
                            else
                              subentry->sel_x = subentry->cols - 1;
                            subentry->sel_y = 0;
                            num = eina_list_count(subentry->files) - 1 -
                              ((subentry->sel_y * subentry->cols) +
                               subentry->sel_x);
                            if (num < 0) subentry->sel_x += num;
                            evas_object_geometry_get(subentry->table, &tbx, &tby, NULL, NULL);
                            o = _sel_object_find(entry);
                            if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
                            o = _sel_object_find(subentry);
                            if (o) elm_layout_signal_emit(o, "rage,state,selected", "rage");
                            elm_scroller_region_bring_in
                              (sc,
                               (tbx - bxx) + (subentry->sel_x * subentry->iw),
                               (tby - bxy) + (subentry->sel_y * subentry->ih),
                               entry->iw, entry->ih);
                         }
                       break;
                    }
               }
          }
        else
          {
             o = _sel_object_find(entry);
             if (o) elm_layout_signal_emit(o, "rage,state,unselected", "rage");
             entry->sel_x = sel_x;
             entry->sel_y = sel_y;
             evas_object_geometry_get(entry->table, &tbx, &tby, NULL, NULL);
             o = _sel_object_find(entry);
             if (o) elm_layout_signal_emit(o, "rage,state,selected", "rage");
             elm_scroller_region_bring_in
               (sc,
                (tbx - bxx) + (entry->sel_x * entry->iw),
                (tby - bxy) + (entry->sel_y * entry->ih),
                entry->iw, entry->ih);
          }
        eina_lock_release(&(entry->lock));
        eina_list_free(flatlist);
     }
   entry = _sel_find(base_entry);
   if (entry)
     {
        file = _sel_file_find(entry);
        if (file)
          {
             char buf[PATH_MAX];
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (selfile) free(selfile);
             selfile = strdup(buf);
          }
     }
}

static void
_sel_do(Evas_Object *win, Entry *base_entry)
{
   Entry *entry;

   if (!base_entry) return;
   entry = _sel_find(base_entry);
   if (entry)
     {
        eina_lock_take(&(entry->lock));
        Evas_Object *o = _sel_object_find(entry);
        const char *file = _sel_file_find(entry);
        if (file)
          {
             char buf[PATH_MAX];

             elm_layout_signal_emit(o, "rage,state,selected", "rage");
             _activate(win, entry, file);
             snprintf(buf, sizeof(buf), "%s/%s", entry->path, file);
             if (selfile) free(selfile);
             selfile = strdup(buf);
         }
        eina_lock_release(&(entry->lock));
     }
}

static void
_cb_key_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   Evas_Object *win = data;
   if ((!strcmp(ev->key, "Left")) ||
       (!strcmp(ev->key, "bracketleft")))
     {
        _sel_go(win, dir_entry, -1, 0);
     }
   else if ((!strcmp(ev->key, "Right")) ||
            (!strcmp(ev->key, "bracketright")))
     {
        _sel_go(win, dir_entry, 1, 0);
     }
   else if ((!strcmp(ev->key, "Up")) ||
            (!strcmp(ev->key, "Prior")) ||
            (!strcmp(ev->key, "XF86AudioPrev")))
     {
        _sel_go(win, dir_entry, 0, -1);
     }
   else if ((!strcmp(ev->key, "Down")) ||
            (!strcmp(ev->key, "Next")) ||
            (!strcmp(ev->key, "XF86AudioNext")))
     {
        _sel_go(win, dir_entry, 0, 1);
     }
   else if ((!strcmp(ev->key, "space")) ||
            (!strcmp(ev->key, "Pause")) ||
            (!strcmp(ev->keyname, "p")) ||
            (!strcmp(ev->key, "XF86AudioPlay")) ||
            (!strcmp(ev->key, "Return")) ||
            (!strcmp(ev->key, "KP_Enter")))
     {
        _sel_do(win, dir_entry);
     }
   else if ((!strcmp(ev->keyname, "q")) ||
            (!strcmp(ev->key, "Escape")) ||
            (!strcmp(ev->key, "e")))
     {
        browser_hide(win);
     }
   else key_handle(win, ev);
}

void
browser_show(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (!bx)
     {
        bx = elm_box_add(win);

        sc = elm_scroller_add(win);
        elm_object_style_set(sc, "noclip");
        elm_object_focus_allow_set(sc, EINA_FALSE);
        evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_scroller_content_min_limit(sc, EINA_TRUE, EINA_FALSE);

        bx = elm_box_add(win);
        elm_object_focus_allow_set(bx, EINA_FALSE);
        evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.5);
        elm_object_content_set(sc, bx);
        evas_object_show(bx);

        elm_object_part_content_set(inf->lay, "rage.browser", sc);
        evas_object_show(sc);

        _fill(win);

        // a dummy button to collect key events and have focus
        bt = elm_button_add(win);
        elm_object_focus_highlight_style_set(bt, "blank");
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add(win, bt);
        evas_object_lower(bt);
        evas_object_show(bt);
        elm_object_focus_set(bt, EINA_TRUE);
        evas_object_event_callback_add(bt, EVAS_CALLBACK_KEY_DOWN,
                                       _cb_key_down, win);
     }
   elm_layout_signal_emit(inf->lay, "browser,state,visible", "rage");
}

static void
_cb_hidden(void *data, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   elm_layout_signal_callback_del(obj, "browser,state,hidden,finished", "rage",
                                  _cb_hidden);
   if (fill_thread) ecore_thread_cancel(fill_thread);
   if (dir_entry) _entry_free(dir_entry);
   dir_entry = NULL;
   evas_object_del(bx);
   bx = NULL;
   evas_object_del(bt);
   bt = NULL;
   evas_object_del(sc);
   sc = NULL;
   elm_object_focus_next(data, ELM_FOCUS_PREVIOUS);
}

void
browser_hide(Evas_Object *win)
{
   Inf *inf = evas_object_data_get(win, "inf");

   if (!bx) return;
   elm_layout_signal_callback_add(inf->lay, "browser,state,hidden,finished", "rage",
                                  _cb_hidden, win);
   elm_layout_signal_emit(inf->lay, "browser,state,hidden", "rage");
}

void
browser_toggle(Evas_Object *win)
{
   if (bx) browser_hide(win);
   else browser_show(win);
}

void
browser_size_update(Evas_Object *win EINA_UNUSED)
{
   if (!bx) return;
   // XXX: unpop + repop
}
