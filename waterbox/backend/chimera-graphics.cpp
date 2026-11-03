/* chimera-graphics.cpp - the machine's screen (chimera-graphics.h). */
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/pixelformat.h"

#include "chimera-gl.h"
#include "chimera-graphics.h"
#include "chimera-machine.h"

/* the picture Chimera reads: B, G, R, A in memory */
static const Graphics::PixelFormat kArgb(4, 8, 8, 8, 8, 16, 8, 0, 24);

static uint32 argbOf(const byte *rgb) { return 0xFF000000u | ((uint32)rgb[0] << 16) | ((uint32)rgb[1] << 8) | rgb[2]; }

ChimeraGraphicsManager::ChimeraGraphicsManager() {
	memset(_palette, 0, sizeof _palette);
	memset(_cursorPalette, 0, sizeof _cursorPalette);
	memset(_shot.palette, 0, sizeof _shot.palette);
	initSize(320, 200, nullptr);
}

ChimeraGraphicsManager::~ChimeraGraphicsManager() {
	_game.free();
	_overlay.free();
	_cursor.free();
	_shot.game.free();
	_shot.overlay.free();
	free(_cursorMask);
	free(_lut16);
}

/* OpenGL for games: engines that can draw either way ask whether it is
 * there (kFeatureOpenGLForGame), and it is when the renderer setting offers
 * it; an engine that draws only with OpenGL does not ask, or asks only for
 * shaders, and always has it */
bool ChimeraGraphicsManager::hasFeature(OSystem::Feature f) const {
	return f == OSystem::kFeatureCursorPalette || f == OSystem::kFeatureCursorAlpha || f == OSystem::kFeatureCursorMask ||
	       f == OSystem::kFeatureCursorMaskInvert || f == OSystem::kFeatureCursorMaskPaletteXorColorXnor ||
	       f == OSystem::kFeatureShadersForGame || (f == OSystem::kFeatureOpenGLForGame && chimeraGlOffered());
}

/* initGraphics3d asks for OpenGL inside its graphics transaction; any other
 * transaction that sets a size is a 2D one */
bool ChimeraGraphicsManager::setGraphicsMode(int mode, uint flags) {
	_pending3d = (flags & OSystem::kGfxModeRender3d) != 0;
	return true;
}

void ChimeraGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	if (f == OSystem::kFeatureCursorPalette)
		_cursorPaletteEnabled = enable;
}

bool ChimeraGraphicsManager::getFeatureState(OSystem::Feature f) const {
	return f == OSystem::kFeatureCursorPalette ? _cursorPaletteEnabled : false;
}

Common::List<Graphics::PixelFormat> ChimeraGraphicsManager::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> list;
	list.push_back(kArgb);                                                   // the picture's own
	list.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));      // RGBA8888
	list.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));      // ABGR8888
	list.push_back(Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));       // RGB565
	list.push_back(Graphics::PixelFormat(2, 5, 5, 5, 0, 10, 5, 0, 0));       // XRGB1555
	list.push_back(Graphics::PixelFormat::createFormatCLUT8());
	return list;
}

void ChimeraGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	if (width == 0 || height == 0)
		return;
	_render3d = _pending3d;
	// what OpenGL drew is read into the game screen, in the picture's format
	const Graphics::PixelFormat f = _render3d ? kArgb : format ? *format : Graphics::PixelFormat::createFormatCLUT8();
	if (_render3d && !chimeraGlScreen(width, height))
		error("chimera: no OpenGL for a %ux%u screen", width, height);
	if (_game.w != (int)width || _game.h != (int)height || _game.format != f) {
		_game.free();
		_game.create(width, height, f);
	}
	const uint ow = MAX<uint>(width, 320), oh = MAX<uint>(height, 200);
	if (_overlay.w != (int)ow || _overlay.h != (int)oh) {
		_overlay.free();
		_overlay.create(ow, oh, kArgb);
	}
	_screenChangeID++;
}

void ChimeraGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	_game.copyRectToSurface(buf, pitch, x, y, w, h);
}

void ChimeraGraphicsManager::fillScreen(uint32 col) { _game.fillRect(Common::Rect(_game.w, _game.h), col); }

void ChimeraGraphicsManager::fillScreen(const Common::Rect &r, uint32 col) {
	Common::Rect c = r;
	c.clip(Common::Rect(_game.w, _game.h));
	if (!c.isEmpty())
		_game.fillRect(c, col);
}

/* ScummVM's GUI clears the overlay to have the game behind its dialogs: as
 * in ScummVM's SDL backend, clearing puts a copy of the game screen in it
 * (stretched when the overlay is the larger) */
void ChimeraGraphicsManager::clearOverlay() {
	uint32 pal[256];
	for (int i = 0; i < 256; i++)
		pal[i] = argbOf(_palette + i * 3);
	const Graphics::PixelFormat &f = _game.format;
	if (f.bytesPerPixel == 2 && (!_lut16 || _lutFormat != f)) {
		if (!_lut16)
			_lut16 = (uint32 *)malloc(65536 * 4);
		rebuildLut(f, _lut16);
		_lutFormat = f;
	}
	for (int y = 0; y < _overlay.h; y++) {
		uint32 *o = (uint32 *)_overlay.getBasePtr(0, y);
		const int sy = y * _game.h / _overlay.h;
		const byte *row = (const byte *)_game.getBasePtr(0, sy);
		for (int x = 0; x < _overlay.w; x++) {
			const int sx = x * _game.w / _overlay.w;
			if (f.bytesPerPixel == 1)
				o[x] = pal[row[sx]];
			else if (f.bytesPerPixel == 2)
				o[x] = _lut16[((const uint16 *)row)[sx]];
			else
				o[x] = toArgb(f, pal, row + sx * f.bytesPerPixel);
		}
	}
}

void ChimeraGraphicsManager::grabOverlay(Graphics::Surface &surface) const {
	surface.copyRectToSurface(_overlay, 0, 0, Common::Rect(MIN<int>(surface.w, _overlay.w), MIN<int>(surface.h, _overlay.h)));
}

void ChimeraGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlay.copyRectToSurface(buf, pitch, x, y, w, h);
}

bool ChimeraGraphicsManager::showMouse(bool visible) {
	const bool last = _cursorVisible;
	_cursorVisible = visible;
	return last;
}

void ChimeraGraphicsManager::warpMouse(int x, int y) {
	_mouseX = x;
	_mouseY = y;
	// a real pointer that is moved says where it went (SDL sends a motion
	// event); the engine's idea of the pointer follows it
	chimeraMachineMouseWarped(x, y);
}

void ChimeraGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor,
                                            const Graphics::PixelFormat *format, const byte *mask, frac_t scaleX, frac_t scaleY) {
	if (!buf || !w || !h)
		return;
	const Graphics::PixelFormat f = format ? *format : Graphics::PixelFormat::createFormatCLUT8();
	if (_cursor.w != (int)w || _cursor.h != (int)h || _cursor.format != f) {
		_cursor.free();
		_cursor.create(w, h, f);
	}
	_cursor.copyRectToSurface(buf, w * f.bytesPerPixel, 0, 0, w, h);
	free(_cursorMask);
	_cursorMask = nullptr;
	if (mask) {
		_cursorMask = (byte *)malloc(w * h);
		memcpy(_cursorMask, mask, w * h);
	}
	_cursorHotX = hotspotX;
	_cursorHotY = hotspotY;
	_cursorKey = keycolor;
}

void ChimeraGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	memcpy(_cursorPalette + start * 3, colors, num * 3);
	_cursorPaletteEnabled = true;
}

void ChimeraGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	memcpy(_palette + start * 3, colors, num * 3);
}

void ChimeraGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	memcpy(colors, _palette + start * 3, num * 3);
}

static void copySurface(Graphics::Surface &dst, const Graphics::Surface &src) {
	if (dst.w != src.w || dst.h != src.h || dst.format != src.format) {
		dst.free();
		dst.create(src.w, src.h, src.format);
	}
	dst.copyRectToSurface(src, 0, 0, Common::Rect(src.w, src.h));
}

void ChimeraGraphicsManager::updateScreen() {
	if (_render3d)
		chimeraGlRead(_game);
	copySurface(_shot.game, _game);
	for (int i = 0; i < 256; i++)
		_shot.palette[i] = argbOf(_palette + i * 3);
	_shot.overlayVisible = _overlayVisible;
	if (_overlayVisible)
		copySurface(_shot.overlay, _overlay);
	_shot.cursorVisible = _cursorVisible;
	_shot.mouseX = _mouseX;
	_shot.mouseY = _mouseY;
	_shot.shakeX = _shakeX;
	_shot.shakeY = _shakeY;
	_shot.valid = true;
	chimeraMachineScreenUpdated();
}

uint32 ChimeraGraphicsManager::toArgb(const Graphics::PixelFormat &f, const uint32 *pal, const byte *p) const {
	uint32 c;
	switch (f.bytesPerPixel) {
	case 1:
		return pal[*p];
	case 2:
		c = *(const uint16 *)p;
		break;
	case 3:
		c = p[0] | (p[1] << 8) | (p[2] << 16);
		break;
	default:
		c = *(const uint32 *)p;
		break;
	}
	uint8 r, g, b;
	f.colorToRGB(c, r, g, b);
	return 0xFF000000u | ((uint32)r << 16) | ((uint32)g << 8) | b;
}

void ChimeraGraphicsManager::rebuildLut(const Graphics::PixelFormat &f, uint32 *lut16) {
	for (uint32 c = 0; c < 65536; c++) {
		uint8 r, g, b;
		f.colorToRGB(c, r, g, b);
		lut16[c] = 0xFF000000u | ((uint32)r << 16) | ((uint32)g << 8) | b;
	}
}

void ChimeraGraphicsManager::aspect(int &x, int &y) const {
	const int w = _shot.valid ? (_shot.overlayVisible ? _shot.overlay.w : _shot.game.w) : _game.w;
	const int h = _shot.valid ? (_shot.overlayVisible ? _shot.overlay.h : _shot.game.h) : _game.h;
	if (h == 200 || h == 400) {
		x = 4;
		y = 3;
	} else {
		x = w;
		y = h;
	}
}

void ChimeraGraphicsManager::compose(uint32 *out, int maxW, int maxH, int &w, int &h) {
	if (!_shot.valid) {
		w = MIN<int>(_game.w, maxW);
		h = MIN<int>(_game.h, maxH);
		memset(out, 0, (size_t)w * h * 4);
		for (int i = 0; i < w * h; i++)
			out[i] = 0xFF000000u;
		return;
	}
	const Graphics::Surface &src = _shot.overlayVisible ? _shot.overlay : _shot.game;
	w = MIN<int>(src.w, maxW);
	h = MIN<int>(src.h, maxH);
	const int sx = _shot.overlayVisible ? 0 : _shot.shakeX;
	const int sy = _shot.overlayVisible ? 0 : _shot.shakeY;
	const Graphics::PixelFormat &f = src.format;

	if (f.bytesPerPixel == 2 && (!_lut16 || _lutFormat != f)) {
		if (!_lut16)
			_lut16 = (uint32 *)malloc(65536 * 4);
		rebuildLut(f, _lut16);
		_lutFormat = f;
	}
	for (int y = 0; y < h; y++) {
		uint32 *o = out + (size_t)y * w;
		const int ry = y - sy;
		if (ry < 0 || ry >= src.h) {
			for (int x = 0; x < w; x++)
				o[x] = 0xFF000000u;
			continue;
		}
		const byte *row = (const byte *)src.getBasePtr(0, ry);
		for (int x = 0; x < w; x++) {
			const int rx = x - sx;
			if (rx < 0 || rx >= src.w) {
				o[x] = 0xFF000000u;
				continue;
			}
			if (f.bytesPerPixel == 1)
				o[x] = _shot.palette[row[rx]];
			else if (f.bytesPerPixel == 2)
				o[x] = _lut16[((const uint16 *)row)[rx]];
			else if (f == kArgb)
				o[x] = ((const uint32 *)row)[rx] | 0xFF000000u;
			else
				o[x] = toArgb(f, _shot.palette, row + rx * f.bytesPerPixel);
		}
	}

	// the pointer, over whichever screen is shown
	if (!_shot.cursorVisible || !_cursor.w || !_cursor.h)
		return;
	uint32 curPal[256];
	for (int i = 0; i < 256; i++)
		curPal[i] = _cursorPaletteEnabled ? argbOf(_cursorPalette + i * 3) : _shot.palette[i];
	const Graphics::PixelFormat &cf = _cursor.format;
	const int x0 = _shot.mouseX - _cursorHotX, y0 = _shot.mouseY - _cursorHotY;
	for (int cy = 0; cy < _cursor.h; cy++) {
		const int y = y0 + cy;
		if (y < 0 || y >= h)
			continue;
		const byte *row = (const byte *)_cursor.getBasePtr(0, cy);
		for (int cx = 0; cx < _cursor.w; cx++) {
			const int x = x0 + cx;
			if (x < 0 || x >= w)
				continue;
			const byte *p = row + cx * cf.bytesPerPixel;
			uint32 &d = out[(size_t)y * w + x];
			if (_cursorMask) {
				const byte m = _cursorMask[cy * _cursor.w + cx];
				if (m == kCursorMaskTransparent)
					continue;
				if (m == kCursorMaskInvert) {
					d = (d ^ 0x00FFFFFFu) | 0xFF000000u;
					continue;
				}
				if (m == kCursorMaskPaletteXorColorXnor) {
					d = (d ^ toArgb(cf, curPal, p)) | 0xFF000000u;
					continue;
				}
				d = toArgb(cf, curPal, p);
				continue;
			}
			uint32 raw;
			switch (cf.bytesPerPixel) {
			case 1: raw = *p; break;
			case 2: raw = *(const uint16 *)p; break;
			case 3: raw = p[0] | (p[1] << 8) | (p[2] << 16); break;
			default: raw = *(const uint32 *)p; break;
			}
			if (raw == _cursorKey && cf.aBits() == 0)
				continue;
			if (cf.aBits() == 0 || cf.bytesPerPixel == 1) {
				d = toArgb(cf, curPal, p);
				continue;
			}
			uint8 a, r, g, b;
			cf.colorToARGB(raw, a, r, g, b);
			if (a == 0)
				continue;
			const uint32 dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
			const uint32 nr = (r * a + dr * (255 - a)) / 255;
			const uint32 ng = (g * a + dg * (255 - a)) / 255;
			const uint32 nb = (b * a + db * (255 - a)) / 255;
			d = 0xFF000000u | (nr << 16) | (ng << 8) | nb;
		}
	}
}
