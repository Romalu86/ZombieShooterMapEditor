#pragma once
// Zombie Shooter 1 MapEdit target profile.
// Proven against the V25 target MapEditZS1.exe (SHA-256 below).
// Keep these constants centralized: type/query bit masks such as 0x800 are NOT
// MAX_VID and must not be mechanically rewritten when this table changes.
#ifndef MAPEDIT_TARGET_ZS1
#define MAPEDIT_TARGET_ZS1 1
#endif
#define MAPEDIT_ZS1_TARGET_SHA256 "3bc833c35030712001d3091157ddb92c5085943489c3168d54758e87ed880ae8"

enum {
    MAPEDIT_MAP_LAYER_COUNT = 21,
    MAPEDIT_MAX_VID = 0x1000,
    MAPEDIT_ZS1_LOGIC_SIZE = 0x870,
    MAPEDIT_ZS1_MAP_SIZE = 0x4B28,
    MAPEDIT_ZS1_MAP_EDIT_SIZE = 0x4C38
};
