#ifndef IMPULSE_WARS_MAP_H
#define IMPULSE_WARS_MAP_H

#include <errno.h>
#include <string.h>

#include "env.h"
#include "settings.h"

// clang-format off

const char boringLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry boringMap = {
    .layout = boringLayout,
    .columns = 21,
    .rows = 21,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = false,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 5,
};

const char prototypeArenaLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','d','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','W','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','d','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','w','O','O','O','O','D','D','D','D','D','O','O','O','O','w','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','W','W','O','O','O','d','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','D',
    'D','O','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry prototypeArenaMap = {
    .layout = prototypeArenaLayout,
    .columns = 20,
    .rows = 20,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = true,
    .weaponPickups = 6,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 4,
};

const char snipersLayout[] = {
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
};

mapEntry snipersMap = {
    .layout = snipersLayout,
    .columns = 21,
    .rows = 21,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = false,
    .weaponPickups = 6,
    .defaultWeapon = SNIPER_WEAPON,
    .maxSuddenDeathWalls = 4,
};

const char roomsLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','W','O','O','O','W','D','D','D','D','D','W','O','O','O','W','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry roomsMap = {
    .layout = roomsLayout,
    .columns = 21,
    .rows = 21,
    .randFloatingStandardWalls = 3,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 3,
    .hasSetFloatingWalls = false,
    .weaponPickups = 10,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 5,
};

const char xArenaLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','d','O','O','O','O','O','O','O','O','O','d','O','D',
    'D','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','W','W','D','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','D','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','D','O','O','w','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','D','O','O','O','O','O','D','D','W','W','O','O','w','O','D',
    'D','O','w','O','O','W','W','D','O','O','O','O','d','O','O','D','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','D','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','D','W','W','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','D',
    'D','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry xArena = {
    .layout = xArenaLayout,
    .columns = 23,
    .rows = 23,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = true,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 6,
};

const char crossBounceLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','B','B','B','B','O','O','O','O','B','D','D','D','D','B','O','O','O','O','B','B','B','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','B','B','O','O','O','O','O','w','d','O','O','O','O','O','B','B','O','O','B','D',
    'D','O','O','O','B','D','D','O','O','O','O','O','O','O','O','O','O','D','D','B','O','O','O','D',
    'D','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','B','O','O','B','D','O','O','O','O','O','O','O','O','D',
    'D','B','O','O','O','O','O','O','D','D','B','O','O','B','D','D','O','O','O','O','O','O','B','D',
    'D','D','O','O','O','O','O','O','B','B','B','O','O','B','B','B','O','O','O','O','O','O','D','D',
    'D','D','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','D','D',
    'D','D','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','D','D',
    'D','D','O','O','O','O','O','O','B','B','B','O','O','B','B','B','O','O','O','O','O','O','D','D',
    'D','B','O','O','O','O','O','O','D','D','B','O','O','B','D','D','O','O','O','O','O','O','B','D',
    'D','O','O','O','O','O','O','O','O','D','B','O','O','B','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','D',
    'D','O','O','O','B','D','D','O','O','O','O','O','O','O','O','O','O','D','D','B','O','O','O','D',
    'D','B','O','O','B','B','O','O','O','O','O','d','w','O','O','O','O','O','B','B','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','B','B','B','O','O','O','O','B','D','D','D','D','B','O','O','O','O','B','B','B','B','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry crossBounce = {
    .layout = crossBounceLayout,
    .columns = 24,
    .rows = 24,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = true,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,// TODO: make this exploding weapon
    .maxSuddenDeathWalls = 6,
};

const char asteriskArenaLayout[]= {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','D','W','O','O','O','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','O','O','O','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','D','D','O','O','O','O','O','O','O','O','O','D','D','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','O','O','O','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','O','O','O','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','D','D','O','O','O','O','O','O','O','O','O','D','D','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','O','O','O','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','D','W','O','O','O','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

mapEntry asteriskArena = {
    .layout = asteriskArenaLayout,
    .columns = 23,
    .rows = 23,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = false,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 7,
};

const char foamPitLayout[] = {
    'B','B','B','W','W','W','D','D','D','B','B','D','D','D','W','W','W','B','B','B',
    'B','O','O','O','O','O','O','O','D','B','B','D','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','B','B','O','O','O','O','O','O','O','O','B',
    'W','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','d','O','O','O','O','d','O','O','O','O','O','O','D',
    'D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D',
    'B','B','B','O','O','O','O','O','O','d','d','O','O','O','O','O','O','B','B','B',
    'B','B','B','O','O','O','O','O','O','d','d','O','O','O','O','O','O','B','B','B',
    'D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D',
    'D','O','O','O','O','O','O','d','O','O','O','O','d','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','W',
    'B','O','O','O','O','O','O','O','O','B','B','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','D','B','B','D','O','O','O','O','O','O','O','B',
    'B','B','B','W','W','W','D','D','D','B','B','D','D','D','W','W','W','B','B','B',
};

mapEntry foamPitMap = {
    .layout = foamPitLayout,
    .columns = 20,
    .rows = 20,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = true,
    .weaponPickups = 6,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 5,
};

const char siegeLayout[] = {
    'B','B','B','W','W','W','W','W','D','D','D','D','D','D','D','D','D','W','W','W','W','W','B','B','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','B',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','W','O','O','O','W','D','D','W','W','O','O','O','O','O','W','W','D','D','W','O','O','O','W','W',
    'W','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','W',
    'W','O','O','O','b','O','W','O','b','O','O','O','O','O','O','O','b','O','W','O','b','O','O','O','W',
    'W','O','b','O','O','O','W','O','O','O','O','O','O','O','O','O','O','O','W','O','O','O','b','O','W',
    'W','O','O','O','O','O','W','O','O','O','O','B','B','B','O','O','O','O','W','O','O','O','O','O','W',
    'W','W','O','O','O','W','W','O','O','O','O','O','O','O','O','O','O','O','W','W','O','O','O','W','W',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','b','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'W','W','O','O','O','W','W','O','O','O','O','O','O','O','O','O','O','O','W','W','O','O','O','W','W',
    'W','O','O','O','O','O','W','O','O','O','O','B','B','B','O','O','O','O','W','O','O','O','O','O','W',
    'W','O','b','O','O','O','W','O','O','O','O','O','O','O','O','O','O','O','W','O','O','O','b','O','W',
    'W','O','O','O','b','O','W','O','b','O','O','O','O','O','O','O','b','O','W','O','b','O','O','O','W',
    'W','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','W',
    'W','W','O','O','O','W','D','D','W','W','O','O','O','O','O','W','W','D','D','W','O','O','O','W','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'B','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','B','B','W','W','W','W','W','D','D','D','D','D','D','D','D','D','W','W','W','W','W','B','B','B',
};

mapEntry siegeMap = {
    .layout = siegeLayout,
    .columns = 25,
    .rows = 24,
    .randFloatingStandardWalls = 0,
    .randFloatingBouncyWalls = 0,
    .randFloatingDeathWalls = 0,
    .hasSetFloatingWalls = true,
    .weaponPickups = 5,
    .defaultWeapon = STANDARD_WEAPON,
    .maxSuddenDeathWalls = 8,
};

// clang-format on

mapEntry *maps[] = {
    &boringMap,
    &prototypeArenaMap,
    &snipersMap,
    &roomsMap,
    &xArena,
    &crossBounce,
    &asteriskArena,
    &foamPitMap,
    &siegeMap,
};

void resetMap(iwEnv *e) {
    // if sudden death walls were placed, remove them
    if (e->suddenDeathWallsPlaced) {
        e->suddenDeathWallsPlaced = false;
        DEBUG_LOG("removing sudden death walls");
        // remove walls from the end of the array, sudden death walls
        // are added last
        for (int16_t i = wall_soa_size(&e->walls) - 1; i >= 0; i--) {
            wallEntity *wall = wall_soa_get(&e->walls, i);
            if (wall == NULL) continue;
            if (!wall->isSuddenDeath) {
                // if we reached the first non sudden death wall, we're done
                break;
            }
            wall_soa_remove(&e->walls, i);
            destroyWall(e, wall, true);
        }
    }

    // place floating walls with a set position if there are any
    const mapEntry *map = maps[e->mapIdx];
    if (!map->hasSetFloatingWalls) {
        return;
    }
    DEBUG_LOG("placing set floating walls");

    const uint8_t columns = map->columns;
    const uint8_t rows = map->rows;
    const char *layout = map->layout;
    uint16_t cellIdx = 0;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            char cellType = layout[col + (row * columns)];

            enum entityType wallType;
            switch (cellType) {
            case 'w':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'b':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'd':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                cellIdx++;
                continue;
            }

            const mapCell *cell = cell_soa_get(&e->cells, cellIdx);
            createWall(e, cell->pos, FLOATING_WALL_THICKNESS, FLOATING_WALL_THICKNESS, cellIdx, wallType, true);
            cellIdx++;
        }
    }
}

void setupMap(iwEnv *e, const uint8_t mapIdx) {
    // reset the map if we're switching to the same map
    if (e->mapIdx == mapIdx) {
        resetMap(e);
        return;
    }

    // Destroy walls and clear buffers when switching maps
    if (e->mapIdx != -1) {
        for (size_t i = 0; i < wall_soa_size(&e->walls); i++) {
            wallEntity *wall = wall_soa_get(&e->walls, i);
            if (wall == NULL) continue;
            destroyWall(e, wall, false);
        }

        wall_soa_remove_all(&e->walls);
        cell_soa_remove_all(&e->cells);
        e->suddenDeathWallsPlaced = false;

        // Clear buffers to prevent dangling pointer access
        memset(e->cells.entities, 0, sizeof(e->cells.entities));
        memset(e->walls.entities, 0, sizeof(e->walls.entities));
        memset(e->projectiles.entities, 0, sizeof(e->projectiles.entities));
        memset(e->explodingProjectiles.entities, 0, sizeof(e->explodingProjectiles.entities));
    }

    const uint8_t columns = maps[mapIdx]->columns;
    const uint8_t rows = maps[mapIdx]->rows;
    const char *layout = maps[mapIdx]->layout;

    e->mapIdx = mapIdx;
    e->map = maps[mapIdx];
    e->defaultWeapon = weaponInfos[maps[mapIdx]->defaultWeapon];
    if (e->isTraining && randFloat(&e->randState, 0.0f, 1.0f) < 0.25f) {
        e->defaultWeapon = weaponInfos[randInt(&e->randState, 0, NUM_WEAPONS - 1)];
    }

    uint16_t cellIdx = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            char cellType = layout[col + (row * columns)];
            enum entityType wallType;
            const float x = (col - ((columns - 1) * 0.5f)) * WALL_THICKNESS;
            const float y = (row - (rows - 1) * 0.5f) * WALL_THICKNESS;

            fsVec2 pos = {.x = x, .y = y};
            mapCell *cell = fastCalloc(1, sizeof(mapCell));
            cell->ent = NULL;
            cell->pos = pos;

            cell_soa_add(&e->cells, cell);

            bool floating = false;
            float thickness = WALL_THICKNESS;
            switch (cellType) {
            case 'O':
                cellIdx++;
                continue;
            case 'w':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'b':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'd':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'D':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cellType);
            }

            entity *ent = createWall(e, pos, thickness, thickness, cellIdx, wallType, floating);
            if (!floating) {
                cell->ent = ent;
            }
        }
    }

    // Pre-calculate map coordinate transforms to avoid divisions in entityPosToCellIdx
    e->mapOriginX = ((float)e->map->columns * WALL_THICKNESS) / 2.0f;
    e->mapOriginY = ((float)e->map->rows * WALL_THICKNESS) / 2.0f;
    e->invWallThickness = 1.0f / WALL_THICKNESS;
}

void computeMapBoundsAndQuadrants(iwEnv *e, mapEntry *map) {
    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < wall_soa_size(&e->walls); i++) {
        const wallEntity *wall = wall_soa_get(&e->walls, i);
        if (wall == NULL) continue;
        bounds.min.x = min(wall->pos.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = min(wall->pos.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = max(wall->pos.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = max(wall->pos.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    map->bounds = bounds;
    map->spawnQuads[0] = (mapBounds){
        .min = (fsVec2){
            .x = map->bounds.min.x + WALL_THICKNESS,
            .y = map->bounds.min.y + WALL_THICKNESS,
        },
        .max = (fsVec2){
            .x = 0.0f,
            .y = 0.0f,
        }
    };
    map->spawnQuads[1] = (mapBounds){
        .min = (fsVec2){
            .x = 0.0f,
            .y = map->bounds.min.y + WALL_THICKNESS,
        },
        .max = (fsVec2){
            .x = map->bounds.max.x - WALL_THICKNESS,
            .y = 0.0f,
        }
    };
    map->spawnQuads[2] = (mapBounds){
        .min = (fsVec2){
            .x = map->bounds.min.x + WALL_THICKNESS,
            .y = 0.0f,
        },
        .max = (fsVec2){
            .x = 0.0f,
            .y = map->bounds.max.y - WALL_THICKNESS,
        }
    };
    map->spawnQuads[3] = (mapBounds){
        .min = (fsVec2){
            .x = 0.0f,
            .y = 0.0f,
        },
        .max = (fsVec2){
            .x = map->bounds.max.x - WALL_THICKNESS,
            .y = map->bounds.max.y - WALL_THICKNESS,
        }
    };
}




bool posValidDroneSpawnPoint(const iwEnv *e, const fsVec2 pos) {
    const uint32_t maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE;
    droneEntity dummyDrone = {.pos = pos};
    const entity ent = {.type = DRONE_ENTITY, .entity = &dummyDrone};
    const enum entityType deathWallType = DEATH_WALL_ENTITY;

    if (isOverlappingCircleInLineOfSight(e, &ent, pos, DRONE_DEATH_WALL_SPAWN_DISTANCE, 0, maskBits, &deathWallType)) {
        return false;
    }
    if (isOverlappingCircleInLineOfSight(e, &ent, pos, DRONE_WALL_SPAWN_DISTANCE, 0, maskBits, NULL)) {
        return false;
    }

    return true;
}

void initMaps() {
    for (uint8_t i = 0; i < NUM_MAPS; i++) {
        mapEntry *map = maps[i];

        // Compute map bounds purely from layout dimensions (no physics needed)
        const uint8_t columns = map->columns;
        const uint8_t rows = map->rows;
        const float halfWidth = (columns - 1) * WALL_THICKNESS / 2.0f;
        const float halfHeight = (rows - 1) * WALL_THICKNESS / 2.0f;

        map->bounds = (mapBounds){
            .min = {.x = -halfWidth, .y = -halfHeight},
            .max = {.x = halfWidth, .y = halfHeight}
        };

        // Compute spawn quadrants from bounds
        map->spawnQuads[0] = (mapBounds){
            .min = (fsVec2){.x = map->bounds.min.x + WALL_THICKNESS, .y = map->bounds.min.y + WALL_THICKNESS},
            .max = (fsVec2){.x = 0.0f, .y = 0.0f}
        };
        map->spawnQuads[1] = (mapBounds){
            .min = (fsVec2){.x = 0.0f, .y = map->bounds.min.y + WALL_THICKNESS},
            .max = (fsVec2){.x = map->bounds.max.x - WALL_THICKNESS, .y = 0.0f}
        };
        map->spawnQuads[2] = (mapBounds){
            .min = (fsVec2){.x = map->bounds.min.x + WALL_THICKNESS, .y = 0.0f},
            .max = (fsVec2){.x = 0.0f, .y = map->bounds.max.y - WALL_THICKNESS}
        };
        map->spawnQuads[3] = (mapBounds){
            .min = (fsVec2){.x = 0.0f, .y = 0.0f},
            .max = (fsVec2){.x = map->bounds.max.x - WALL_THICKNESS, .y = map->bounds.max.y - WALL_THICKNESS}
        };

        // Per-environment arrays are now allocated in setupEnv, not here
        map->droneSpawns = NULL;
        map->packedLayout = NULL;
        map->nearestWalls = NULL;
    }
}

void destroyMaps() {
    // Per-environment arrays are now freed in destroyEnv, not here
    // Global map data (bounds, quadrants) doesn't need cleanup
}

void placeRandFloatingWall(iwEnv *e, const enum entityType wallType) {
    fsVec2 pos;
    if (!findOpenPos(e, FLOATING_WALL_SHAPE, &pos, -1)) {
        ERROR("failed to find open position for floating wall");
    }
    int16_t cellIdx = entityPosToCellIdx(e, pos);
    createWall(e, pos, FLOATING_WALL_THICKNESS, FLOATING_WALL_THICKNESS, cellIdx, wallType, true);
}

void placeRandFloatingWalls(iwEnv *e, const int mapIdx) {
    for (int i = 0; i < maps[mapIdx]->randFloatingStandardWalls; i++) {
        placeRandFloatingWall(e, STANDARD_WALL_ENTITY);
    }
    for (int i = 0; i < maps[mapIdx]->randFloatingBouncyWalls; i++) {
        placeRandFloatingWall(e, BOUNCY_WALL_ENTITY);
    }
    for (int i = 0; i < maps[mapIdx]->randFloatingDeathWalls; i++) {
        placeRandFloatingWall(e, DEATH_WALL_ENTITY);
    }
}

#endif
