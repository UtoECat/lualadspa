/*
 * LuaLadspa - write your own LADSPA plugins on lua :)
 * Plugin instance implementation, parsing, execution
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

static void ProxyGlobals(lua_State* L) {
	lua_createtable(L, 0, 1);
	lua_createtable(L, 0, 1);
	lua_pushvalue(L, LUA_GLOBALSINDEX);	
	lua_setfield(L, -2, "__index"); // proxy reads
 	lua_setreadonly(L, -1, true); // make metatable readonly
 	lua_setmetatable(L, -2); // apply
	lua_setsafeenv(L, -1, true);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "_G");
	lua_replace(L, LUA_GLOBALSINDEX);
}

static void luaopen_extra2(lua_State* L);

LuaState::LuaState() {
	L = lua_newstate(limitedAlloc, this);
	if (!L) throw "NOMEM";
	luaL_openlibs(L);
	luaopen_extra2(L);
	lua_setthreaddata(L, this);
}

double LuaState::memoryUsageFactor() {
	return (double)allocdata.allocated/(double)allocdata.maxlimit;
}

/*
 * Loads internal functions to the registry
 * + apply sandbox
 */
void InitMasterState(LuaState& L) {
	OpenInternals(L); // opened in REGISTRY!
	OpenLuaLadspa(L);
	luaL_sandbox(L);
	ProxyGlobals(L);
}

#include "internal.h"
bool InitMasterValues(LuaState& L, PluginProperties* props) {
	lua_pushlightuserdata(L, props);
	lua_setfield(L, LUA_REGISTRYINDEX, "props");
	lua_setreadonly(L, LUA_GLOBALSINDEX, false);
	// load internal bytecode
	if (!L.loadBytecode(internal_bcode, internal_size, "=internal")) return false;
	// do it
	lua_pushvalue(L, LUA_REGISTRYINDEX);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) return false;
	lua_setreadonly(L, LUA_GLOBALSINDEX, true);
	return true;
}	

bool InitInstanceBuffers(lua_State* L, PluginHandle* H) {
	lua_createtable(L, H->P->portCount, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_GLOBALSINDEX, "buffers");
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_REGISTRYINDEX, "buffers");
	for (int i = 1; i <= H->P->portCount; i++) {
		if (!NewBuffer(L, true)) return false;
		lua_rawseti(L, -2, i);
	}
	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
	// add samplerate here
	lua_pushnumber(L, H->samplerate);
	lua_setfield(L, LUA_REGISTRYINDEX, "samplerate");
	return true;
}

/*
 * Loads ladspa library + applies sandbox
 */
void InitInstanceState(LuaState& L) {
	OpenLuaLadspa(L);
	luaL_sandbox(L);
	ProxyGlobals(L);
}

LuaState::~LuaState() {
	if (L) lua_close(L);
}

LuaState::LuaState(LuaState&& src) {
	L = src.L;
	allocdata = src.allocdata;
	src.L = nullptr;
}

void LuaState::limitMemoryKB(size_t KBytes) {
	allocdata.maxlimit = KBytes << 10; // cast to KiloBytes
}

bool LuaState::loadBytecode(const char* b, size_t sz, const char* name) {
	return luau_load(L, name, b, sz, 0) == LUA_OK;
}
	
bool LuaState::loadBytecode(const std::string &bc, const char* name) {
	return luau_load(L, name, bc.c_str(), bc.size(), 0) == LUA_OK;
}

void LuaState::compileCode(const char* s, size_t l, std::string& b) {
	char* c = luau_compile(s, l, nullptr, &l);
	b.clear();
	b.append(c, l);
	free(c);
}

#include <malloc.h>
void* LuaState::limalloc(void* p, size_t old, size_t nsz) {
	size_t add = 0;
	if (nsz) {
		if (!p) {
			add = nsz;
			if (allocdata.allocated + nsz <= allocdata.maxlimit) p = malloc(nsz);
		} else if (old < nsz) {
			add = nsz - old;
			if (allocdata.allocated + nsz <= allocdata.maxlimit)
				p = realloc(p, nsz);
			else p = nullptr; // keep old block
		} else {
			add = old - nsz;
			void* o = p;
			p = realloc(p, nsz);
			if (!p) p = o; // no throw on shrinking
			allocdata.allocated -= add; // here not add, but sub <3
			return p;
		}
		// do nothing if equal
	} else if (p) {
		add = old;
		free(p); p = nullptr;
		allocdata.allocated -= add; // here not add, but sub <3
		return nullptr;
	}
	allocdata.allocated += add;
	return p;
}

/*
 * INTERNALS
 */

static int setup_arrays(PluginProperties* props, int n) {
	props->portNames = std::make_unique<const char*[]>(n);
	props->portDescriptors = std::make_unique<LADSPA_PortDescriptor[]>(n);
	props->portRangeHints = std::make_unique<LADSPA_PortRangeHint[]>(n);
	return 0;
}

static int luaI_setvalue(lua_State* L) {
	if (!lua_islightuserdata(L, 1)) luaL_error(L, "pointer excepted!");
	PluginProperties* prop = reinterpret_cast<PluginProperties*>
		(lua_tolightuserdata(L, 1));
	std::string name = luaL_checkstring(L, 2);
	lua_settop(L, 3);
	#define TOK(S) #S
	#define EQ(STR, ...) else if (name == TOK(STR)) {prop->STR = __VA_ARGS__;}
	if (name.empty()) {
		luaL_error(L, "internal field name excepted!");
	}
	EQ(name, lua_tostring(L, -1))
	EQ(label, lua_tostring(L, -1))
	EQ(maker, lua_tostring(L, -1))
	EQ(copyright, lua_tostring(L, -1))
	EQ(realtime, lua_toboolean(L, -1))
	EQ(portCount, lua_tointeger(L, -1); setup_arrays(prop, lua_tointeger(L, -1)))
	#undef TOK
	#undef EQ
	else luaL_error(L, "bad internal field name %s!", name.c_str());
	return 0;
}

static int luaI_setport(lua_State* L) {
	if (!lua_islightuserdata(L, 1)) luaL_error(L, "pointer excepted!");
	PluginProperties* prop = reinterpret_cast<PluginProperties*>
		(lua_tolightuserdata(L, 1));
	int index = luaL_checkinteger(L, 2)-1;
	if (index < 0 || index >= prop->portCount) luaL_error(L, "bad port index");
	const char* name = luaL_checkstring(L, 3);
	int descriptor = luaL_checkinteger(L, 4);
	int hint = luaL_checkinteger(L, 5);
	float min = luaL_optnumber(L, 6, -1.0);
	float max = luaL_optnumber(L, 7,  1.0);
	prop->portNames.get()[index] = name;
	prop->portDescriptors.get()[index] = descriptor;
	prop->portRangeHints.get()[index] = (LADSPA_PortRangeHint) {
		hint, min, max
	};
	return 0;
}

static int luaI_collect(lua_State* L) {
	lua_gc(L, LUA_GCCOLLECT, 0);
	return 0;
}

static const luaL_Reg internal_funcs[] = {
	{"_setvalue", luaI_setvalue},
	{"_setport", luaI_setport},
	{"_collect", luaI_collect},
	{nullptr, nullptr}
};

void OpenInternals(lua_State* L) {
	lua_pushvalue(L, LUA_REGISTRYINDEX);
	luaL_register(L, nullptr, internal_funcs);
	lua_pop(L, 1);
}

// hehe
extern FILE* outlog;

// extra logger for plugins
static int luaB_print2(lua_State* L) {
	int n = lua_gettop(L);
	try {
		std::string buffer;
		for (int i = 1; i <= n; i++) {
			size_t l;
			const char* s = luaL_tolstring(L, i, &l);
			if (i > 1)
				buffer += "\t";
			buffer.append(s, l);
			lua_pop(L, 1);
		}
		fprintf(outlog, "[Print]: ");
		fwrite(buffer.c_str(), 1, buffer.size(), outlog);
		fprintf(outlog, "\n");
	} catch(...) {
		fprintf(outlog, "[Print]: NOMEM\n");
	}
	return 0;
}

static const luaL_Reg extra_funcs[] = {
	{"print", luaB_print2}, // override print function
	{nullptr, nullptr}
};

static void luaopen_extra2(lua_State* L) {
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, nullptr, extra_funcs);
	lua_pop(L, 1);
}
