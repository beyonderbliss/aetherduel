#include "CImageGUISkin.h"
#include <IVideoDriver.h>
#include <ITexture.h>
#include <IGUIElement.h>
#include <IGUISpriteBank.h>
#include "clipRects.h"
#include <algorithm>
#include <cmath>

namespace {

// Draw a procedural rounded rectangle as clipped horizontal spans.
// Unlike draw2DPolygon(), every span accepts Irrlicht's clip rectangle.
void drawRoundedRect(irr::video::IVideoDriver* driver,
					 const irr::core::rect<irr::s32>& rect,
					 irr::s32 radius,
					 irr::video::SColor topColor,
					 irr::video::SColor bottomColor,
					 const irr::core::rect<irr::s32>* clip) {
	if(!driver || rect.getWidth() <= 0 || rect.getHeight() <= 0)
		return;

	const irr::s32 width = rect.getWidth();
	const irr::s32 height = rect.getHeight();
	radius = std::max<irr::s32>(0, std::min(radius, std::min(width, height) / 2));

	for(irr::s32 y = 0; y < height; ++y) {
		irr::s32 inset = 0;
		if(radius > 0) {
			const irr::s32 dy = std::min(y, height - 1 - y);
			if(dy < radius) {
				const irr::f32 dx = std::sqrt(static_cast<irr::f32>(radius * radius - (radius - dy) * (radius - dy)));
				inset = radius - static_cast<irr::s32>(dx);
			}
		}

		if(width <= inset * 2)
			continue;

		const irr::f32 amount = height > 1 ? static_cast<irr::f32>(y) / static_cast<irr::f32>(height - 1) : 0.0f;
		const irr::video::SColor color = topColor.getInterpolated(bottomColor, amount);
		const irr::core::rect<irr::s32> span(
			rect.UpperLeftCorner.X + inset,
			rect.UpperLeftCorner.Y + y,
			rect.LowerRightCorner.X - inset,
			rect.UpperLeftCorner.Y + y + 1);
		driver->draw2DRectangle(color, span, clip);
	}
}

void drawElementStylePatch(irr::video::IVideoDriver* driver,
					   irr::video::ITexture* texture,
					   const irr::core::rect<irr::s32>& dst,
					   const irr::core::rect<irr::s32>& src,
					   const irr::core::rect<irr::s32>& outer,
					   irr::s32 radius,
					   const irr::core::rect<irr::s32>* clip,
					   const irr::video::SColor* colors) {
	if(!driver || !texture || dst.getWidth() <= 0 || dst.getHeight() <= 0 || src.getWidth() <= 0 || src.getHeight() <= 0)
		return;
	if(radius < 0) {
		driver->draw2DImage(texture, dst, src, clip, colors, true);
		return;
	}
	radius = std::max<irr::s32>(0, std::min(radius, std::min(outer.getWidth(), outer.getHeight()) / 2));
	for(irr::s32 y = std::max(dst.UpperLeftCorner.Y, outer.UpperLeftCorner.Y); y < std::min(dst.LowerRightCorner.Y, outer.LowerRightCorner.Y); ++y) {
		const irr::s32 localY = y - outer.UpperLeftCorner.Y;
		irr::s32 inset = 0;
		if(radius > 0) {
			const irr::s32 dy = std::min(localY, outer.getHeight() - 1 - localY);
			if(dy < radius) {
				const irr::f32 dx = std::sqrt(static_cast<irr::f32>(radius * radius - (radius - dy) * (radius - dy)));
				inset = radius - static_cast<irr::s32>(dx);
			}
		}
		irr::s32 left = std::max(dst.UpperLeftCorner.X, outer.UpperLeftCorner.X + inset);
		irr::s32 right = std::min(dst.LowerRightCorner.X, outer.LowerRightCorner.X - inset);
		if(clip) {
			left = std::max(left, clip->UpperLeftCorner.X);
			right = std::min(right, clip->LowerRightCorner.X);
			if(y < clip->UpperLeftCorner.Y || y >= clip->LowerRightCorner.Y) continue;
		}
		if(left >= right) continue;
		const irr::s32 sy = src.UpperLeftCorner.Y + std::min(src.getHeight() - 1, (y - dst.UpperLeftCorner.Y) * src.getHeight() / dst.getHeight());
		const irr::s32 sx1 = src.UpperLeftCorner.X + (left - dst.UpperLeftCorner.X) * src.getWidth() / dst.getWidth();
		const irr::s32 sx2 = src.UpperLeftCorner.X + ((right - dst.UpperLeftCorner.X) * src.getWidth() + dst.getWidth() - 1) / dst.getWidth();
		if(sx1 >= sx2) continue;
		driver->draw2DImage(texture, irr::core::rect<irr::s32>(left, y, right, y + 1),
			irr::core::rect<irr::s32>(sx1, sy, std::min(src.LowerRightCorner.X, sx2), sy + 1), clip, colors, true);
	}
}

} // namespace

namespace irr {
namespace gui {

CImageGUISkin::CImageGUISkin(video::IVideoDriver* driver, IGUISkin* fallbackSkin) {
	VideoDriver = driver;
	FallbackSkin = fallbackSkin;
	FallbackSkin->grab();
}

CImageGUISkin::~CImageGUISkin() {
	FallbackSkin->drop();
	properties.clear();
	custom_colors.clear();
	element_alpha.clear();

}

void CImageGUISkin::setProperty(core::stringw key, core::stringw value) {
	properties.set(key, value);
}
core::stringw CImageGUISkin::getProperty(core::stringw key) {
	core::map<core::stringw, core::stringw>::Node* node = properties.find(key);
	if(node == 0)
		return core::stringw("");
	else return node->getValue();
}
void CImageGUISkin::setCustomColor(ygo::skin::CustomSkinElements key, video::SColor value) {
	custom_colors.set(key, value);
}
video::SColor CImageGUISkin::getCustomColor(ygo::skin::CustomSkinElements key, video::SColor fallback) {
	core::map<ygo::skin::CustomSkinElements, video::SColor>::Node* node = custom_colors.find(key);
	if(node == 0)
		return fallback;
	else return node->getValue();
}
void CImageGUISkin::loadConfig(const SImageGUISkinConfig& config) {
	Config = config;
}

void CImageGUISkin::setElementAlpha(const IGUIElement* element, u8 alpha) {
	if(!element)
		return;
	if(alpha == 255)
		element_alpha.erase(element);
	else
		element_alpha[element] = alpha;
}

u8 CImageGUISkin::getElementAlpha(const IGUIElement* element) const {
	const auto found = element_alpha.find(element);
	return found == element_alpha.end() ? 255 : found->second;
}

void CImageGUISkin::drawRoundedButtonPane(IGUIElement* element, const core::rect<s32>& rect, const core::rect<s32>* clip, u8 state) {
	if(!VideoDriver || !element)
		return;

	const core::dimension2d<s32> size = rect.getSize();
	if(size.Width <= 0 || size.Height <= 0)
		return;

	// Standard buttons use a compact radius derived from their own bounds.
	// The width constraint prevents narrow controls from becoming pill-shaped.
	const s32 radius = std::max<s32>(1, std::min(size.Height / 3, size.Width / 6));
	const u8 alpha = getElementAlpha(element);
	const auto faded = [alpha](video::SColor color) {
		color.setAlpha(static_cast<u32>(color.getAlpha()) * alpha / 255);
		return color;
	};
	const auto insetRect = [](const core::rect<s32>& source, s32 amount) {
		return core::rect<s32>(source.UpperLeftCorner.X + amount,
			source.UpperLeftCorner.Y + amount,
			source.LowerRightCorner.X - amount,
			source.LowerRightCorner.Y - amount);
	};

	const video::SColor dark = faded(getColor(EGDC_3D_DARK_SHADOW));
	const video::SColor shadow = faded(getColor(EGDC_3D_SHADOW));
	const video::SColor face = faded(getColor(EGDC_3D_FACE));
	const video::SColor highlight = faded(getColor(EGDC_3D_HIGH_LIGHT));

	drawRoundedRect(VideoDriver, rect, radius, dark, dark, clip);

	if(size.Width <= 2 || size.Height <= 2)
		return;
	const core::rect<s32> borderRect = insetRect(rect, 1);
	drawRoundedRect(VideoDriver, borderRect, std::max<s32>(0, radius - 1), shadow, shadow, clip);

	if(size.Width <= 4 || size.Height <= 4)
		return;
	const core::rect<s32> faceRect = insetRect(rect, 2);
	if(state == 2) {
		const video::SColor pressed = faded(getColor(EGDC_3D_SHADOW));
		drawRoundedRect(VideoDriver, faceRect, std::max<s32>(0, radius - 2), pressed, face, clip);
	} else if(state == 1) {
		drawRoundedRect(VideoDriver, faceRect, std::max<s32>(0, radius - 2), face, face, clip);
	} else {
		drawRoundedRect(VideoDriver, faceRect, std::max<s32>(0, radius - 2), highlight, face, clip);
	}
}


void CImageGUISkin::drawRoundedSunkenPane(IGUIElement* element, video::SColor bgcolor, bool flat, bool fillBackGround,
										 const core::rect<s32>& rect, const core::rect<s32>* clip, s32 radius) {
	if(!VideoDriver || !element || rect.getWidth() <= 0 || rect.getHeight() <= 0)
		return;

	const u8 alpha = getElementAlpha(element);
	const auto faded = [alpha](video::SColor color) {
		color.setAlpha(static_cast<u32>(color.getAlpha()) * alpha / 255);
		return color;
	};
	const auto insetRect = [](const core::rect<s32>& source, s32 amount) {
		return core::rect<s32>(source.UpperLeftCorner.X + amount,
			source.UpperLeftCorner.Y + amount,
			source.LowerRightCorner.X - amount,
			source.LowerRightCorner.Y - amount);
	};

	// Keep the skin's sunken-pane palette, but render its shell with rounded geometry.
	const video::SColor outer = faded(getColor(flat ? EGDC_3D_SHADOW : EGDC_3D_DARK_SHADOW));
	const video::SColor inner = faded(getColor(EGDC_3D_SHADOW));
	drawRoundedRect(VideoDriver, rect, radius, outer, outer, clip);

	if(rect.getWidth() <= 2 || rect.getHeight() <= 2)
		return;
	const core::rect<s32> innerRect = insetRect(rect, 1);
	drawRoundedRect(VideoDriver, innerRect, std::max<s32>(0, radius - 1), inner, inner, clip);

	if(fillBackGround && rect.getWidth() > 4 && rect.getHeight() > 4) {
		const core::rect<s32> faceRect = insetRect(rect, 2);
		drawRoundedRect(VideoDriver, faceRect, std::max<s32>(0, radius - 2), faded(bgcolor), faded(bgcolor), clip);
	}
}

video::SColor CImageGUISkin::getColor(EGUI_DEFAULT_COLOR color) const {
	return FallbackSkin->getColor(color);
}

void CImageGUISkin::setColor(EGUI_DEFAULT_COLOR which, video::SColor newColor) {
	FallbackSkin->setColor(which, newColor);
}

s32 CImageGUISkin::getSize(EGUI_DEFAULT_SIZE size) const {
	return FallbackSkin->getSize(size);
}

const wchar_t* CImageGUISkin::getDefaultText(EGUI_DEFAULT_TEXT text) const {
	return FallbackSkin->getDefaultText(text);
}

void CImageGUISkin::setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t* newText) {
	FallbackSkin->setDefaultText(which, newText);
}


void CImageGUISkin::setSize(EGUI_DEFAULT_SIZE which, s32 size) {
	FallbackSkin->setSize(which, size);
}

IGUIFont* CImageGUISkin::getFont(EGUI_DEFAULT_FONT defaultFont) const {
	return FallbackSkin->getFont(defaultFont);
}

void CImageGUISkin::setFont(IGUIFont* font, EGUI_DEFAULT_FONT defaultFont) {
	FallbackSkin->setFont(font, defaultFont);
}

IGUISpriteBank* CImageGUISkin::getSpriteBank() const {
	return FallbackSkin->getSpriteBank();
}

void CImageGUISkin::setSpriteBank(IGUISpriteBank* bank) {
	FallbackSkin->setSpriteBank(bank);
}

u32 CImageGUISkin::getIcon(EGUI_DEFAULT_ICON icon) const {
	return FallbackSkin->getIcon(icon);
}

void CImageGUISkin::setIcon(EGUI_DEFAULT_ICON icon, u32 index) {
	FallbackSkin->setIcon(icon, index);
}

void CImageGUISkin::draw3DButtonPaneStandard(IGUIElement* element, const core::rect<s32>& rect, const core::rect<s32>* clip) {
	drawRoundedButtonPane(element, rect, clip, element && !element->isEnabled() ? 1 : 0);
}

void CImageGUISkin::draw3DButtonPaneDisabled(IGUIElement* element,
											 const core::rect<s32>& rect,
											 const core::rect<s32>* clip) {
	if(!VideoDriver)
		return;
	drawRoundedButtonPane(element, rect, clip, 1);
}

void CImageGUISkin::draw3DButtonPanePressed(IGUIElement* element, const core::rect<s32>& rect, const core::rect<s32>* clip) {
	drawRoundedButtonPane(element, rect, clip, 2);
}

void CImageGUISkin::draw3DSunkenPane(IGUIElement* element,
									 video::SColor bgcolor, bool flat, bool fillBackGround,
									 const core::rect<s32>& rect,
									 const core::rect<s32>* clip) {
	if(element->getType() == EGUIET_MENU) {
		if(!Config.MenuPressed.Texture) {
			const s32 radius = std::max<s32>(1, std::min<s32>(6, std::min(rect.getWidth(), rect.getHeight()) / 4));
			drawRoundedSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip, radius);
			return;
		}

		drawElementStyle(Config.MenuPressed, rect, clip);
	} else if(element->getType() == EGUIET_CHECK_BOX) {
		// Round only the checkbox shell; preserve the active skin texture and its separately drawn icon.
		const SImageGUIElementStyle& style = element->isEnabled() ? Config.CheckBox : Config.CheckBoxDisabled;
		if(!style.Texture) {
			const s32 radius = std::max<s32>(1, std::min(rect.getWidth(), rect.getHeight()) / 4);
			drawRoundedSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip, radius);
			return;
		}
		const s32 radius = std::max<s32>(1, std::min(rect.getWidth(), rect.getHeight()) / 4);
		drawElementStyle(style, rect, clip, 0, radius);
	} else if(element->getType() == EGUIET_EDIT_BOX) {
		if(!Config.SunkenPane.Texture) {
			const s32 radius = std::max<s32>(1, std::min(rect.getHeight() / 4, rect.getWidth() / 8));
			drawRoundedSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip, radius);
			return;
		}
		const s32 radius = std::max<s32>(1, std::min(rect.getHeight() / 4, rect.getWidth() / 8));
		if(fillBackGround) {
			video::SColor background = bgcolor;
			background.setAlpha(static_cast<u32>(background.getAlpha()) * getElementAlpha(element) / 255);
			drawRoundedRect(VideoDriver, rect, radius, background, background, clip);
		}
		drawElementStyle(Config.SunkenPane, rect, clip, 0, radius);
	} else if(element->getType() == EGUIET_COMBO_BOX) {
		if(element->isEnabled()) {
			if(!Config.ComboBox.Texture) {
				const s32 radius = std::max<s32>(1, std::min(rect.getHeight() / 4, rect.getWidth() / 8));
				drawRoundedSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip, radius);
				return;
			}

			if(fillBackGround)
				FallbackSkin->draw2DRectangle(element, bgcolor, rect, clip);

			drawElementStyle(Config.ComboBox, rect, clip);
		} else {
			if(!Config.ComboBoxDisabled.Texture) {
				const s32 radius = std::max<s32>(1, std::min(rect.getHeight() / 4, rect.getWidth() / 8));
				drawRoundedSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip, radius);
				return;
			}

			if(fillBackGround)
				FallbackSkin->draw2DRectangle(element, bgcolor, rect, clip);

			drawElementStyle(Config.ComboBoxDisabled, rect, clip);
		}
	} else {
		if(!Config.SunkenPane.Texture) {
			FallbackSkin->draw3DSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip);
			return;
		}

		if(fillBackGround)
			FallbackSkin->draw2DRectangle(element, bgcolor, rect, clip);

		drawElementStyle(Config.SunkenPane, rect, clip);
	}
}

// 1.7 updates by Mamnarock
core::rect<s32> CImageGUISkin::draw3DWindowBackground(IGUIElement* element,
													  bool drawTitleBar, video::SColor titleBarColor,
													  const core::rect<s32>& rect,
													  const core::rect<s32>* clip,
													  core::rect<s32>* checkClientArea) {
	if(!Config.Window.Texture) {
		const s32 radius = std::max<s32>(1, std::min<s32>(8, std::min(rect.getWidth(), rect.getHeight()) / 4));
		const s32 titleHeight = std::min<s32>(rect.getHeight(), std::max<s32>(0, getSize(EGDS_WINDOW_BUTTON_WIDTH) + 4));

		// The client-area query must not paint; CGUIWindow performs the actual draw separately.
		if(checkClientArea) {
			*checkClientArea = rect;
			checkClientArea->UpperLeftCorner.X += 1;
			checkClientArea->UpperLeftCorner.Y += drawTitleBar ? titleHeight : 1;
			checkClientArea->LowerRightCorner.X -= 2;
			checkClientArea->LowerRightCorner.Y -= 2;
		} else {
			drawRoundedSunkenPane(element, getColor(EGDC_3D_FACE), false, true, rect, clip, radius);

			if(drawTitleBar && titleHeight > 0) {
				core::rect<s32> titleRect(rect.UpperLeftCorner.X + 2, rect.UpperLeftCorner.Y + 2,
					rect.LowerRightCorner.X - 2, rect.UpperLeftCorner.Y + titleHeight);
				titleBarColor.setAlpha(static_cast<u32>(titleBarColor.getAlpha()) * getElementAlpha(element) / 255);
				drawRoundedRect(VideoDriver, titleRect, std::max<s32>(0, radius - 1), titleBarColor, titleBarColor, clip);
				if(titleRect.getHeight() > radius) {
					core::rect<s32> squareBottom(titleRect.UpperLeftCorner.X, titleRect.UpperLeftCorner.Y + radius,
						titleRect.LowerRightCorner.X, titleRect.LowerRightCorner.Y);
					VideoDriver->draw2DRectangle(titleBarColor, squareBottom, clip);
				}
			}
		}

		return core::rect<s32>(rect.UpperLeftCorner.X + 2, rect.UpperLeftCorner.Y + 2,
			rect.LowerRightCorner.X - 2, rect.UpperLeftCorner.Y + titleHeight);
	}

	if(checkClientArea) {
		*checkClientArea = rect;
		checkClientArea->UpperLeftCorner.Y = rect.UpperLeftCorner.Y + Config.Window.DstBorder.Top;
		checkClientArea->UpperLeftCorner.X = rect.UpperLeftCorner.X + Config.Window.DstBorder.Left;
		checkClientArea->LowerRightCorner.X = rect.LowerRightCorner.X - Config.Window.DstBorder.Right;
		checkClientArea->LowerRightCorner.Y = rect.LowerRightCorner.Y - Config.Window.DstBorder.Bottom;
	}

	drawElementStyle(Config.Window, rect, clip);

	return core::rect<s32>(rect.UpperLeftCorner.X + Config.Window.DstBorder.Left, rect.UpperLeftCorner.Y, rect.LowerRightCorner.X - Config.Window.DstBorder.Right, rect.UpperLeftCorner.Y + Config.Window.DstBorder.Top);
}

void CImageGUISkin::draw3DMenuPane(IGUIElement* element,
								   const core::rect<s32>& rect,
								   const core::rect<s32>* clip) {
	if(!Config.MenuPane.Texture) {
		const s32 radius = std::max<s32>(1, std::min<s32>(8, std::min(rect.getWidth(), rect.getHeight()) / 4));
		drawRoundedSunkenPane(element, getColor(EGDC_3D_FACE), false, true, rect, clip, radius);
		return;
	}

	drawElementStyle(Config.MenuPane, rect, clip);
}

void CImageGUISkin::draw3DToolBar(IGUIElement* element,
								  const core::rect<s32>& rect,
								  const core::rect<s32>* clip) {
	if(!Config.MenuBar.Texture) {
		const s32 radius = std::max<s32>(1, std::min<s32>(8, std::min(rect.getWidth(), rect.getHeight()) / 4));
		drawRoundedSunkenPane(element, getColor(EGDC_3D_FACE), false, true, rect, clip, radius);
		return;
	}

	drawElementStyle(Config.MenuBar, rect, clip);
}

void CImageGUISkin::draw3DTabButton(IGUIElement* element, bool active,
									const core::rect<s32>& rect, const core::rect<s32>* clip, gui::EGUI_ALIGNMENT alignment) {
	if(active) {
		if(!Config.TabButtonPressed.Texture) {
			const s32 radius = std::max<s32>(1, std::min<s32>(6, std::min(rect.getWidth(), rect.getHeight()) / 4));
			drawRoundedSunkenPane(element, getColor(EGDC_3D_FACE), false, true, rect, clip, radius);
			return;
		}

		drawElementStyle(Config.TabButtonPressed, rect, clip);
	} else {
		if(!Config.TabButton.Texture) {
			drawRoundedButtonPane(element, rect, clip, 0);
			return;
		}

		drawElementStyle(Config.TabButton, rect, clip);
	}
}

void CImageGUISkin::draw3DTabBody(IGUIElement* element, bool border, bool background,
								  const core::rect<s32>& rect, const core::rect<s32>* clip, s32 tabHeight, gui::EGUI_ALIGNMENT alignment) {
	if(!Config.TabBody.Texture) {
		if(!border && !background)
			return;

		if(tabHeight == -1)
			tabHeight = getSize(EGDS_BUTTON_HEIGHT);

		core::rect<s32> bodyRect(rect);
		if(alignment == EGUIA_UPPERLEFT)
			bodyRect.UpperLeftCorner.Y = std::min(bodyRect.LowerRightCorner.Y, bodyRect.UpperLeftCorner.Y + tabHeight + 2);
		else
			bodyRect.LowerRightCorner.Y = std::max(bodyRect.UpperLeftCorner.Y, bodyRect.LowerRightCorner.Y - tabHeight - 2);

		const s32 radius = std::max<s32>(1, std::min<s32>(8, std::min(bodyRect.getWidth(), bodyRect.getHeight()) / 4));
		if(border)
			drawRoundedSunkenPane(element, getColor(EGDC_3D_FACE), false, background, bodyRect, clip, radius);
		else
			drawRoundedRect(VideoDriver, bodyRect, radius, getColor(EGDC_3D_FACE), getColor(EGDC_3D_FACE), clip);
		return;
	}
	if(!border && !background)
		return;

	core::rect<s32> newclip, newrect;
	newclip.LowerRightCorner.set(clip->LowerRightCorner);
	newclip.UpperLeftCorner.set(clip->UpperLeftCorner.X, clip->UpperLeftCorner.Y + tabHeight);

	newrect.LowerRightCorner.set(rect.LowerRightCorner);
	newrect.UpperLeftCorner.set(rect.UpperLeftCorner.X, rect.LowerRightCorner.Y + tabHeight);

	drawElementStyle(Config.TabBody, newclip, &newclip);
}

void CImageGUISkin::drawIcon(IGUIElement* element, EGUI_DEFAULT_ICON icon,
							 const core::vector2di position, u32 starttime, u32 currenttime,
							 bool loop, const core::rect<s32>* clip) {
	video::SColor Color = getColor(gui::EGDC_WINDOW_SYMBOL);

	if(icon == gui::EGDI_CHECK_BOX_CHECKED) {
		if(Config.CheckBoxColor.getAlpha() > 0)
			Color = Config.CheckBoxColor;
	}

	// Patch to make checkbox and radio color come from gui::EGDC_WINDOW_SYMBOL -- Madoc
	if(icon == gui::EGDI_CHECK_BOX_CHECKED || icon == gui::EGDI_RADIO_BUTTON_CHECKED) {
		FallbackSkin->getSpriteBank()->draw2DSprite(
			FallbackSkin->getIcon(icon),
			position,
			clip,
			Color,
			starttime,
			currenttime,
			loop,
			true);
	}

	else FallbackSkin->drawIcon(element, icon, position, starttime, currenttime, loop, clip);
}

void CImageGUISkin::drawHorizontalProgressBar(IGUIElement* element, const core::rect<s32>& rectangle, const core::rect<s32>* clip,
											  f32 filledRatio, video::SColor fillColor, video::SColor emptyColor) {
	if(!Config.ProgressBar.Texture || !Config.ProgressBarFilled.Texture) {
		return;
	}

	// Draw empty progress bar
	drawElementStyle(Config.ProgressBar, rectangle, clip, &emptyColor);

	// Draw filled progress bar on top
	if(filledRatio < 0.0f)
		filledRatio = 0.0f;
	else
		if(filledRatio > 1.0f)
			filledRatio = 1.0f;

	if(filledRatio > 0.0f) {
		s32 filledPixels = (s32)(filledRatio * rectangle.getSize().Width);
		s32 height = rectangle.getSize().Height;

		core::rect<s32> clipRect = clip ? *clip : rectangle;
		if(filledPixels < height) {
			if(clipRect.LowerRightCorner.X > rectangle.UpperLeftCorner.X + filledPixels)
				clipRect.LowerRightCorner.X = rectangle.UpperLeftCorner.X + filledPixels;

			filledPixels = height;
		}

		core::rect<s32> filledRect = core::rect<s32>(
			rectangle.UpperLeftCorner.X,
			rectangle.UpperLeftCorner.Y,
			rectangle.UpperLeftCorner.X + filledPixels,
			rectangle.LowerRightCorner.Y);

		drawElementStyle(Config.ProgressBarFilled, filledRect, &clipRect, &fillColor);
	}
}

void CImageGUISkin::drawElementStyle(const SImageGUIElementStyle& elem, const core::rect<s32>& rect, const core::rect<s32>* clip, video::SColor* pcolor, s32 roundedRadius) {
	core::rect<s32> srcRect;
	core::rect<s32> dstRect;

	core::dimension2du tsize = elem.Texture->getOriginalSize();
	video::ITexture* texture = elem.Texture;

	video::SColor color = elem.Color;
	if(pcolor)
		color = *pcolor;

	video::SColor faceColor = getColor(EGDC_3D_FACE);
	color.setRed((u8)(color.getRed() * faceColor.getRed() / 255));
	color.setGreen((u8)(color.getGreen() * faceColor.getGreen() / 255));
	color.setBlue((u8)(color.getBlue() * faceColor.getBlue() / 255));
	color.setAlpha((u8)(color.getAlpha() * faceColor.getAlpha() / 255));

	video::SColor colors[4] = { color, color, color, color };

	core::dimension2di dstSize = rect.getSize();

	// Scale the border if there is insufficient room
	SImageGUIElementStyle::SBorder dst = elem.DstBorder;
	f32 scale = 1.0f;
	if(dstSize.Width < dst.Left + dst.Right) {
		scale = dstSize.Width / (f32)(dst.Left + dst.Right);
	}
	if(dstSize.Height < dst.Top + dst.Bottom) {
		f32 x = dstSize.Height / (f32)(dst.Top + dst.Bottom);
		if(x < scale) {
			scale = x;
		}
	}

	if(scale < 1.0f) {
		dst.Left = (s32)(dst.Left * scale);
		dst.Right = (s32)(dst.Right * scale);
		dst.Top = (s32)(dst.Top * scale);
		dst.Bottom = (s32)(dst.Bottom * scale);
	}

	const SImageGUIElementStyle::SBorder& src = elem.SrcBorder;

	// Draw the top left corner
	srcRect = core::rect<s32>(0, 0, src.Left, src.Top);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y, rect.UpperLeftCorner.X + dst.Left, rect.UpperLeftCorner.Y + dst.Top);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the top right corner
	srcRect = core::rect<s32>(tsize.Width - src.Right, 0, tsize.Width, src.Top);
	dstRect = core::rect<s32>(rect.LowerRightCorner.X - dst.Right, rect.UpperLeftCorner.Y, rect.LowerRightCorner.X, rect.UpperLeftCorner.Y + dst.Top);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the top border
	srcRect = core::rect<s32>(src.Left, 0, tsize.Width - src.Right, src.Top);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X + dst.Left, rect.UpperLeftCorner.Y, rect.LowerRightCorner.X - dst.Right, rect.UpperLeftCorner.Y + dst.Top);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the left border
	srcRect = core::rect<s32>(0, src.Top, src.Left, tsize.Height - src.Bottom);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y + dst.Top, rect.UpperLeftCorner.X + dst.Left, rect.LowerRightCorner.Y - dst.Bottom);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the right border
	srcRect = core::rect<s32>(tsize.Width - src.Right, src.Top, tsize.Width, tsize.Height - src.Bottom);
	dstRect = core::rect<s32>(rect.LowerRightCorner.X - dst.Right, rect.UpperLeftCorner.Y + dst.Top, rect.LowerRightCorner.X, rect.LowerRightCorner.Y - dst.Bottom);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the middle section
	srcRect = core::rect<s32>(src.Left, src.Top, tsize.Width - src.Right, tsize.Height - src.Bottom);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X + dst.Left, rect.UpperLeftCorner.Y + dst.Top, rect.LowerRightCorner.X - dst.Right, rect.LowerRightCorner.Y - dst.Bottom);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the bottom left corner
	srcRect = core::rect<s32>(0, tsize.Height - src.Bottom, src.Left, tsize.Height);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X, rect.LowerRightCorner.Y - dst.Bottom, rect.UpperLeftCorner.X + dst.Left, rect.LowerRightCorner.Y);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the bottom right corner
	srcRect = core::rect<s32>(tsize.Width - src.Right, tsize.Height - src.Bottom, tsize.Width, tsize.Height);
	dstRect = core::rect<s32>(rect.LowerRightCorner.X - dst.Right, rect.LowerRightCorner.Y - dst.Bottom, rect.LowerRightCorner.X, rect.LowerRightCorner.Y);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);

	// Draw the bottom border
	srcRect = core::rect<s32>(src.Left, tsize.Height - src.Bottom, tsize.Width - src.Right, tsize.Height);
	dstRect = core::rect<s32>(rect.UpperLeftCorner.X + dst.Left, rect.LowerRightCorner.Y - dst.Bottom, rect.LowerRightCorner.X - dst.Right, rect.LowerRightCorner.Y);
	if(!clip || clipRects(dstRect, srcRect, *clip))
		drawElementStylePatch(VideoDriver, texture, dstRect, srcRect, rect, roundedRadius, clip, colors);
}

void CImageGUISkin::draw2DRectangle(IGUIElement* element, const video::SColor &color,
									const core::rect<s32>& pos, const core::rect<s32>* clip) {
	FallbackSkin->draw2DRectangle(element, color, pos, clip);
}

} // namespace gui
} // namespace irr

