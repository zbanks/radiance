#include "dynamic/object.h"
#include "core/config.h"
#include "core/slot.h"
#include "patterns/pattern.h"

#include <dlfcn.h>

int dynamic_load_so(const char * soname){
    void * library = dlopen(soname, RTLD_NOW);
    if(!library){
        printf(".so %s not  found\n", soname);
        printf("error: %s\n", dlerror());
        return -1;
    }

    pattern_t * dyn_patterns = dlsym(library, "patterns");
    int * n_dyn_patterns_ptr = dlsym(library, "n_patterns");

    if(!n_dyn_patterns_ptr || !dyn_patterns){
        printf("No patterns found in %s\n", soname);
        goto fail;
    }

    int n_dyn_patterns = *n_dyn_patterns_ptr;
    pattern_t ** new_patterns = malloc((n_dyn_patterns + n_patterns) * sizeof(pattern_t *));

    if(!new_patterns) goto fail;
    memcpy(new_patterns, patterns, n_patterns * sizeof(pattern_t *));
    memcpy(new_patterns +  n_patterns, dyn_patterns, n_dyn_patterns * sizeof(pattern_t *));
    pattern_t ** old_patterns = patterns;
    patterns = new_patterns;
    n_patterns += n_dyn_patterns;
    free(old_patterns);

    return 0;
fail:
    //dlclose(library);
    return -1;

}

