/*
 * LuaLadspa - write your own LADSPA plugins on lua :)
 * Floating point buffer (actually udata that transfer pointer with
 * unknown size to private lua state ._.)
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
#define BUFFNAME "_bufferMT"

static inline void* getudata(lua_State* L) {
	#if DEEP_DEBUG
	return luaL_checkudata(L, 1, BUFFNAME)
	#else
	return lua_touserdata(L, 1); // no typechecks
	#endif
}

static int luaB_index(lua_State* L) {
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*>(getudata(L));
	int idx = luaL_checkinteger(L, 2) - 1;
	sample_type *ptr = B->buffer + idx;

	#if DEEP_DEBUG
	if (UNLIKELY(idx < 0 || idx >= B->size)) luaL_error(L, "out of bounds");
	else if (UNLIKELY(!B->buffer)) luaL_error(L, "buffer is empity");
	#endif

	lua_pushnumber(L, *ptr);
	return 1;
}

static int luaB_newindex(lua_State* L) {
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*>(getudata(L));
	int idx = luaL_checkinteger(L, 2) - 1;
	sample_type value = lua_tonumber(L, 3);
	sample_type *ptr = B->buffer + idx;

	#if DEEP_DEBUG
	if (UNLIKELY(idx < 0 || idx >= B->size)) luaL_error(L, "out of bounds");
	else if (UNLIKELY(!B->buffer)) luaL_error(L, "buffer is empity");
	#endif

	*ptr = value;
	return 0;
}

static int luaP_version(lua_State* L) {
	lua_pushnumber(L, version_major);
	lua_pushnumber(L, version_minor);
	return 2;
}

static void buffdtor(lua_State* LL, void* p) {
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*>(p);
	if (B->external) return; // do not try to destroy external buffers!
	LuaState& L = *reinterpret_cast<LuaState*>(lua_getthreaddata(LL));
	L.limalloc(B->buffer, B->size * sizeof(sample_type), 0);
}

LadspaBuffer* NewBuffer(lua_State* L, bool external) {	
	void* ud;
	if (external)	ud = lua_newuserdatatagged(L, sizeof(LadspaBuffer), 0);
	else ud = lua_newuserdatatagged(L, sizeof(LadspaBuffer), 24);
	if (luaL_newmetatable(L, "_bufferMT")) {
		lua_pushboolean(L, 0);
		lua_setfield(L, -2, "__metatable");
		lua_pushcfunction(L, luaB_index, "index");
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, luaB_newindex, "newindex");
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*>(ud);
	B->buffer = nullptr;
	B->size = 0;
	B->external = external;
	return B;
}

static int luaB_new(lua_State* LL) {
	int n = luaL_optinteger(LL, 1, 0);
	if (n < 0) n = 0;
	LadspaBuffer* B = NewBuffer(LL, false);
	LuaState& L = *reinterpret_cast<LuaState*>(lua_getthreaddata(LL));
	if (n) {
		B->buffer = reinterpret_cast<sample_type*> (L.limalloc(nullptr, 
			0, n * sizeof(sample_type)));
		if (!B->buffer) luaL_error(L, "NOMEM");
		B->size = n;
	}
	return 1;
}

static int luaB_resize(lua_State* LL) {
	int n = luaL_optinteger(LL, 2, 0);
	if (n < 0) n = 0;
	LadspaBuffer* B = reinterpret_cast<LadspaBuffer*> (
			luaL_checkudata(LL, 1, BUFFNAME));
	LuaState& L = *reinterpret_cast<LuaState*>(lua_getthreaddata(LL));
	if (n) {
		B->buffer = reinterpret_cast<sample_type*>( L.limalloc(B->buffer,
			B->size * sizeof(sample_type),
			n * sizeof(sample_type)
		));
		if (!B->buffer) luaL_error(L, "NOMEM");
		B->size = n;
	}
	lua_pushboolean(L, 1);
	return 1;
}

static int luaP_getusage(lua_State* LL) {
	LuaState& L = *reinterpret_cast<LuaState*>(lua_getthreaddata(LL));
	lua_pushnumber(L, L.memoryUsageFactor());
	return 1;
}

static int luaP_getrate(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "samplerate");
	return 1;
}

static const luaL_Reg ladspa_funcs[] = {
	{"getVersion", luaP_version},
	{"newBuffer", luaB_new},
	{"resizeBuffer", luaB_resize},
	{"getSampleRate", luaP_getrate},
	{"getMemoryUsage", luaP_getusage},
	{nullptr, nullptr}
};

void OpenLuaLadspa(lua_State* L) {
	lua_setuserdatadtor(L, 24, buffdtor);
	luaL_register(L, "ladspa", ladspa_funcs);
	lua_pop(L, 1);
}
