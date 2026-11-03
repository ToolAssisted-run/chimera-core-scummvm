/* chimera-fs.h - the machine's files: the game's folder, read out of the zip
 * Chimera mounted, and nothing else.
 *
 * A project gives the game as a zip of its folder (the owner's decision of
 * 2026-11-03). The zip is opened once, at Init, and its central directory
 * made into a tree of folders and files; ScummVM sees that tree under
 * /game, through its own FSNode interface, exactly as it would see a folder
 * on a disk. A file is read where it lies in the zip when an engine opens it
 * (an uncompressed member is read in place; a compressed one is inflated as
 * it is read), so a game of hundreds of megabytes costs the machine's
 * memory only what the engine holds open.
 *
 * There is no other path: no host folder, no current directory, no
 * configuration file. Saved games are the save manager's (chimera-saves.h).
 */
#ifndef CHIMERA_FS_H
#define CHIMERA_FS_H

#include "backends/fs/abstract-fs.h"
#include "backends/fs/fs-factory.h"
#include "common/array.h"
#include "common/str.h"

namespace Common {
class SeekableReadStream;
}

/* the folder ScummVM is told the game is in */
#define CHIMERA_GAME_PATH "/game"

struct ChimeraZipEntry {
	Common::String name;  // this entry's own name (the last part of its path)
	Common::String path;  // its whole path in the machine: /game/...
	bool dir;
	int parent;           // index of the folder holding it; -1 for the root
	Common::Array<int> children;  // sorted by name
	uint16 method;        // 0 stored, 8 deflated
	int64 localHeader;    // where its local header starts in the zip
	int64 compSize, size;
	int64 dataOffset;     // where its bytes start; -1 until asked
};

/* The zip, read once. All of it lives in the machine's memory (it is part of
 * every savestate and of the sealed baseline, being read during Init). */
class ChimeraZip {
public:
	/* opens the mounted file and reads its directory; on failure the reason
	 * is a key of the core's language tables with its argument */
	bool open(const char *mountedName, Common::String &error);
	const ChimeraZipEntry *entry(int i) const { return (i >= 0 && i < (int)_entries.size()) ? &_entries[i] : nullptr; }
	/* the entry at a machine path (case folded, as ScummVM's folders are); -1 */
	int find(const Common::String &path) const;
	int root() const { return 0; }
	int fileCount() const { return _files; }
	int count() const { return (int)_entries.size(); }
	/* a member's bytes, read where they lie (inflated as they are read); null
	 * for a folder or a damaged entry */
	Common::SeekableReadStream *createReadStream(int i);

private:
	bool locate(int i, int64 &offset);
	int addPath(const Common::String &zipPath, bool dir);
	int _fd = -1;
	int _files = 0;
	Common::Array<ChimeraZipEntry> _entries;
	void *_index = nullptr;  // the case-folded path index (chimera-fs.cpp)
};

extern ChimeraZip *g_chimeraZip;

class ChimeraFilesystemFactory final : public FilesystemFactory {
public:
	AbstractFSNode *makeRootFileNode() const override;
	AbstractFSNode *makeCurrentDirectoryFileNode() const override;
	AbstractFSNode *makeFileNodePath(const Common::String &path) const override;
};

#endif
