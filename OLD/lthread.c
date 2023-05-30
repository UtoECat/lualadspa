/*
 * LuaJack - write your own jack clients on lua :)
 * Copyright (C) 2022 UtoECat

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

#include "luajack.h"
#include "bytecode.h"

lua_State* newState() {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_loadbuffer(L, luaJIT_BC_luajack, luaJIT_BC_luajack_SIZE) != LUA_OK) {
		fprintf(stderr, "Internal bytecode corruption!\n");
		lua_close(L);
		return NULL;
	}

	// first arg
	luaL_openjack(mainState);
	return L;
}

int lj_initState(const char** argv) {
	mainState = newState();
	if (!mainState) return -1;

	// second arg
	lua_newtable(mainState);
	for (int i = 0; argv[i]; i++) {
		lua_pushstring(mainState, argv[i]);
		lua_rawseti(mainState, i);
	}

	if (lua_pcall(mainState, 2, 0, 0) != LUA_OK) {
		fprintf(stderr, "Init failed! Reason :\n%s",
			lua_tostring(mainState, -1));
		return -2;
	}
	return 0;
}
