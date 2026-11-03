/* chimera-saves.cpp - saved games in the machine's memory (chimera-saves.h). */
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "common/compression/deflate.h"
#include "common/memstream.h"

#include "chimera-saves.h"

/* A file being written: in memory until it is finished, then the manager's.
 * finalize() and the destructor both hand it over, so a file an engine
 * forgets to finalize is kept too (as a file on a disk would be). */
class ChimeraSaveWriter final : public Common::MemoryWriteStreamDynamic {
public:
	ChimeraSaveWriter(ChimeraSaveFileManager *mgr, const Common::String &name)
		: Common::MemoryWriteStreamDynamic(DisposeAfterUse::YES), _mgr(mgr), _name(name) {}
	~ChimeraSaveWriter() override { finalize(); }
	void finalize() override { _mgr->commit(_name, getData(), (uint32)size()); }

private:
	ChimeraSaveFileManager *_mgr;
	Common::String _name;
};

int ChimeraSaveFileManager::find(const Common::String &name) const {
	for (uint i = 0; i < _files.size(); i++)
		if (_files[i].name == name)
			return (int)i;
	return -1;
}

bool ChimeraSaveFileManager::locked(const Common::String &name) const {
	for (const Common::String &l : _locked)
		if (l == name)
			return true;
	return false;
}

void ChimeraSaveFileManager::commit(const Common::String &name, const byte *data, uint32 size) {
	int i = find(name);
	if (i < 0) {
		ChimeraSaveFile f;
		f.name = name;
		// keep the list in name order
		uint at = 0;
		while (at < _files.size() && _files[at].name.compareTo(name) < 0)
			at++;
		_files.insert_at(at, f);
		i = (int)at;
	}
	_files[i].data.resize(size);
	if (size)
		memcpy(_files[i].data.data(), data, size);
}

void ChimeraSaveFileManager::put(const Common::String &name, const byte *data, uint32 size) { commit(name, data, size); }

Common::OutSaveFile *ChimeraSaveFileManager::openForSaving(const Common::String &name, bool compress) {
	if (locked(name)) {
		setError(Common::kWritingFailed, "Savefile '" + name + "' is locked");
		return nullptr;
	}
	// what a new file of this name holds until it is finished: nothing
	commit(name, nullptr, 0);
	Common::WriteStream *w = new ChimeraSaveWriter(this, name);
	return new Common::OutSaveFile(compress ? Common::wrapCompressedWriteStream(w) : w);
}

Common::InSaveFile *ChimeraSaveFileManager::openRawFile(const Common::String &name) {
	const int i = find(name);
	if (i < 0 || locked(name))
		return nullptr;
	const Common::Array<byte> &d = _files[i].data;
	byte *copy = (byte *)malloc(d.size() ? d.size() : 1);
	if (d.size())
		memcpy(copy, d.data(), d.size());
	return new Common::MemoryReadStream(copy, d.size(), DisposeAfterUse::YES);
}

/* as ScummVM's default save manager says it, so an engine that reports
 * the error says the same */
Common::InSaveFile *ChimeraSaveFileManager::openForLoading(const Common::String &name) {
	if (locked(name)) {
		setError(Common::kReadingFailed, Common::String::format("Savefile '%s' is locked and cannot be loaded", name.c_str()));
		return nullptr;
	}
	Common::InSaveFile *raw = openRawFile(name);
	if (!raw) {
		setError(Common::kPathDoesNotExist, Common::String::format("Savefile '%s' does not exist", name.c_str()));
		return nullptr;
	}
	return Common::wrapCompressedReadStream(raw);
}

bool ChimeraSaveFileManager::removeSavefile(const Common::String &name) {
	const int i = find(name);
	if (i < 0 || locked(name)) {
		setError(Common::kPathDoesNotExist, "Failed to remove savefile '" + name + "': it does not exist");
		return false;
	}
	_files.remove_at(i);
	return true;
}

bool ChimeraSaveFileManager::exists(const Common::String &name) { return find(name) >= 0 && !locked(name); }

Common::StringArray ChimeraSaveFileManager::listSavefiles(const Common::String &pattern) {
	Common::StringArray out;
	for (const ChimeraSaveFile &f : _files)
		if (!locked(f.name) && f.name.matchString(pattern, true))
			out.push_back(f.name);
	return out;
}
