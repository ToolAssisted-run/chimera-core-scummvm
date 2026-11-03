/* chimera-saves.h - the machine's saved games, kept in its own memory.
 *
 * ScummVM's engines save through a SaveFileManager. This one keeps every
 * file in the machine's memory, so a saved game is part of every savestate
 * and goes back with a rewind, as Chimera's docs/save-data.md asks of a
 * core. Export Save Data writes them out under their names; a project's Save
 * data slot puts files of those names back before the game starts. The
 * files are what ScummVM itself writes into its saves folder (compressed as
 * it compresses them), so they move between this core and ScummVM.
 */
#ifndef CHIMERA_SAVES_H
#define CHIMERA_SAVES_H

#include "common/array.h"
#include "common/savefile.h"
#include "common/str.h"

struct ChimeraSaveFile {
	Common::String name;
	Common::Array<byte> data;
};

class ChimeraSaveFileManager final : public Common::SaveFileManager {
public:
	Common::OutSaveFile *openForSaving(const Common::String &name, bool compress = true) override;
	Common::InSaveFile *openForLoading(const Common::String &name) override;
	Common::InSaveFile *openRawFile(const Common::String &name) override;
	bool removeSavefile(const Common::String &name) override;
	Common::StringArray listSavefiles(const Common::String &pattern) override;
	void updateSavefilesList(Common::StringArray &lockedFiles) override { _locked = lockedFiles; }
	bool exists(const Common::String &name) override;

	/* a file as the project gave it (before the game starts) */
	void put(const Common::String &name, const byte *data, uint32 size);
	/* every file, in name order: what Export Save Data writes */
	const Common::Array<ChimeraSaveFile> &files() const { return _files; }

	/* called by a file being written when it is finished */
	void commit(const Common::String &name, const byte *data, uint32 size);

private:
	int find(const Common::String &name) const;
	bool locked(const Common::String &name) const;
	Common::Array<ChimeraSaveFile> _files;  // sorted by name
	Common::StringArray _locked;
};

#endif
