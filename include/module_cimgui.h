// module_cimgui.h
#ifndef MODULE_CIMGUI_H
#define MODULE_CIMGUI_H

#include <stdbool.h>
#include <lua.h>

void cimgui_init(void);
void cimgui_new_frame(void);
void cimgui_render(void);
void cimgui_cleanup(void);
void cimgui_call_draw(void);

#endif