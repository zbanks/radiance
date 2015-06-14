#ifndef __UI_LAYOUT_CONSTANTS_H__
#define __UI_LAYOUT_CONSTANTS_H__

enum layout_tiling {
    LAYOUT_TILE_NONE = 0,
    LAYOUT_TILE_N_ACROSS = 1,
    LAYOUT_TILE_N_DOWN = -1,
};

enum layout_alignment {
    LAYOUT_ALIGN_TL = 0,
    LAYOUT_ALIGN_BR = 1,
    LAYOUT_ALIGN_TC = 2,
    LAYOUT_ALIGN_CC = 3,
    LAYOUT_ALIGN_TR = 4,
};

#endif
