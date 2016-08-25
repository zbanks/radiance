#include <stdio.h>
#include <strings.h>
#include "util/ini.h"
#include "util/config.h"

#define CFGOBJ config
#define CFGOBJ_PATH "util/config.def"
#include "util/config_gen_c.def"
#undef CFGOBJ
#undef CFGOBJ_PATH

struct config config;

#define CFGOBJ params
#define CFGOBJ_PATH "util/params.def"
#include "util/config_gen_c.def"
#undef CFGOBJ
#undef CFGOBJ_PATH

struct params params;

int params_refresh(void) {
    int rc = params_load(&params, config.paths.params_config);
    loglevel = params.debug.loglevel;
    return rc;
}
