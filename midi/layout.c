#include "midi/layout.h"
#include "midi/midi.h"
#include "midi/controllers.h"
#include "core/slot.h"

static struct midi_note_slot_map np2_map[16];

void midi_setup_layout(){
    if(controllers_enabled[MIDI_NK2_1].available){
        for(int i = 0; (i < n_slots) && (i < 8); i++){
            midi_connect_param(&slots[i].alpha, MIDI_NK2_1, 176, NK2_S0 + i);
        }
    }

    memset(np2_map, 0, sizeof(np2_map));
    if(controllers_enabled[MIDI_NP2_1].available){
        for(int i = 0; i < 16; i++){
            np2_map[i].slot_index = (i < 8) ? i : i - 8;
            np2_map[i].device = MIDI_NP2_1;
            np2_map[i].data1 = (i < 8) ? NP2_PAD_NA(i) : NP2_PAD_NB(i-8);
            np2_map[i].note_on_ev = (i < 8) ? PATEV_M1_NOTE_ON : PATEV_M2_NOTE_ON;
            np2_map[i].note_off_ev = (i < 8) ? PATEV_M1_NOTE_OFF : PATEV_M2_NOTE_OFF;
            np2_map[i].aftertouch_ev = (i < 8) ? PATEV_M1_AFTERTOUCH : PATEV_M2_AFTERTOUCH;
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
                //printf("Hit found %d %d %p\n", i, np2_map[i].slot_index, (void *) np2_map[i].active_hit);
                int midi_ev = event->event & MIDI_EV_STATUS_MASK;
                enum pat_event pev;
                if(midi_ev == MIDI_EV_NOTE_ON)
                    pev = np2_map[i].note_on_ev;
                else if(midi_ev == MIDI_EV_NOTE_OFF)
                    pev = np2_map[i].note_off_ev;
                else if(midi_ev == MIDI_EV_CHAN_AT)
                    pev = np2_map[i].aftertouch_ev;
                else continue;

                slot_t * slot = &slots[np2_map[i].slot_index];
                if(!slot->pattern) continue;

                r += slot->pattern->event(slot, pev, event->data2 / 127.0);
            }
        }
    }
    return r;
}
