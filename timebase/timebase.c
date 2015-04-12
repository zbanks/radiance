#include "timebase/timebase.h"

#include <SDL/SDL_thread.h>

#include "core/err.h"
#include "core/audio.h"

static long time = 0;

static SDL_mutex* updating;

void timebase_init()
{
    updating = SDL_CreateMutex();
    if(!updating) FAIL("Unable to create mutex: %s\n", SDL_GetError());
}

void timebase_del()
{
    SDL_DestroyMutex(updating);
}

void timebase_update(chunk_pt chunk)
{
    time = SDL_GetTicks();
}

long timebase_time()
{
    if(SDL_LockMutex(updating)) FAIL("Unable to lock mutex: %s\n", SDL_GetError());
    long result = time;
    if(SDL_UnlockMutex(updating)) FAIL("Unable to unlock mutex: %s\n", SDL_GetError());

    return result;
}
