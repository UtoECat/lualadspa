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

lua_State* workState = NULL;

static int checkjack(jack_status_t stat) {
	if (!Client) {
		#define CHECK_MSG(id, str)\
		if (stat & id) {\
			fprintf(stderr, "JACK : " str "!\n");\
		}
		CHECK_MSG(JackServerFailed, "Unable to connect to the JACK server")
		CHECK_MSG(JackInitFailure, "Unable to initialize new JACK client")
		CHECK_MSG(JackVersionError, "Client's Protocol version does not match")
		CHECK_MSG(JackBackendError, "Backend Error")
		CHECK_MSG(JackFailure, "Operation Failed")
		CHECK_MSG(JackInvalidOption, "Invalid Option?")

		#undef CHECK_MSG
		return -1;
	}
	if (stat & JackServerStarted) {
		fprintf(stderr, "Jack server was started!\n");
	}
	return 0;
}

static int init(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	jack_status_t stat;
	lua_pushboolean(L, 1);

	// reuse existed client
	if (Client == NULL) return 1;
	Client = jack_client_open(name, JackServerName, &stat, server_name);
	if (checkjack(stat)) {
		lua_pushboolean(L, 0); // false
	}
	return 1;
}

int getrate(lua_State* L) {
	lua_pushinteger(L, Client ? jack_get_sample_rate(Client) : -1);
	return 1;
}

int isactive(lua_State* L) {
	lua_pushinteger(L, client_active);
	return 1;
}

static struct luaL_Reg funcs[];

/*
** Buffer to store the compiled bytecode.
*/
struct bcWriter {
	void*  data;
	size_t size;
	size_t pos;
};

static int resizebuff(lua_State* L, struct bcWriter* W, size_t sz) {
	void* old = W->data;
	if (sz) {
		if (W->data) {
			W->data = realloc(W->data, sz);
		} else {
			// init
			W->data = calloc(sz, 1);
			W->pos  = 0;
		}
		if (!W->data) {
			free(old);
			luaL_error(L, "Out of memory!");
		}
	} else {
		free(W->data);
		W->data = NULL;
	}
	W->size = sz;
	return 0;
}

static int addbuff (lua_State* L, struct bcWriter* W, const void* bc, size_t sz) {
	if (!W->data) resizebuff(L, W, 64);
	if (W->pos + sz >= W->size) resizebuff(L, W, W->size + sz + 64);
	memcpy(W->data + W->pos, bc, sz);
	W->pos += sz;
	return 0;
}

static int bcwriter (lua_State *L, const void *bc, size_t size, void *ud) {
  struct bcWriter *W = (struct bcWriter*)ud;
	addbuff(L, W, bc, size);
	return 0;
}

static int luaL_pushdump (lua_State* L, struct bcWriter* W) {
	int stat = luaL_loadbuffer(L, W->data, W->size);
	resizebuff(W, 0);
	assert(stat == LUA_OK);
	return stat;
}

static int luaL_dump (lua_State *L, struct bcWriter* bc) {
	assert(bc && !bc->data);
  luaL_checktype(L, -1, LUA_TFUNCTION);
  if (l_unlikely(lua_dump(L, bcwriter, bc, 0) != LUA_OK))
    return luaL_error(L, "Can't dump given function!");
  return LUA_OK;
}

static lua_State* worker = NULL;

static int proc_func(jack_nframes_t cnt, void* ud) {
	lua_State* L = (lua_State*)ud;
	lua_getfield(L, LUA_GLOBALSINDEX, "process");
	lua_pushinteger(L, cnt);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error catched during process function! : %s\n", lua_tostring(L, -1));
		return 1;
	}
	return 0;
}

int activate(lua_State* L) {
	lua_State* wS = newState();
	if (!wS) luaL_error(L, "???");
	lua_pushnil(wS);
	if (lua_pcall(wS, 2, 0, 0) != LUA_OK) { // init new state
		fatalErr:
		lua_pushstring(L, lua_tostring(wS, -1));
		lua_close(wS);
		lua_error(L);
	}

	int top = lua_gettop(L);
	if (lua_isfunction(L, 1)) {
		lua_pushvalue(L, 1);
		struct bcWriter BC = {0};
		luaL_dump(L, &BC);
		luaL_pushdump(wS, &BC);
	} else if (lua_isstring(L, 1)) {
		if (luaL_loadstring(wS, lua_tostring(L, 1)) != LUA_OK) goto fatalErr;
	} else {
		lua_pushstring(wS, "Audio processing state init function or code string excepted");
		goto fatalErr;
	}
	if (pcall(wS, 0, 0, 0) != LUA_OK) {
		goto fatalErr;
	}
	if (!lua_getfield(wS, "process", LUA_GLOBALSINDEX)) {
		lua_pushstring(wS, "process function is excepted to be created in the globals table!");
		goto fatalErr;
	}
	jack_set_process_callback(Client, proc_func, wS);
	worker = wS;
	jack_activate(Client);
	return 0;
}

int deactivate(lua_State* L) {
	jack_deactivate(Client); 
	if (worker) {
		lua_close(worker);
		worker = NULL;
	}
	return 0;
}

int create_port(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	int flags = lua_toboolean(L, 2) ? JackPortIsInput : JackPortIsOutput;
	flags = flags | (lua_toboolean(L, 3) ? JackPortIsTerminal : 0);
	const char* type = luaL_optstring(L, 4, JACK_DEFAULT_AUDIO_TYPE);

	jack_port_t* prt = jack_port_register(J, name, type, flags, 0);
	if (!prt) {
		luaL_error(L, "Can't create port by any reason...");
	}
	lua_pushlightuserdata(L, prt);
	return 1;
}

int close_port(lua_State* L) {
	jack_port_t* port = (jack_port_t*)lua_touserdata(L, 1);
	if (port) jack_port_unregister(J, port);
	return 0;
}

static struct luaL_Reg funcs[] = {
	{"create_port", create_port},
	{"close_port",  close_port},
	{"getrate",     getrate},
	{"isactive",    isactive},
	{"init",        init},
	{NULL, NULL}
};
