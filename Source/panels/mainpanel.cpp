#include "panels/mainpanel.hpp"

#include "control.h"
#include "engine/clx_sprite.hpp"
#include "engine/load_clx.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/text_render.hpp"
#include "utils/display.h"
#include "utils/language.h"
#include "utils/sdl_compat.h"
#include "utils/sdl_geometry.h"
#include "utils/stdcompat/optional.hpp"
#include "utils/surface_to_clx.hpp"

namespace devilution {

OptionalOwnedClxSpriteList PanelButtonDown;

namespace {

OptionalOwnedClxSpriteList PanelButton;

void DrawButtonText(const Surface &out, string_view text, Rectangle placement, UiFlags style, int spacing = 1)
{
	DrawString(out, text, { placement.position + Displacement { 0, 1 }, placement.size }, UiFlags::AlignCenter | UiFlags::KerningFitSpacing | UiFlags::ColorBlack, spacing);
	DrawString(out, text, placement, UiFlags::AlignCenter | UiFlags::KerningFitSpacing | style, spacing);
}

void DrawButtonOnPanel(Point position, string_view text)
{
	int spacing = 2;
	int width = std::min<int>(GetLineWidth(text, GameFont12, spacing), 61);
	if (width > 53) {
		spacing = 1;
	}
	DrawButtonText(*pBtmBuff, text, { position, { 61, 0 } }, UiFlags::ColorButtonface, spacing);
}

void RenderMainButton(const Surface &out, int buttonId, string_view text)
{
	DrawButtonOnPanel({ PanBtnPos[buttonId].x, PanBtnPos[buttonId].y + 42 + 3 }, text);

	Point position { 0, 21 * buttonId };
	int spacing = 2;
	int width = std::min<int>(GetLineWidth(text, GameFont12, spacing), 61);
	if (width > 53) {
		spacing = 1;
	}
	DrawButtonText(out, text, { position + Displacement { 1, 4 }, { out.w(), 0 } }, UiFlags::ColorButtonpushed, spacing);
}

} // namespace

void LoadMainPanel()
{
	PanelButton = LoadOptionalClx("ctrlpan\\control_button.clx");

	std::optional<OwnedSurface> out;
	constexpr uint16_t NumButtonSprites = 6;
	{
		out.emplace(61, 21 * 6);
		int y = 0;
		for (uint16_t i = 0; i < NumButtonSprites; i++) {
			RenderClxSprite(*out, (*PanelButton)[0], { 0, y });
			y += 21;
		}
	}

	RenderMainButton(*out, 0, _("char"));
	RenderMainButton(*out, 2, _("map"));
	RenderMainButton(*out, 3, _("menu"));
	RenderMainButton(*out, 4, _("inv"));
	PanelButtonDown = SurfaceToClx(*out, NumButtonSprites);
}

void FreeMainPanel()
{
	PanelButtonDown = std::nullopt;
}

} // namespace devilution
