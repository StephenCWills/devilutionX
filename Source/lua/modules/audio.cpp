#include "lua/modules/render.hpp"

#include <sol/sol.hpp>

#include "effects.h"
#include "lua/metadoc.hpp"
#include "utils/log.hpp"

namespace devilution {

namespace {

bool IsValidSfx(int16_t psfx)
{
	return psfx >= 0 && psfx <= static_cast<int16_t>(SfxID::LAST);
}

} // namespace

sol::table LuaAudioModule(sol::state_view &lua)
{
	LogDebug("LuaAudioModule: begin");
	sol::table table = lua.create_table();
	SetWithSignature(table,
	    "playSfx", "(id: number)",
	    [](int16_t psfx) { if (IsValidSfx(psfx)) PlaySFX(static_cast<SfxID>(psfx)); });
	SetWithSignature(table,
	    "playSfxLoc", "(id: number, x: number, y: number)",
	    [](int16_t psfx, int x, int y) { if (IsValidSfx(psfx)) PlaySfxLoc(static_cast<SfxID>(psfx), { x, y }); });
	LogDebug("LuaAudioModule: return table");
	return table;
}

} // namespace devilution
