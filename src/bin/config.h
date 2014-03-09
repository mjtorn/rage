#ifndef _CONFIG_H__
#define _CONFIG_H__ 1

typedef struct _Config Config;

struct _Config
{
   const char *emotion_engine;
};

void config_init(void);
void config_shutdown(void);
Config *config_get(void);
void config_save(void);

#endif
