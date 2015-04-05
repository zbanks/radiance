#ifndef __UI_H
#define __UI_H

void ui_init();
void ui_quit();
void ui_update_master();
void ui_update_slot(slot_t* slot);
void ui_render();
int ui_poll();

#endif
