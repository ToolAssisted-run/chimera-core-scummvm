/* chimera-gl.cpp - the OpenGL the machine has (chimera-gl.h). */
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/textconsole.h"
#include "graphics/opengl/context.h"

#include "chimera-gl.h"

/* OSMesa's entry points, declared rather than included: <GL/osmesa.h>
 * brings Mesa's <GL/gl.h>, which would meet ScummVM's GLAD header. */
extern "C" {
typedef struct osmesa_context *OSMesaContext;
typedef void (*OSMESAproc)();
OSMesaContext OSMesaCreateContextExt(unsigned format, int depthBits, int stencilBits, int accumBits,
                                     OSMesaContext sharelist);
unsigned char OSMesaMakeCurrent(OSMesaContext ctx, void *buffer, unsigned type, int width, int height);
void OSMesaPixelStore(int pname, int value);
OSMESAproc OSMesaGetProcAddress(const char *funcName);
}

static const unsigned kOsmesaBgra = 0x1;        // OSMESA_BGRA: B, G, R, A in memory, as the picture
static const unsigned kGlUnsignedByte = 0x1401;  // GL_UNSIGNED_BYTE
static const int kOsmesaYUp = 0x11;             // OSMESA_Y_UP

static bool g_offered;
static OSMesaContext g_ctx;
static uint32 *g_buffer;
static int g_w, g_h;
typedef void (*GlFinishProc)();
static GlFinishProc g_glFinish;

void chimeraGlSetOffered(bool offered) { g_offered = offered; }
bool chimeraGlOffered() { return g_offered; }

void *chimeraGlProcAddress(const char *name) { return (void *)OSMesaGetProcAddress(name); }

bool chimeraGlScreen(int w, int h) {
	const bool first = g_ctx == nullptr;
	if (first) {
		// 24-bit depth and 8-bit stencil, as a desktop's window has
		g_ctx = OSMesaCreateContextExt(kOsmesaBgra, 24, 8, 0, nullptr);
		if (!g_ctx) {
			warning("chimera: Mesa would not make an OpenGL context");
			return false;
		}
	}
	if (w != g_w || h != g_h || !g_buffer) {
		free(g_buffer);
		g_buffer = (uint32 *)calloc((size_t)w * h, 4);
		g_w = w;
		g_h = h;
	}
	if (!OSMesaMakeCurrent(g_ctx, g_buffer, kGlUnsignedByte, w, h)) {
		warning("chimera: Mesa would not draw on a %dx%d screen", w, h);
		return false;
	}
	// the buffer's first row is the top of the picture, as Chimera's
	OSMesaPixelStore(kOsmesaYUp, 0);
	if (first) {
		g_glFinish = (GlFinishProc)OSMesaGetProcAddress("glFinish");
		OpenGLContext.initialize(OpenGL::kContextGL);
	}
	return true;
}

void chimeraGlRead(Graphics::Surface &dst) {
	if (!g_ctx || !g_buffer)
		return;
	// the drawing is finished, and OSMesa copies what was drawn on the
	// screen since the last time into the buffer
	g_glFinish();
	const int w = MIN<int>(dst.w, g_w), h = MIN<int>(dst.h, g_h);
	for (int y = 0; y < h; y++)
		memcpy(dst.getBasePtr(0, y), g_buffer + (size_t)y * g_w, (size_t)w * 4);
}
