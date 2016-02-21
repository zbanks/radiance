#include "core/config.h"
#include "core/err.h"
#include "output/flux.h"
#include "output/slice.h"

#if FLUX_ENABLED

#include <flux.h>

static flux_cli_t * flux_client;

int output_flux_init(){
    if(!config.flux.enabled) return -1;

    flux_client = flux_cli_init(config.flux.broker, config.flux.timeout, config.flux.verbose);
    if(flux_client){
        printf("Flux connected\n");
/* for each output strip
*/
        return 0;
    }
    return -1;
}

int output_flux_enumerate(output_strip_t * strips, int n_strips){
    int n_found = 0;
    flux_id_t * ids_available = NULL;
    int n_ids_available = flux_cli_id_list(flux_client, &ids_available);
    for(int j = 0; j < n_ids_available; j++){
        printf("Id: %16s\n", (char *) &ids_available[j]);
    }
    for(int i = 0; i < n_strips; i++){
        //int r = flux_cli_id_check(flux_client, strips[i].id_str);
        int found_id = 0;
        for(int j = 0; j < n_ids_available; j++){
            if(strncmp(ids_available[j], strips[i].id_str, sizeof(flux_id_t)) == 0){
                found_id = 1;
                break;
            }
        }
        if(!found_id){
            strips[i].bus &= !OUTPUT_FLUX;
            continue;
        }

        char * reply = NULL;
        int reply_size = flux_cli_send(flux_client, strips[i].id_str, "LENGTH?", "", 0, &reply);
        if(reply_size >= 0){
            strips[i].length = atoi(reply);
            printf("Lux device '%s' with length '%s'\n", strips[i].id_str, reply);
            n_found++;
            strips[i].bus |= OUTPUT_FLUX;
        }else{
            strips[i].bus &= !OUTPUT_FLUX;
        }
        free(reply);
    }
    free(ids_available);
    return n_found;
}

int output_flux_push(output_strip_t * strip, unsigned char * frame, int length){
    if(!flux_client) return -1;

    char * reply = NULL;
    int reply_size = flux_cli_send(flux_client, strip->id_str, "FRAME", (char *) frame, length, &reply);
    if(reply_size == 2 && memcmp(reply, "OK", 2) == 0){

    }else{
        if(config.flux.verbose) printf("Recieved error response from flux: %.*s\n", reply_size, reply);
    }
    free(reply);
    return 0;
}

void output_flux_del(){
    flux_cli_del(flux_client);
}

#endif
