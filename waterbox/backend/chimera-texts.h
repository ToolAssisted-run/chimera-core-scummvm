/* chimera-texts.h - what the core says to a person is a key of the
 * package's language tables (languages/<code>.json), and a text with
 * arguments is its key followed by each argument after a 0x1F byte
 * (Chimera's docs/translations.md, "A core's texts"). */
#ifndef CHIMERA_TEXTS_H
#define CHIMERA_TEXTS_H

#include "common/str.h"

#define TEXT_ARG "\x1f"

static inline Common::String chimeraText1(const char *key, const Common::String &arg) {
	return Common::String(key) + TEXT_ARG + arg;
}

#endif
