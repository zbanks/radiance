#include "midi/layout.h"
#include "midi/midi.h"
#include "midi/controllers.h"
#include "hits/hit.h"
#include "core/slot.h"

static struct midi_note_hit_map np2_map[16];

void midi_setup_layout(){
    if(controllers_enabled[MIDI_NK2_1].available){
        for(int i = 0; (i < n_slots) && (i < 8); i++){
            midi_connect_param(&slots[i].alpha, MIDI_NK2_1, 176, NK2_S0 + i);
        }
    }

    if(controllers_enabled[MIDI_NK2_2].available){
        for(int i = 0; (i < n_hit_slots) && (i < 8); i++){
            midi_connect_param(&hit_slots[i].alpha, MIDI_NK2_2, 176, NK2_S0 + i);
        }
    }

    memset(np2_map, 0, sizeof(np2_map));
    if(controllers_enabled[MIDI_NP2_1].available){
        for(int i = 0; i < 16; i++){
            np2_map[i].slot_index = (i < 8) ? i : i - 8;
            np2_map[i].device = MIDI_NP2_1;
            np2_map[i].data1 = (i < 8) ? NP2_PAD_NA(i) : NP2_PAD_NB(i-8);
        }
    }
}

int midi_handle_note_event(struct midi_event * event){
    if(!(((event->event & MIDI_EV_STATUS_MASK) == MIDI_EV_NOTE_ON) || ((event->event & MIDI_EV_STATUS_MASK) == MIDI_EV_NOTE_OFF)))
        return 0;

    int r = 0;  

    if(controllers_enabled[MIDI_NP2_1].available){
        for(int i = 0; i < 16; i++){
            if((np2_map[i].device == event->device) && (np2_map[i].data1 == event->data1)){
                printf("Hit found %d %d %x\n", i, np2_map[i].slot_index, np2_map[i].active_hit);
                if((event->event & MIDI_EV_STATUS_MASK) == MIDI_EV_NOTE_ON){
                    if(np2_map[i].active_hit){
                        np2_map[i].active_hit->hit->event(np2_map[i].active_hit, HITEV_NOTE_OFF, 0.);
                        np2_map[i].active_hit = 0;
                    }
                    if((np2_map[i].slot_index < n_hit_slots) && (hit_slots[np2_map[i].slot_index].hit)){
                        np2_map[i].active_hit = hit_slots[np2_map[i].slot_index].hit->start(&hit_slots[np2_map[i].slot_index]);
                        if(np2_map[i].active_hit)
                            np2_map[i].active_hit->hit->event(np2_map[i].active_hit, HITEV_NOTE_ON, event->data2 / 127.0);
                    }
                }else{ // note off
                    if(np2_map[i].active_hit && np2_map[i].active_hit->hit)
                        np2_map[i].active_hit->hit->event(np2_map[i].active_hit, HITEV_NOTE_OFF, event->data2 / 127.0);
                    np2_map[i].active_hit = 0;
                }
                r++;
            }
        }
    }
    return r;
}
