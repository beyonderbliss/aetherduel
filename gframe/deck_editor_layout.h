#ifndef DECK_EDITOR_LAYOUT_H
#define DECK_EDITOR_LAYOUT_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <rect.h>

namespace ygo::deck_editor {

enum class LayoutMode : std::uint8_t {
	Compact,
	Standard,
	Wide
};

// Shared logical-coordinate geometry for the native Deck Editor.
// All rectangles are expressed in the same logical units used by Game::Scale().
struct Layout {
	irr::core::recti usableViewport;
	irr::core::recti previewPanel;
	irr::core::recti rightWorkspace;
	irr::core::recti header;
	irr::core::recti workspace;
	irr::core::recti deckPanel;
	irr::core::recti searchPanel;
	irr::core::recti searchFilters;
	irr::core::recti searchSort;
	irr::core::recti searchResults;
	irr::core::recti searchScrollbar;
	LayoutMode mode{ LayoutMode::Standard };
	bool valid{ false };
};

inline Layout Calculate(const irr::core::recti& viewport) {
	Layout layout;
	layout.usableViewport = viewport;

	const irr::s32 width = std::max<irr::s32>(0, viewport.getWidth());
	const irr::s32 height = std::max<irr::s32>(0, viewport.getHeight());
	if(width <= 0 || height <= 0)
		return layout;

	// Initial mode thresholds are logical viewport units and remain tunable
	// until validated on phone, constrained-landscape, and wide-screen devices.
	if(width < 960 || height < 520)
		layout.mode = LayoutMode::Compact;
	else if(width >= 1280)
		layout.mode = LayoutMode::Wide;
	else
		layout.mode = LayoutMode::Standard;

	const irr::s32 outerPadding = 4;
	const irr::s32 gap = 4;
	const irr::s32 left = viewport.UpperLeftCorner.X + outerPadding;
	const irr::s32 top = viewport.UpperLeftCorner.Y + outerPadding;
	const irr::s32 right = std::max(left, viewport.LowerRightCorner.X - outerPadding);
	const irr::s32 bottom = std::max(top, viewport.LowerRightCorner.Y - outerPadding);
	const irr::s32 innerWidth = std::max<irr::s32>(0, right - left);

	const irr::s32 previewWidth = std::clamp(
		static_cast<irr::s32>(std::lround(innerWidth * 0.25f)),
		static_cast<irr::s32>(180),
		static_cast<irr::s32>(320)
	);
	const irr::s32 previewRight = std::min(right, left + previewWidth);
	const irr::s32 workspaceLeft = std::min(right, previewRight + gap);
	const irr::s32 headerHeight = layout.mode == LayoutMode::Compact ? 40 :
		(layout.mode == LayoutMode::Wide ? 48 : 44);
	const irr::s32 headerBottom = std::min(bottom, top + headerHeight);
	const irr::s32 workspaceTop = std::min(bottom, headerBottom + gap);
	const irr::s32 workspaceWidth = std::max<irr::s32>(0, right - workspaceLeft);
	const irr::s32 splitWidth = std::max<irr::s32>(0, workspaceWidth - gap);
	const irr::s32 deckWidth = (splitWidth * 7) / 12;
	const irr::s32 searchLeft = std::min(right, workspaceLeft + deckWidth + gap);

	layout.previewPanel = { left, top, previewRight, bottom };
	layout.rightWorkspace = { workspaceLeft, top, right, bottom };
	layout.header = { workspaceLeft, top, right, headerBottom };
	layout.workspace = { workspaceLeft, workspaceTop, right, bottom };
	layout.deckPanel = { workspaceLeft, workspaceTop, searchLeft, bottom };
	layout.searchPanel = { searchLeft, workspaceTop, right, bottom };

	const irr::s32 filterHeight = std::min<irr::s32>(122, std::max<irr::s32>(0, bottom - workspaceTop));
	const irr::s32 filterBottom = std::min(bottom, workspaceTop + filterHeight);
	const irr::s32 sortTop = std::min(bottom, filterBottom + gap);
	const irr::s32 sortBottom = std::min(bottom, sortTop + 24);
	const irr::s32 resultsTop = std::min(bottom, sortBottom + gap);
	layout.searchFilters = { searchLeft, workspaceTop, right, filterBottom };
	layout.searchSort = { searchLeft, sortTop, right, sortBottom };
	layout.searchResults = { searchLeft, resultsTop, right, bottom };
	layout.searchScrollbar = { std::max(searchLeft, right - 20), resultsTop, right, bottom };
	layout.valid = true;
	return layout;
}

} // namespace ygo::deck_editor

#endif // DECK_EDITOR_LAYOUT_H
