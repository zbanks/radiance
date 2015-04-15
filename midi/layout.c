#include "midi/layout.h"
#include "midi/midi.h"
#include "midi/controllers.h"
#include "core/slot.h"

void midi_setup_layout(){
    if(controllers_enabled[MIDI_NK2_1].available){
        for(int i = 0; (i < n_slots) && (i < 8); i++){
            midi_connect_param(&slots[i].alpha, MIDI_NK2_1, 176, NK2_S0 + i);
        }
    }
}
