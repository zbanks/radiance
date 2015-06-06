#include "core/config.h"
#include "output/lux.h"
#include "output/slice.h"
#include "output/serial.h"

#include <string.h>

int output_lux_init(){
    if(!config.lux.enabled) return -1;
    return serial_init();
/* for each output strip 
*/
}

int output_lux_enumerate(output_strip_t * strips, int n_strips){
    struct lux_frame cmd;
    struct lux_frame resp;
    int n_found = 0;
    int r;

    for(int i = 0; i < n_strips; i++){

        if(config.lux.write_only){
            strips[i].bus |= OUTPUT_LUX;
            n_found++;
        }else{
            cmd.data.carray.cmd = CMD_GET_ID;
            cmd.destination = strips[i].id_int;
            cmd.length = 1;

            if((r = lux_command_response(&cmd, &resp, 20))){
                printf("failed cmd get_id: %d\n", r);
                strips[i].bus &= ~OUTPUT_LUX;
            }else{
                printf("Found light strip %d @0x%08x: '%s'\n", i, cmd.destination, resp.data.raw);
                n_found++;
                strips[i].bus |= OUTPUT_LUX;

                cmd.data.carray.cmd = CMD_GET_LENGTH;
                cmd.length = 1;
                if((r = lux_command_response(&cmd, &resp, 20))){
                    printf("failed cmd get_length: %d\n", r);
                }else{
                    if(strips[i].length == resp.data.ssingle_r.data)
                        printf("...length matches: %d\n", resp.data.ssingle_r.data);
                    else
                        printf("...length mis-match! %d in file, %d from strip\n", strips[i].length, resp.data.ssingle_r.data);
                }
            }
        }
    }
    return n_found;
}

int output_lux_push(output_strip_t * strip, unsigned char * frame, int length){
    static struct lux_frame lf;

    lf.data.carray.cmd = CMD_FRAME;
    memcpy(lf.data.carray.data, frame, length);
    lf.destination = strip->id_int;
    lf.length = length + 1;

    return lux_tx_packet(&lf);
}

void output_lux_del(){
    serial_close();
}
