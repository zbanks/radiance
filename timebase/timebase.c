#include "timebase/timebase.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "core/err.h"
#include "core/audio.h"

static long cur_ms;

static double period_ms_per_mb;
static long pt_ms;
static long pt_mb;

static SDL_mutex* updating;

static enum {
    MANUAL,
    AUTOMATIC,
} timebase_state;

void timebase_init()
{
    updating = SDL_CreateMutex();
    if(!updating) FAIL("Unable to create mutex: %s\n", SDL_GetError());

    pt_ms = 0;
    pt_mb = 0;
    period_ms_per_mb = 60. / 140.;
}

void timebase_del()
{
    SDL_DestroyMutex(updating);
}

void timebase_update(chunk_pt chunk)
{
    if(SDL_LockMutex(updating)) FAIL("Unable to lock mutex: %s\n", SDL_GetError());
    cur_ms = SDL_GetTicks();
    if(SDL_UnlockMutex(updating)) FAIL("Unable to unlock mutex: %s\n", SDL_GetError());
}

static long get_cur_mb()
{
    return pt_mb + (long)((double)(cur_ms - pt_ms) / period_ms_per_mb);
}

void timebase_tap(float alpha)
{
    if(SDL_LockMutex(updating)) FAIL("Unable to lock mutex: %s\n", SDL_GetError());

    pt_mb = get_cur_mb();
    pt_ms = cur_ms;

    long error_mb = ((pt_mb + 1500) % 1000) - 500;
    period_ms_per_mb *= 1000. / (1000 - error_mb * alpha);

    if(SDL_UnlockMutex(updating)) FAIL("Unable to unlock mutex: %s\n", SDL_GetError());
}

long timebase_get()
{
    if(SDL_LockMutex(updating)) FAIL("Unable to lock mutex: %s\n", SDL_GetError());
    long result = get_cur_mb();
    if(SDL_UnlockMutex(updating)) FAIL("Unable to unlock mutex: %s\n", SDL_GetError());

    return result;
}

