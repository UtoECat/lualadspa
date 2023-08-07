/*
 * LuaLadspa - write your own LADSPA plugins on lua :)
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

#include "ladspa.h"
#include "luau.hpp"
#pragma once

#define DEEP_DEBUG 0

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0) 
#else
#define LIKELY(x) x
#define UNLIKELY(x) x
#endif

static_assert(sizeof(LADSPA_Data) <= sizeof(lua_Number));

#define IS_INPUT(x)   LADSPA_IS_PORT_INPUT(x)
#define IS_OUTPUT(x)  LADSPA_IS_PORT_OUTPUT(x)
#define IS_CONTROL(x) LADSPA_IS_PORT_CONTROL(x)
#define IS_AUDIO(x)   LADSPA_IS_PORT_AUDIO(x)

#define HTOGGLED     LADSPA_HINT_TOGGLED
#define HSAMPLERATE  LADSPA_HINT_SAMPLE_RATE
#define HLOGARITHMIC LADSPA_HINT_LOGARITHMIC
#define HINTEGER     LADSPA_HINT_INTEGER

using sample_type = LADSPA_Data;
constexpr int default_properties = LADSPA_PROPERTY_REALTIME;

	// this is more meaningful names for this types
typedef LADSPA_PortDescriptor port_type;
using port_hints = LADSPA_PortRangeHint;

constexpr int version_major = 0;
constexpr int version_minor = 2;

#include <cstdio>
#include <cstdarg>
#include <string>
#include <memory>

const std::string strformat(const char * const fmt, ...);
const std::string vstrformat(const char * const fmt, va_list args);	
void  logError(const char* message, ...);
void logInfo(const char* message, ...);
bool loadFileContent(const char* finm, std::string& buff);

// RAII Lua State
class LuaState {
	private :
	struct LimitedAllocData {
		size_t allocated = 0; // how many bytes was allocated
		size_t maxlimit = 32 << 20; // 32 Mb
	};
	// limited allocator function
	static void* limitedAlloc(void* ud, void* p, size_t oldsz, size_t nsz) {
		LuaState* L = reinterpret_cast<LuaState*>(ud);
		return L->limalloc(p, oldsz, nsz);
	}
	public:
	LimitedAllocData allocdata;
	lua_State* L;
	public:

	LuaState(); 
	~LuaState();
	LuaState(const LuaState&) = delete;
	LuaState(LuaState&& src);

	operator lua_State*() const {
		return L;
	}

	void* limalloc(void* p, size_t old, size_t nsz);
	double memoryUsageFactor();

	void limitMemoryKB(size_t KBytes);
	bool loadBytecode(const char* buff, size_t size, const char* name);	
	bool loadBytecode(const std::string &bc, const char* name);
	static void compileCode(const char* src, size_t len, std::string& out);
};

struct PluginProperties {
	/* makes it easy to reuse plugin as quick as possible */
	std::string bytecode;
	/* This lua state MUST BE KEEPED ALIVE DURING THE WHOLE USAGE
	 * OF THIS PLUGIN TYPE!
	 * (Strings below are used DIRECTLY from it.)
	 * This desizion was made to simplify implementation
	 */
	LuaState  master;
	
	/*
	 * We cache this values here just because accessing lua state
	 * REQUIRES writing to stack. Even if there is workaround by
	 * acessing lua objects DIRECTLY, it is not very intuitive (and safe).
	 */
	const char* name;
	const char* label;
	const char* maker;
	const char* copyright;
	bool        realtime;
	
	// this array is maintained through uniqueptr, but strings in it
	// are still from LuaState!	
	std::unique_ptr<const char*[]> portNames;

	// this is why ladspa sucks a bit... but it's not very critical.
	std::unique_ptr<LADSPA_PortDescriptor[]> portDescriptors;
	std::unique_ptr<LADSPA_PortRangeHint[]> portRangeHints;
	size_t portCount;
};

struct PluginHandle {
	PluginProperties *P; // master (READONLY!!!)
	int activated; // debug
	unsigned int samplerate;
	unsigned long samplescnt;
	LuaState L; // plugin instance
};

/*
 * Loads internal functions to the registry
 * + apply sandbox
 */
void InitMasterState(LuaState& L);
bool InitMasterValues(LuaState& L, PluginProperties* props); 

/*
 * Loads ladspa library + applies sandbox
 */
void InitInstanceState(LuaState& L);
/*
 * finally adds buffers to communicate
 */
bool InitInstanceBuffers(lua_State* L, PluginHandle* H);

struct LadspaBuffer {
	sample_type* buffer;
	size_t size   : 63;
	bool external :  1;
};

LadspaBuffer* NewBuffer(lua_State* L, bool external);

// custom libs
void OpenInternals(lua_State* L);
void OpenLuaLadspa(lua_State* L);

// modules/database api
extern "C" void refreshDatabase();

// already precompiled modules
const std::string& loadModule(const char* id);
const char* modulesNameIterator(long int idx);

// where to search for new modules?
const char* searchPathesIterator(long int idx);


