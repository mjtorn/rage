#include <Elementary.h>
#include <Eio.h>
#include "main.h"
#include "win.h"
#include "winvid.h"
#include "winlist.h"
#include "dnd.h"

void
_cb_drag_enter(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
   printf("dnd enter\n");
}

void
_cb_drag_leave(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED)
{
   printf("dnd leave\n");
}

void
_cb_drag_pos(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED, Elm_Xdnd_Action action EINA_UNUSED)
{
   /* printf("dnd at %i %i act:%i\n", x, y, action); */
}

static int
_xtov(char x)
{
   if ((x >= '0') && (x <= '9')) return x - '0';
   if ((x >= 'a') && (x <= 'f')) return 10 + (x - 'a');
   if ((x >= 'A') && (x <= 'F')) return 10 + (x - 'A');
   return 0;
}

static char *
_escape_parse(const char *str)
{
   char *dest = malloc(strlen(str) + 1);
   char *d;
   const char *s;

   for (d = dest, s = str; *s; d++)
     {
        if (s[0] == '%')
          {
             if (s[1] && s[2])
               {
                  *d = (_xtov(s[1]) << 4) | (_xtov(s[2]));
                  s += 3;
               }
             else s++;
          }
        else
          {
             *d = s[0];
             s++;
          }
     }
   *d = 0;
   return dest;
}

struct _recurse_data
{
   Evas_Object *win;
   Eina_Bool inserted;
};

static void
_cb_recurse_end(void *data, Eio_File *f EINA_UNUSED)
{
   struct _recurse_data *d = data;
   if (d->inserted)
     {
        win_video_next(d->win);
        win_list_content_update(d->win);
     }

   free(d);
}

static void
_cb_recurse_error(void *data EINA_UNUSED, Eio_File *f EINA_UNUSED, int error)
{
   printf("error recursing: %d\n", error);
}

static Eina_Bool
_cb_recurse_filter(void *data EINA_UNUSED, Eio_File *f EINA_UNUSED, const Eina_File_Direct_Info *info)
{
   return info->type == EINA_FILE_REG || info->type == EINA_FILE_DIR;
}

static void
_cb_recurse(void *data, Eio_File *f EINA_UNUSED, const Eina_File_Direct_Info *info)
{
   struct _recurse_data *d = data;
   char *path;
   if (info->type == EINA_FILE_DIR)
      return;

   path = _escape_parse(info->path);
   printf("inserting '%s'\n", path);
   win_video_insert(d->win, path);
   d->inserted = EINA_TRUE;
   free(path);
}

static void
_recurse_dir(Evas_Object *win, const char *path)
{
   struct _recurse_data *data = calloc(1, sizeof(*data));
   data->win = win;
   eio_dir_stat_ls(path, _cb_recurse_filter, _cb_recurse, _cb_recurse_end,
                   _cb_recurse_error, data);
}

Eina_Bool
_cb_drop(void *data, Evas_Object *o EINA_UNUSED, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   Eina_Bool inserted = EINA_FALSE;

   if (!ev->data) return EINA_TRUE;
   if (strchr(ev->data, '\n'))
     {
        char *p, *p2, *p3, *tb, *tt;

        tb = malloc(strlen(ev->data) + 1);
        if (tb)
          {
             for (p = ev->data; p;)
               {
                  p2 = strchr(p, '\n');
                  p3 = strchr(p, '\r');
                  if (p2 && p3)
                    {
                       if (p3 < p2) p2 = p3;
                    }
                  if (p2)
                    {
                       strncpy(tb, p, p2 - p);
                       tb[p2 - p] = 0;
                       p = p2;
                       while ((*p) && (isspace(*p))) p++;
                       if (strlen(tb) > 0)
                         {
                            tt = _escape_parse(tb);
                            if (tt)
                              {
                                 if (ecore_file_is_dir(tt))
                                   {
                                      _recurse_dir(win, tt);
                                   }
                                 else
                                   {
                                      win_video_insert(win, tt);
                                      inserted = EINA_TRUE;
                                   }
                                 free(tt);
                              }
                         }
                    }
                  else
                    {
                       strcpy(tb, p);
                       if (strlen(tb) > 0)
                         {
                            tt = _escape_parse(tb);
                            if (tt)
                              {
                                 if (ecore_file_is_dir(tt))
                                   {
                                      _recurse_dir(win, tt);
                                   }
                                 else
                                   {
                                      win_video_insert(win, tt);
                                      inserted = EINA_TRUE;
                                   }
                                 free(tt);
                              }
                         }
                       break;
                    }
               }
             free(tb);
          }
     }
   else
     {
        char *tt = _escape_parse(ev->data);
        if (tt)
          {
             if (ecore_file_is_dir(tt))
               {
                  _recurse_dir(win, tt);
               }
             else
               {
                  win_video_insert(win, tt);
                  inserted = EINA_TRUE;
               }
             free(tt);
          }
     }
   if (inserted)
     {
        win_video_next(win);
        win_list_content_update(win);
     }
   return EINA_TRUE;
}

void
dnd_init(Evas_Object *win, Evas_Object *tgt)
{
   eio_init();
   elm_drop_target_add(tgt,
                       ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE,
                       _cb_drag_enter, win,
                       _cb_drag_leave, win,
                       _cb_drag_pos, win,
                       _cb_drop, win);
}

void
dnd_shutdown(void)
{
   eio_shutdown();
}
