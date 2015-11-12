#ifndef _ALBUMART_H__
#define _ALBUMART_H__ 1

void albumart_find(const char *file,
                   const char *artist, const char *album, const char *title,
                   const char *extrastr,
                   void (*fetch_done) (void *data), void *fetch_data);
char *albumart_file_get(const char *file);

#endif
