/*
 * LuaLadspa - write your own LADSPA plugins on lua :)
 * Common ladspa function to enumerate, search, load/unload plugins.
 * Copyright (C) 2023 UtoECat

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lualadspa.hpp"

const std::string vstrformat(const char * const fmt, va_list args) {
	va_list args2;
	va_copy(args2, args);
	const int len = std::vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);
	
	if (len <= 0) throw "formatting error";
	auto tmp = std::make_unique<char[]>(len+1);

	std::vsnprintf(tmp.get(), len+1, fmt, args);
	return std::string(tmp.get(), len);
}

const std::string strformat(const char * const fmt, ...) {
	va_list args;
	va_start(args, fmt);

	const std::string res = vstrformat(fmt, args);

	va_end(args);
	return res;
}

FILE* outlog = stdout; 
static volatile bool nooverlog = false;

void logError(const char* message, ...) {
	va_list args;
	va_start(args, message);
	const std::string res = vstrformat(message, args);
	fprintf(outlog, "[Error]: ");
	fwrite(res.c_str(), 1, res.size(), outlog);
	fprintf(outlog, "\n");
	fflush(outlog); // to see errors in case of crash
	va_end(args);
	return;
}

void logInfo(const char* message, ...) {
	va_list args;
	va_start(args, message);
	const std::string res = vstrformat(message, args);
	fprintf(outlog, "[Info ]: ");
	fwrite(res.c_str(), 1, res.size(), outlog);
	fprintf(outlog, "\n");
	va_end(args);
	return;
}

extern "C" FILE* setlogdesc(FILE* f) {
	FILE* old = outlog;
	outlog = f;
	nooverlog = true;
	return old;
}

/*
 * Reads a whole file into memory (std::string)
 */
#include "fileIO.hpp"
bool loadFileContent(const char* finm, std::string& buff) {
	FileIO file; // RAIL stdio file wrapper
	file.open(finm, "r");	
	if (!file) {
		logError("Can't open file %s!", finm);
		return false;		
	};

	// get file size 
	file.seek(0, ::FileIO::SeekBase::END);
	size_t len = file.tell();

	// allocate buffer to load file source
	// (actually preallocating std::string internal buffer)
	buff.clear();
	try {
		buff.reserve(len + 2);
	} catch (...) {
		return false; // no message
	}

	file.rewind();
	// reading by fixed size block, since this is really good for disk IO
	const size_t block_size = 256;
	size_t blocks_num = len / block_size;

	try {
		char tmp[block_size];
		for (size_t bli = 0; bli < blocks_num; bli++) {
			if (!file) throw (bli * block_size); // IO error
			auto cnt = file.read(tmp, block_size);
			if (cnt < 1) throw (bli * block_size + cnt); //file shrinked?
			buff.append(tmp, block_size);
		};
		// read rest
		size_t n = len - (blocks_num * block_size);
		auto cnt = file.read(tmp, 1, block_size);
		if (cnt != n) throw n;
		buff.append(tmp, cnt);
	} catch (size_t i) {
		logError("Can't read file after opening" 
			"(pos %i/%i)", (int)i, (int)len);
		return false;
	}

	file.close();
	return true;
}

#include <vector>

std::shared_ptr<PluginProperties> LoadPlugin(const char* name) {
	std::string code;
	if (!loadFileContent(name, code)) {
		return nullptr; // error
	}
	std::string bytecode;
	LuaState::compileCode(code.c_str(), code.size(), bytecode);
	code.clear();

	// we need state here
	auto p = std::make_shared<PluginProperties>();
	LuaState& L = p.get()->master;
	InitMasterState(L);

	p.get()->bytecode = bytecode;
	logInfo("length : %li", bytecode.size());
	if (!L.loadBytecode(bytecode, name)) {
		// can't continue
		luaerror:
		bytecode = lua_isstring(L, -1) ? lua_tostring(L, -1) : "?";
		logError("Can't load plugin %s! Error : %s!", name, bytecode.c_str());
		return nullptr;
	}
	logInfo("Compiles sucessfully");
	// attempt to set values
	if (lua_pcall(L, 0, 1, 0) != LUA_OK) goto luaerror;
	logInfo("Runs successfully");
	lua_pop(L, 1);
	// and parse it + do some internal stuff to optimize master state
	if (!InitMasterValues(L, p.get())) goto luaerror;
	return p; // well done!
}

/*
 * LADSPA_Descriptor here
 */

PluginHandle* makeHandle(PlugPropShared props, unsigned long rate) {
	auto handle = std::make_unique<PluginHandle>();
	PluginHandle* H = handle.get();
	H->P = props;
	H->samplerate = rate;
	LuaState& L = handle.get()->L;
	InitInstanceState(L);
	std::string str = props->bytecode;

	if (!L.loadBytecode(str, props->name)) {
		// can't continue
		luaerror:
		str = lua_tostring(L, -1);
		logError("Can't instanciate plugin %s! Error : %s!", props->name,
			str.c_str());
		return nullptr;
	}
	// attempt to call
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) goto luaerror;
	// final step
	InitInstanceBuffers(L, H);
	return handle.release();
}

static void* newinstance(const LADSPA_Descriptor* D, unsigned long rate) {
	auto P = *(reinterpret_cast<PlugPropShared*> (
		D->ImplementationData));
	return makeHandle(P, rate);
}

static void cleaninstance(void* state) {
	auto handle = reinterpret_cast<PluginHandle*>(state);
	delete handle;
}

#define BUFFNAME "_bufferMT"

static void connectport(void* state, unsigned long idx, sample_type* data) {
	auto handle = reinterpret_cast<PluginHandle*>(state);
	LuaState& L = handle->L;
	if (lua_getfield(L, LUA_REGISTRYINDEX, "buffers") != LUA_TTABLE) {
		logError("Connect : no buffers in the registry");
		lua_pop(L, 1);
		return;
	}
	if (lua_rawgeti(L, -1, idx+1) != LUA_TUSERDATA) {
		logError("no buffer in the registry at index %i!", idx);
		lua_pop(L, 2);
		return;
	}
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*> (
		luaL_checkudata(L, -1, BUFFNAME) // crashes
	);
	B->buffer = data;
	lua_pop(L, 2);
}

static void docall(lua_State* L, const char* field) {
	if (lua_getfield(L, LUA_GLOBALSINDEX, field) != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return;
	}
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		logError("Error while calling %s() : %s", field, lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

static void activate(void* state) {
	auto handle = reinterpret_cast<PluginHandle*>(state);
	LuaState& L = handle->L;
	auto top = lua_gettop(L);
	docall(L, "activate");
	handle->activated = true;
	if (top != lua_gettop(L))logError("bad top! (was %i, now %i)", top, lua_gettop(L));
}

static void deactivate(void* state) {
	auto handle = reinterpret_cast<PluginHandle*>(state);
	LuaState& L = handle->L;
	auto top = lua_gettop(L);
	docall(L, "deactivate");
	handle->activated = false;
	if (top != lua_gettop(L))logError("bad top! (was %i, now %i)", top, lua_gettop(L));
}

static void run(void* state, unsigned long samplecount) {
	auto handle = reinterpret_cast<PluginHandle*>(state);
	LuaState& L = handle->L;
	if (!handle->activated) logError("Plugin was not activated!");

	auto top = lua_gettop(L);
	// for each external buffer
	if (lua_getfield(L, LUA_REGISTRYINDEX, "buffers") != LUA_TTABLE) {
		logError("Run : no buffers in the registry"); lua_pop(L, 1); return;
	}
	int cnt = lua_objlen(L, -1);
	for (int i = 1; i <= cnt; i++) {
		lua_rawgeti(L, -1, i);
		LadspaBuffer* B = reinterpret_cast<LadspaBuffer*> (
			luaL_checkudata(L, -1, BUFFNAME) // crashes, assumes it readonly
		);
		B->size = handle->P->portDescriptors[i-1] & LADSPA_PORT_CONTROL ? 1 : samplecount;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	if (lua_getfield(L, LUA_GLOBALSINDEX, "run") != LUA_TFUNCTION) {
		lua_pop(L, 1);
		if (top != lua_gettop(L)) logError("bad top! (was %i, now %i)", top, lua_gettop(L));
		return;
	}
	lua_pushnumber(L, samplecount);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		logError("Error while calling run() : %s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	if (top != lua_gettop(L)) logError("bad top! (was %i, now %i)", top, lua_gettop(L));
}

// holy right :D (you must release std::shared_ptr here!)
LADSPA_Descriptor makeDescriptor(PlugPropShared prop) {
	return (LADSPA_Descriptor) {
		123, prop->label, default_properties, prop->name, prop->maker,
		prop->copyright, prop->portCount, prop->portDescriptors.get(),
		prop->portNames.get(), prop->portRangeHints.get(),
		(void*)new PlugPropShared(prop), newinstance, connectport, activate, run, 
		nullptr, nullptr, deactivate, cleaninstance
	};
}

// OS specific stuff is hidden behind the scenes

#include <exception>

static fsys::path empty_path;
static fsys::path search_pathes[2] = {};

#ifdef linux
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

static const char* getHome() {
	const char* home = nullptr;
#ifdef _WIN32
	home = getenv("USERPROFILE");
#else
	home = getenv("HOME");
#endif
	if (!home) { // last try
		#ifdef linux
		fprintf(stderr, "NO $HOME!\n");
		home = getpwuid(getuid())->pw_dir;
		#endif
		// no way to workaround!
	}
	fprintf(stderr, "$HOME = %s!\n", home);
	return home ? home : "";
}

#include <stdlib.h>
static void initPathes() {
	#ifdef _WIN32
	search_pathes[0] = "C:/lualadspa/";
	search_pathes[1] = (fsys::path(getHome()) / "AppData/Local/lualadspa/");
	#else
	search_pathes[0] = "/usr/share/lualadspa";
	search_pathes[1] = (fsys::path(getHome()) / ".lualadspa");
	#endif
}

static volatile bool init_done = false;

class LUALADSPA {
	/*
	 * Array of loaded plugins.
	 * (Plugin properties are stored in ImplementationData!)
	 * In's important to keep them in Shared pointer, so ImplementationData is actually a Shared Pointer on PluginProperties :)
	 * std::shared_ptr is thread safe, so there should be npo any problems, hopefully.
	 */
	public:
	std::vector<LADSPA_Descriptor> plugins;
	private:
	void tryLoadPlugins(const fsys::path& path) {
		for (auto const& entry : fsys::directory_iterator(path)) {
			auto prop = LoadPlugin(entry.path().c_str());
			if (prop) {
				plugins.push_back(makeDescriptor(prop));
			}
		}
	}
	public:
	LUALADSPA() {
		initPathes();
		if (!nooverlog) {
			FILE* f = fopen("./lladspa.log", "w");
			if (!f) f = stderr; // IMPORTANT TO FALLBACK!
													// (since there may be more than 1 lualadspa user)
			outlog = f;
		}

		// enum pathes
		for (int i = 0; i < 2; i++) {
			auto& p = search_pathes[i];
			try {
				logInfo("Process %s...", p.c_str());
				tryLoadPlugins(p);
			} catch (std::exception& e) {
				logError("Can't process directory %s : %s", p.c_str(), e.what());
			};
		}
		logInfo("All directories was passed successfully!");
		init_done = true;
	}
	~LUALADSPA() {
		for (auto &i : plugins) {
			auto prop = reinterpret_cast<PlugPropShared*>(i.ImplementationData);
			delete prop; // yeah...
		}
		// no plugins available
		plugins.clear();
		logInfo("Plugins are unloaded!");
		if (outlog != stderr) fclose(outlog);
		init_done = false;
	}
};

class LUALADSPA* _G = nullptr;

extern "C" const LADSPA_Descriptor* ladspa_descriptor(unsigned long Index) {
	if (!_G) _G = new LUALADSPA;
	if (Index >= _G->plugins.size())
		return NULL;
	return _G->plugins.data() + Index;
}

static class DEST {
	public:
	~DEST() {
		init_done = false;
		delete _G;
	}
} __DEST; // static destructor
