#include <stdio.h>
#include "ui/layout.h"
#include "util/ini.h"
#include "ui/text.h"

struct layout layout = {
    #define CFGSECTION(w, d...) .w = { d },
    #define CFG(n, type, default) .n = default,
    #include "layout.def"
};

char* mystrdup(const char* s) {
    char* p = malloc(strlen(s)+1);
    if (p) strcpy(p, s);
    return p;
}

static int parse_layout(void * user, const char * section, const char * name, const char * value){
    struct layout * cfg = (struct layout *) user;
    if (0) ;
    #define CFGSECTION(s, d...) else if(strcasecmp(section, #s) == 0){ struct SECTIONSTRUCT(s) * sp = &cfg->s; d }
    #define CFG(n, type, default) if(strcasecmp(name, STRINGIFY(n))==0){ sp->n = type##_FN(value); return 1;} 
    #include "layout.def"
    return 0;
}

int dump_layout(struct layout* cfg, char * filename){
    FILE * stream = fopen(filename, "w");
    if(!stream)
        return 1;

    #define CFGSECTION(s, d...) fprintf(stream, "[" #s "]\n"); if(1){ struct SECTIONSTRUCT(s) * sp = &cfg->s; d; } fprintf(stream, "\n");
    #define CFG(n, type, default) fprintf(stream, "%s=" type##_FMT "\n", STRINGIFY(n), sp->n); 
    #include "layout.def"

    fclose(stream);

    return 0;
}

int load_layout(struct layout* cfg, const char * filename){
    if(ini_parse(filename, parse_layout, cfg) < 0){
        printf("Unable to load layout configuration file: '%s'\n", filename);
        return 1;
    }
    printf("Loaded layout configuration file: '%s'\n", filename);
    return 0;
}

void rect_array_layout(struct rect_array * array_spec, int index, rect_t * rect){
    if(array_spec->tile != 0){
        printf("Unsupported tile format: %d\n", array_spec->tile);
    }
    int index_x = index;
    int index_y = index;

    rect->x = array_spec->x + array_spec->px * index_x;
    rect->y = array_spec->y + array_spec->py * index_y;
    rect->w = array_spec->w;
    rect->h = array_spec->h;
}

void rect_array_origin(struct rect_array * array_spec, rect_t * rect){
    rect->x = 0;
    rect->y = 0;
    rect->w = array_spec->w;
    rect->h = array_spec->h;
}
