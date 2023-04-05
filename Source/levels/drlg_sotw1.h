/**
 * @file levels/drlg_sotw1.h
 */
#pragma once

#include "engine/world_tile.hpp"
#include "levels/gendung.h"

namespace devilution {

void CreateSOTW1Dungeon(uint32_t rseed, lvl_entry entry);
void LoadPreSOTW1Dungeon(const char *path);
void LoadSOTW1Dungeon(const char *path, Point spawn);

} // namespace devilution
