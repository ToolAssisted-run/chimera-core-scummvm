/* chimera-fs.cpp - the game's folder out of its zip (chimera-fs.h). */
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backends/fs/abstract-fs.h"
#include "common/compression/deflate.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "chimera-fs.h"
#include "chimera-texts.h"

ChimeraZip *g_chimeraZip = nullptr;

static uint16 rd16(const byte *p) { return (uint16)(p[0] | (p[1] << 8)); }
static uint32 rd32(const byte *p) { return (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) | ((uint32)p[3] << 24); }
static uint64 rd64(const byte *p) { return (uint64)rd32(p) | ((uint64)rd32(p + 4) << 32); }

static bool readAt(int fd, int64 at, void *dst, size_t n) {
	byte *d = (byte *)dst;
	while (n) {
		ssize_t got = pread(fd, d, n, (off_t)at);
		if (got <= 0)
			return false;
		d += got;
		at += got;
		n -= (size_t)got;
	}
	return true;
}

/* the case-folded index of every path (ScummVM looks names up without
 * regard to case, as the DOS and Windows games it runs expect) */
typedef Common::HashMap<Common::String, int, Common::IgnoreCase_Hash, Common::IgnoreCase_EqualTo> PathIndex;
#define g_index ((PathIndex *)_index)

int ChimeraZip::find(const Common::String &path) const {
	Common::String p = path;
	while (p.size() > 1 && p.lastChar() == '/')
		p.deleteLastChar();
	if (p.empty())
		p = "/";
	PathIndex::const_iterator it = g_index->find(p);
	return it == g_index->end() ? -1 : it->_value;
}

int ChimeraZip::addPath(const Common::String &zipPath, bool dir) {
	// every folder on the way, made as needed
	int parent = 1;
	Common::String sofar = CHIMERA_GAME_PATH;
	const char *s = zipPath.c_str();
	while (*s) {
		const char *slash = strchr(s, '/');
		Common::String part = slash ? Common::String(s, slash - s) : Common::String(s);
		s = slash ? slash + 1 : s + strlen(s);
		if (part.empty() || part == ".")
			continue;
		sofar += "/" + part;
		const bool isDir = slash != nullptr || dir;
		int found = find(sofar);
		if (found < 0) {
			ChimeraZipEntry e;
			e.name = part;
			e.path = sofar;
			e.dir = isDir;
			e.parent = parent;
			e.method = 0;
			e.localHeader = -1;
			e.compSize = e.size = 0;
			e.dataOffset = -1;
			_entries.push_back(e);
			found = (int)_entries.size() - 1;
			(*g_index)[sofar] = found;
			_entries[parent].children.push_back(found);
		} else if (isDir != _entries[found].dir) {
			// a file and a folder of one name: the zip is not a folder
			return -2;
		}
		parent = found;
	}
	return parent;
}

bool ChimeraZip::open(const char *mountedName, Common::String &error) {
	_index = new PathIndex();
	_entries.clear();
	ChimeraZipEntry root;
	root.name = "";
	root.path = "/";
	root.dir = true;
	root.parent = -1;
	root.method = 0;
	root.localHeader = root.compSize = root.size = root.dataOffset = -1;
	_entries.push_back(root);
	ChimeraZipEntry game = root;
	game.name = "game";
	game.path = CHIMERA_GAME_PATH;
	game.parent = 0;
	_entries.push_back(game);
	_entries[0].children.push_back(1);
	(*g_index)["/"] = 0;
	(*g_index)[CHIMERA_GAME_PATH] = 1;

	_fd = ::open(mountedName, O_RDONLY);
	if (_fd < 0) {
		error = chimeraText1("error.game_missing", mountedName);
		return false;
	}
	struct stat st;
	if (fstat(_fd, &st) != 0 || st.st_size < 22) {
		error = chimeraText1("error.not_zip", mountedName);
		return false;
	}
	const int64 fileSize = st.st_size;

	// the end of central directory record, in the last 64 KiB and 22 bytes
	const int64 tailLen = fileSize < 65557 ? fileSize : 65557;
	Common::Array<byte> tail;
	tail.resize((uint)tailLen);
	if (!readAt(_fd, fileSize - tailLen, tail.data(), (size_t)tailLen)) {
		error = chimeraText1("error.not_zip", mountedName);
		return false;
	}
	int64 eocd = -1;
	for (int64 i = tailLen - 22; i >= 0; i--)
		if (rd32(&tail[(uint)i]) == 0x06054b50) {
			eocd = i;
			break;
		}
	if (eocd < 0) {
		error = chimeraText1("error.not_zip", mountedName);
		return false;
	}
	uint64 count = rd16(&tail[(uint)eocd + 10]);
	uint64 cdSize = rd32(&tail[(uint)eocd + 12]);
	uint64 cdOffset = rd32(&tail[(uint)eocd + 16]);
	if (count == 0xFFFF || cdSize == 0xFFFFFFFF || cdOffset == 0xFFFFFFFF) {
		// ZIP64: the locator sits just before the record
		byte loc[20], rec[56];
		const int64 locAt = fileSize - tailLen + eocd - 20;
		if (locAt < 0 || !readAt(_fd, locAt, loc, 20) || rd32(loc) != 0x07064b50 ||
		    !readAt(_fd, (int64)rd64(loc + 8), rec, 56) || rd32(rec) != 0x06064b50) {
			error = chimeraText1("error.not_zip", mountedName);
			return false;
		}
		count = rd64(rec + 32);
		cdSize = rd64(rec + 40);
		cdOffset = rd64(rec + 48);
	}
	if (cdOffset + cdSize > (uint64)fileSize || cdSize > 0x40000000) {
		error = chimeraText1("error.not_zip", mountedName);
		return false;
	}
	Common::Array<byte> cd;
	cd.resize((uint)cdSize);
	if (cdSize && !readAt(_fd, (int64)cdOffset, cd.data(), (size_t)cdSize)) {
		error = chimeraText1("error.not_zip", mountedName);
		return false;
	}

	struct Raw {
		Common::String name;
		uint16 method, flags;
		uint64 comp, size, local;
	};
	Common::Array<Raw> raws;
	size_t at = 0;
	for (uint64 n = 0; n < count; n++) {
		if (at + 46 > cd.size() || rd32(&cd[at]) != 0x02014b50) {
			error = chimeraText1("error.not_zip", mountedName);
			return false;
		}
		const byte *h = &cd[at];
		Raw r;
		r.flags = rd16(h + 8);
		r.method = rd16(h + 10);
		r.comp = rd32(h + 20);
		r.size = rd32(h + 24);
		const uint16 nameLen = rd16(h + 28), extraLen = rd16(h + 30), commentLen = rd16(h + 32);
		r.local = rd32(h + 42);
		if (at + 46 + nameLen + extraLen + commentLen > cd.size()) {
			error = chimeraText1("error.not_zip", mountedName);
			return false;
		}
		r.name = Common::String((const char *)h + 46, nameLen);
		// the ZIP64 extra field holds the 64-bit values the header gave as all ones
		const byte *x = h + 46 + nameLen, *xend = x + extraLen;
		while (x + 4 <= xend) {
			const uint16 id = rd16(x), len = rd16(x + 2);
			if (x + 4 + len > xend)
				break;
			if (id == 0x0001) {
				const byte *v = x + 4, *vend = x + 4 + len;
				if (r.size == 0xFFFFFFFF && v + 8 <= vend) { r.size = rd64(v); v += 8; }
				if (r.comp == 0xFFFFFFFF && v + 8 <= vend) { r.comp = rd64(v); v += 8; }
				if (r.local == 0xFFFFFFFF && v + 8 <= vend) { r.local = rd64(v); v += 8; }
			}
			x += 4 + len;
		}
		at += 46 + nameLen + extraLen + commentLen;
		// backslashes are folder separators in zips made on Windows
		for (uint i = 0; i < r.name.size(); i++)
			if (r.name[i] == '\\')
				r.name.setChar('/', i);
		raws.push_back(r);
	}

	// a zip of one folder (what zipping a folder makes) is that folder
	Common::String top;
	bool oneTop = true, any = false;
	for (const Raw &r : raws) {
		if (r.name.hasPrefix("__MACOSX/"))
			continue;
		const char *slash = strchr(r.name.c_str(), '/');
		if (!slash) {
			oneTop = false;
			break;
		}
		Common::String first(r.name.c_str(), slash - r.name.c_str());
		if (!any) {
			top = first;
			any = true;
		} else if (!first.equals(top)) {
			oneTop = false;
			break;
		}
	}
	const Common::String strip = (oneTop && any) ? top + "/" : Common::String();

	for (const Raw &r : raws) {
		if (r.name.hasPrefix("__MACOSX/"))
			continue;
		Common::String name = r.name;
		if (!strip.empty()) {
			if (!name.hasPrefix(strip))
				continue;
			name = Common::String(name.c_str() + strip.size());
		}
		const bool dir = name.empty() || name.lastChar() == '/';
		if (name.empty())
			continue;
		const int i = addPath(name, dir);
		if (i == -2) {
			error = chimeraText1("error.not_zip", mountedName);
			return false;
		}
		if (dir || i < 2)
			continue;
		if (r.flags & 1) {
			error = chimeraText1("error.zip_encrypted", r.name);
			return false;
		}
		if (r.method != 0 && r.method != 8) {
			error = chimeraText1("error.zip_method", r.name);
			return false;
		}
		ChimeraZipEntry &e = _entries[i];
		e.method = r.method;
		e.compSize = (int64)r.comp;
		e.size = (int64)r.size;
		e.localHeader = (int64)r.local;
		_files++;
	}
	if (_files == 0) {
		error = chimeraText1("error.zip_empty", mountedName);
		return false;
	}
	// the listings in name order, whatever order the zip keeps
	for (ChimeraZipEntry &e : _entries)
		Common::sort(e.children.begin(), e.children.end(), [this](int a, int b) {
			return _entries[a].name.compareTo(_entries[b].name) < 0;
		});
	return true;
}

/* A member's bytes where they lie in the zip, read through a small buffer
 * of its own; a seek costs nothing until the next read. */
class ChimeraZipReadStream final : public Common::SeekableReadStream {
public:
	ChimeraZipReadStream(int fd, int64 base, int64 size) : _fd(fd), _base(base), _size(size) {}
	bool eos() const override { return _eos; }
	bool err() const override { return _err; }
	void clearErr() override { _eos = _err = false; }
	int64 pos() const override { return _pos; }
	int64 size() const override { return _size; }
	bool seek(int64 offset, int whence = SEEK_SET) override {
		int64 to = whence == SEEK_CUR ? _pos + offset : whence == SEEK_END ? _size + offset : offset;
		if (to < 0 || to > _size)
			return false;
		_pos = to;
		_eos = false;
		return true;
	}
	uint32 read(void *dataPtr, uint32 dataSize) override {
		byte *d = (byte *)dataPtr;
		uint32 done = 0;
		while (done < dataSize) {
			if (_pos >= _size) {
				_eos = true;
				break;
			}
			if (_pos < _bufAt || _pos >= _bufAt + _bufLen) {
				const int64 left = _size - _pos;
				// a large read goes straight to the caller
				if (dataSize - done >= sizeof _buf) {
					uint32 n = (uint32)MIN<int64>(dataSize - done, left);
					if (!readAt(_fd, _base + _pos, d + done, n)) {
						_err = true;
						break;
					}
					_pos += n;
					done += n;
					continue;
				}
				_bufAt = _pos;
				_bufLen = (uint32)MIN<int64>(sizeof _buf, left);
				if (!readAt(_fd, _base + _bufAt, _buf, _bufLen)) {
					_bufLen = 0;
					_err = true;
					break;
				}
			}
			uint32 n = (uint32)MIN<int64>(dataSize - done, _bufAt + _bufLen - _pos);
			memcpy(d + done, _buf + (_pos - _bufAt), n);
			_pos += n;
			done += n;
		}
		return done;
	}

private:
	int _fd;
	int64 _base, _size, _pos = 0;
	int64 _bufAt = 0;
	uint32 _bufLen = 0;
	bool _eos = false, _err = false;
	byte _buf[4096];
};

Common::SeekableReadStream *ChimeraZip::createReadStream(int i) {
	if (i < 2 || i >= (int)_entries.size() || _entries[i].dir)
		return nullptr;
	const ChimeraZipEntry &e = _entries[i];
	int64 offset;
	if (!locate(i, offset)) {
		warning("chimera: the zip's entry for %s is damaged", e.path.c_str());
		return nullptr;
	}
	if (e.method == 0)
		return new ChimeraZipReadStream(_fd, offset, e.size);
	return Common::wrapDeflateReadStream(new ChimeraZipReadStream(_fd, offset, e.compSize), DisposeAfterUse::YES, (uint64)e.size);
}

bool ChimeraZip::locate(int i, int64 &offset) {
	ChimeraZipEntry &e = _entries[i];
	if (e.dataOffset < 0) {
		byte lh[30];
		if (!readAt(_fd, e.localHeader, lh, 30) || rd32(lh) != 0x04034b50)
			return false;
		e.dataOffset = e.localHeader + 30 + rd16(lh + 26) + rd16(lh + 28);
	}
	offset = e.dataOffset;
	return true;
}

/* ------------------------------------------------------------ the nodes */

class ChimeraFSNode final : public AbstractFSNode {
public:
	explicit ChimeraFSNode(int index, const Common::String &missingPath = Common::String())
		: _index(index), _missing(missingPath) {}

	bool exists() const override { return _index >= 0; }
	Common::U32String getDisplayName() const override { return Common::U32String(getName()); }
	Common::String getName() const override {
		if (_index < 0) {
			const char *s = strrchr(_missing.c_str(), '/');
			return s ? Common::String(s + 1) : _missing;
		}
		return g_chimeraZip->entry(_index)->name;
	}
	Common::String getPath() const override { return _index < 0 ? _missing : g_chimeraZip->entry(_index)->path; }
	bool isDirectory() const override { return _index >= 0 && g_chimeraZip->entry(_index)->dir; }
	bool isReadable() const override { return _index >= 0; }
	bool isWritable() const override { return false; }

	AbstractFSNode *getChild(const Common::String &name) const override {
		Common::String p = getPath();
		if (p.lastChar() != '/')
			p += "/";
		p += name;
		const int i = g_chimeraZip->find(p);
		return new ChimeraFSNode(i, p);
	}
	AbstractFSNode *getParent() const override {
		if (_index <= 0)
			return nullptr;
		return new ChimeraFSNode(g_chimeraZip->entry(_index)->parent);
	}
	bool getChildren(AbstractFSList &list, ListMode mode, bool hidden) const override {
		if (!isDirectory())
			return false;
		for (int c : g_chimeraZip->entry(_index)->children) {
			const ChimeraZipEntry *e = g_chimeraZip->entry(c);
			if ((mode == Common::FSNode::kListFilesOnly && e->dir) ||
			    (mode == Common::FSNode::kListDirectoriesOnly && !e->dir))
				continue;
			if (!hidden && e->name.hasPrefix("."))
				continue;
			list.push_back(new ChimeraFSNode(c));
		}
		return true;
	}
	Common::SeekableReadStream *createReadStream() override { return _index < 0 ? nullptr : g_chimeraZip->createReadStream(_index); }
	Common::SeekableWriteStream *createWriteStream(bool atomic) override { return nullptr; }
	bool createDirectory() override { return false; }

private:
	int _index;
	Common::String _missing;
};

AbstractFSNode *ChimeraFilesystemFactory::makeRootFileNode() const { return new ChimeraFSNode(0); }

AbstractFSNode *ChimeraFilesystemFactory::makeCurrentDirectoryFileNode() const { return new ChimeraFSNode(1); }

AbstractFSNode *ChimeraFilesystemFactory::makeFileNodePath(const Common::String &path) const {
	if (!g_chimeraZip)
		return new ChimeraFSNode(-1, path);
	const int i = g_chimeraZip->find(path);
	return new ChimeraFSNode(i, path);
}
