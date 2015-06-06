#include "core/config.h"
#include "core/err.h"
#include "output/flux.h"
#include "output/slice.h"

#include <flux.h>
#include <czmq.h>

#define BROKER_URL "tcp://musicazoo.mit.edu:1365"
//#define BROKER_URL "tcp://localhost:1365"
//
static flux_cli_t * flux_client;

int output_flux_init(){
    if(!config.flux.enabled) return -1;

    flux_client = flux_cli_init(config.flux.broker, config.flux.verbose);
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
    for(int i = 0; i < n_strips; i++){
        int r = flux_cli_id_check(flux_client, strips[i].id_str);
        if(r){
            strips[i].bus &= !OUTPUT_FLUX;
            continue;
        }

        zmsg_t * imsg = zmsg_new();
        zmsg_t * reply = NULL;
        if(!flux_cli_send(flux_client, strips[i].id_str, "INFO", &imsg, &reply)){
            zhash_t * info = zhash_unpack(zmsg_first(reply));
            char * length_str  = zhash_lookup(info, "length");
            if(length_str)
                strips[i].length = atoi(length_str);
            printf("Lux device '%s' with length '%s'\n", (char *) zhash_lookup(info, "id"), (char *) zhash_lookup(info, "length"));
            zhash_destroy(&info);
            n_found++;
            strips[i].bus |= OUTPUT_FLUX;
        }else{
            strips[i].bus &= !OUTPUT_FLUX;
        }
        zmsg_destroy(&reply);
    }
    return n_found;
}

int output_flux_push(output_strip_t * strip, unsigned char * frame, int length){
    if(!flux_client) return -1;

    zmsg_t * fmsg = zmsg_new();
    zmsg_t * reply = NULL;
    zmsg_pushmem(fmsg, frame, length);
    flux_cli_send(flux_client, strip->id_str, "FRAME", &fmsg, &reply);
    zmsg_destroy(&reply);
    return 0;
}

void output_flux_del(){
    flux_cli_del(flux_client);
}
