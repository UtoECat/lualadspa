#pragma once
// TODO:
// This is editable fileIO implementation. You can change
// FILE* IO to ZIP archinve readonly IO and everything will still
// work, but just in the archive now :p.
// WARNING: this class MUST NOT throw ANYTHING!
#include <string>
#include <filesystem>

namespace fsys {
	using namespace ::std::filesystem;
};

class FileIO {
	private:
	FILE* f = nullptr;
	public:
	FileIO() = default;
	~FileIO() {
		close();
	}
	// FILE* clonning is not supported on some
	// unix systems, and in most non-unix ones
	FileIO(const FileIO&) = delete;
	public:
	using pos_type = decltype(ftell(nullptr));
	using size_type = decltype(fread(nullptr, 0, 0, nullptr));
	enum SeekBase {
		SET = SEEK_SET,
		END = SEEK_END,
		CUR = SEEK_CUR
	};
	public:
	// status
	bool isOpened() const {return !!f;}
	bool hasError() const {return f ? ferror(f) : false;}
	bool isEOF() {return !f || feof(f);}
	operator bool() const {return isOpened() && !hasError();}

	// opening/closing
	void close() {if (f) fclose(f); f = nullptr;}
	bool open(fsys::path path, const char* mode) {
		f = fopen(path.string().c_str(), mode);
		return isOpened();
	}

	// seeking
	pos_type tell() {return ftell(f);}	
	bool seek(pos_type pos, SeekBase base) {
		return fseek(f, pos, (int)base) >= 0;
	}
	void rewind() {
		fseek(f, 0, (int)SeekBase::SET);
		clearerr(f);
	}

	// IO

	// dst space must be not less than itemsz*cnt!
	// returns number of readed elements (in simple case
	// aka two args only, returns 1 in sucess reading, 0 if 
	// EOF or other shit, -1 in case of error (and sets error flag)
	// ((c lib does this by itself!!!1)).
	size_type read(void* dst, size_t itemsz, size_t num = 1) {
		return fread(dst, itemsz, num, f);
	}
	
	// same things aas above, but for writing.
	size_type write(const void* src, size_t itemsz, size_t num = 1) {
		return fwrite(src, itemsz, num, f);
	}

	bool write(const std::string& s) {
		size_type cnt = write(s.c_str(), 1, s.size());
		if (cnt < s.size()) return false;
		return true;
	}

	bool read(std::string& s, size_type newlen = 0) {
		if (newlen) s.resize(newlen);
		size_type cnt = read(s.data(), 1, s.size());
		if (cnt < s.size()) return false;
		return true;
	}

};


