/* chimera-graphics.h - the machine's screen.
 *
 * ScummVM's engines draw into a surface the backend keeps (the game screen),
 * the GUI into another (the overlay), and ask for the mouse cursor to be
 * drawn over them. A real screen shows what was there when the engine last
 * called updateScreen(), so that call takes a copy of what is shown (the
 * game screen and its palette, or the overlay, and where the cursor is). At
 * the end of each frame the copy is turned into the BGRA picture Chimera
 * reads; with rendering off (turbo) that last step is skipped and nothing
 * else changes. The copy is the machine's (every savestate has it); the
 * picture is not (it is made again from the copy every frame).
 *
 * The overlay is the size of the game screen (at least 320x200), so a click
 * names the same place whichever is shown.
 */
#ifndef CHIMERA_GRAPHICS_H
#define CHIMERA_GRAPHICS_H

#include "backends/graphics/graphics.h"
#include "graphics/surface.h"

class ChimeraGraphicsManager final : public GraphicsManager {
public:
	ChimeraGraphicsManager();
	~ChimeraGraphicsManager() override;

	bool hasFeature(OSystem::Feature f) const override;
	void setFeatureState(OSystem::Feature f, bool enable) override;
	bool getFeatureState(OSystem::Feature f) const override;

	Graphics::PixelFormat getScreenFormat() const override { return _game.format; }
	Common::List<Graphics::PixelFormat> getSupportedFormats() const override;
	void initSize(uint width, uint height, const Graphics::PixelFormat *format = nullptr) override;
	int getScreenChangeID() const override { return _screenChangeID; }
	void beginGFXTransaction() override {}
	OSystem::TransactionError endGFXTransaction() override { return OSystem::kTransactionSuccess; }
	int16 getHeight() const override { return _game.h; }
	int16 getWidth() const override { return _game.w; }
	void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) override;
	Graphics::Surface *lockScreen() override { return &_game; }
	void unlockScreen() override {}
	void fillScreen(uint32 col) override;
	void fillScreen(const Common::Rect &r, uint32 col) override;
	void updateScreen() override;
	void setShakePos(int shakeXOffset, int shakeYOffset) override { _shakeX = shakeXOffset; _shakeY = shakeYOffset; }
	void setFocusRectangle(const Common::Rect &rect) override {}
	void clearFocusRectangle() override {}

	void showOverlay(bool inGUI) override { _overlayVisible = true; _overlayInGUI = inGUI; }
	void hideOverlay() override { _overlayVisible = false; _overlayInGUI = false; }
	bool isOverlayVisible() const override { return _overlayVisible; }
	Graphics::PixelFormat getOverlayFormat() const override { return _overlay.format; }
	void clearOverlay() override;
	void grabOverlay(Graphics::Surface &surface) const override;
	void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) override;
	int16 getOverlayHeight() const override { return _overlay.h; }
	int16 getOverlayWidth() const override { return _overlay.w; }

	bool showMouse(bool visible) override;
	void warpMouse(int x, int y) override;
	void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor,
	                    const Graphics::PixelFormat *format, const byte *mask, frac_t scaleX, frac_t scaleY) override;
	void setCursorPalette(const byte *colors, uint start, uint num) override;

	void setPalette(const byte *colors, uint start, uint num) override;
	void grabPalette(byte *colors, uint start, uint num) const override;

	/* the driver's side */
	/* the shown screen's size: the overlay's while it is shown */
	int shownWidth() const { return _overlayVisible ? _overlay.w : _game.w; }
	int shownHeight() const { return _overlayVisible ? _overlay.h : _game.h; }
	/* where the pointer is, in the shown screen's pixels */
	void setMousePosition(int x, int y) { _mouseX = x; _mouseY = y; }
	int mouseX() const { return _mouseX; }
	int mouseY() const { return _mouseY; }
	/* the picture of the last updateScreen into out (0xAARRGGBB); its size */
	void compose(uint32 *out, int maxW, int maxH, int &w, int &h);
	/* square pixels unless the game is one of 200 or 400 lines, drawn for a 4:3 screen */
	void aspect(int &x, int &y) const;

private:
	void rebuildLut(const Graphics::PixelFormat &f, uint32 *lut16);
	uint32 toArgb(const Graphics::PixelFormat &f, const uint32 *pal, const byte *p) const;

	Graphics::Surface _game, _overlay, _cursor;
	byte _palette[256 * 3];
	byte _cursorPalette[256 * 3];
	bool _cursorPaletteEnabled = false;
	bool _overlayVisible = false, _overlayInGUI = false;
	bool _cursorVisible = false;
	int _cursorHotX = 0, _cursorHotY = 0;
	uint32 _cursorKey = 0;
	byte *_cursorMask = nullptr;
	int _mouseX = 0, _mouseY = 0;
	int _shakeX = 0, _shakeY = 0;
	int _screenChangeID = 1;

	/* what updateScreen() last showed */
	struct Shot {
		Graphics::Surface game, overlay;
		uint32 palette[256];
		bool overlayVisible = false, cursorVisible = false;
		int mouseX = 0, mouseY = 0, shakeX = 0, shakeY = 0;
		bool valid = false;
	} _shot;
	uint32 *_lut16 = nullptr;   // a 16-bit game screen's colours, for its format
	Graphics::PixelFormat _lutFormat;
};

#endif
