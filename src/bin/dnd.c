#include <Elementary.h>
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
_cb_drag_pos(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action)
{
   printf("dnd at %i %i act:%i\n", x, y, action);
}

Eina_Bool
_cb_drop(void *data, Evas_Object *o EINA_UNUSED, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   Eina_Bool inserted = EINA_FALSE;

   if (!ev->data) return EINA_TRUE;
   if (strchr(ev->data, '\n'))
     {
        char *p, *p2, *p3, *tb;
        
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
                  else if (!p2) p3 = p2;
                  if (p2)
                    {
                       strncpy(tb, p, p2 - p);
                       tb[p2 - p] = 0;
                       p = p2;
                       while ((*p) && (isspace(*p))) p++;
                       if (strlen(tb) > 0)
                         {
                            win_video_insert(win, tb);
                            inserted = EINA_TRUE;
                         }
                    }
                  else
                    {
                       strcpy(tb, p);
                       if (strlen(tb) > 0)
                         {
                            win_video_insert(win, tb);
                            inserted = EINA_TRUE;
                         }
                       break;
                    }
               }
             free(tb);
          }
     }
   else
     {
        win_video_insert(win, ev->data);
        inserted = EINA_TRUE;
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
   elm_drop_target_add(tgt,
                       ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE,
                       _cb_drag_enter, win,
                       _cb_drag_leave, win,
                       _cb_drag_pos, win,
                       _cb_drop, win);
}
