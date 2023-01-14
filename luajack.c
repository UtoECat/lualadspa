/*
 * LuaJack - write your own jack clients on lua :)
 * Copyright (C) UtoECat 2023. All rights reserved!
 * GNU General Public License 3.0. No any Warrianty!
 */

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/luajit.h>
#include <jack/jack.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

static const char*    server_name = "default";
static int            client_active = 0;
static jack_client_t* J = NULL;
static mtx_t          wait_mutex;

struct port_lua {
	jack_port_t* port;
};

/*
 * Creates Jack port
 *
 * @arg port name
 * @arg port is input (else output)? 
 * @arg port is terminal?
 */

int new_port(lua_State* L) {
	if (client_active) luaL_error(L, "Ports can't be created while client is activated!");
	const char* name = luaL_checkstring(L, 1);
	const char* type = JACK_DEFAULT_AUDIO_TYPE;
	int flags = JackPortIsOutput;

	if (lua_type(L,2) == LUA_TBOOLEAN) {
		flags = lua_toboolean(L, 2) ? JackPortIsInput : JackPortIsOutput;
	} else if (lua_isnoneornil(L, 2)) {
		flags = JackPortIsOutput;
	} else luaL_typerror(L, 2, "boolean");
	
	if (lua_type(L,3) == LUA_TBOOLEAN) {
		flags = flags | (lua_toboolean(L, 3) ? JackPortIsTerminal : 0);
	} else if (!lua_isnoneornil(L, 3)) luaL_typerror(L, 3, "boolean");

	jack_port_t* prt = jack_port_register(J, name, type, flags, 0);
	if (!prt) {
		luaL_error(L, "Can't create port by any reason...");
	}
	struct port_lua* p = (struct port_lua*)lua_newuserdata(L, sizeof(struct port_lua));
	p->port = prt;
	luaL_getmetatable(L, "jackportmt");
	lua_setmetatable(L, -2);
	return 1;
}

/*
 * Method of lua_port object. 
 */
int portclose(lua_State* L) {
	struct port_lua* p = (struct port_lua*) luaL_checkudata(L, 1, "jackportmt");
	if (p->port) jack_port_unregister(J, p->port);
	else fprintf(stderr, "warning: attempt to close port again!\n");
	p->port = NULL;
	return 0;
}

/*
 * __gc metamethod of lua_port object. 
 */
int portcollect(lua_State* L) {
	struct port_lua* p = (struct port_lua*) luaL_checkudata(L, 1, "jackportmt");
	if (p->port) jack_port_unregister(J, p->port);
	p->port = NULL;
	return 0;
}

/*
 * Method of lua_port object. 
 */
int portconnected(lua_State* L) {
	struct port_lua* p = (struct port_lua*) luaL_checkudata(L, 1, "jackportmt");
	(!p->port) ? lua_pushnil(L) : lua_pushboolean(L, jack_port_connected(p->port));
	return 1;
}

/*
 * Method of lua_port object!
 * Returns lightuserdata with buffer of port
 *
 * @arg lua_port object
 * @arg needed size of buffer
 *
 * @ret lightuserdata or nil, if port was closed by any reason
 */
int portbuff(lua_State* L) {
	struct port_lua* p = (struct port_lua*) luaL_checkudata(L, 1, "jackportmt");
	jack_nframes_t cnt = luaL_checkinteger(L, 2);
	(!p->port) ? lua_pushnil(L) : lua_pushlightuserdata(L, jack_port_get_buffer(p->port, cnt));
	return 1;
}

int getrate(lua_State* L) {
	lua_pushinteger(L, jack_get_sample_rate(J));
	return 1;
}

struct luaL_Reg funcs[] = {
	{"newport", new_port},
	{"getrate", getrate},
	{NULL, NULL}
};

void openjacklib(lua_State* L) {
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_setfuncs(L, funcs, 0);
	luaL_newmetatable(L, "jackportmt");	
	#define PUSHKCFUNC(name, func)\
	lua_pushcfunction(L, func);\
	lua_setfield(L, -2, name);
	PUSHKCFUNC("close", portclose);
	PUSHKCFUNC("connected", portconnected);
	PUSHKCFUNC("buffer", portbuff);
	PUSHKCFUNC("__gc", portcollect);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	#undef PUSHKCFUNC
	lua_pop(L, 1);
	// remove 'bad' functions
	#define REMOVE(name)\
	lua_pushnil(L); lua_setfield(L, LUA_GLOBALSINDEX, name);
	REMOVE("loadfile");
	REMOVE("dofile");
	#undef REMOVE
}

//
// EXECUTION AND ERROR CHECKING
//

int parseargs(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "bad usage: Lua file required!\n");
		return -1;
	}
	if (argv[1][0] == '-') {
		switch (argv[1][1]) {
			case 'h':
				fprintf(stderr, "help is here!\n");
				fprintf(stderr, "usage:\n");
				fprintf(stderr, "\t%s [code.lua] \t\t\t - runs file\n", argv[0]);
				fprintf(stderr, "\t%s -h \t\t\t\t - shows help\n", argv[0]);
				fprintf(stderr, "\t%s -v \t\t\t\t - shows version\n", argv[0]);
				fprintf(stderr, "\t%s [code.lua] [server name]\t - runs file for jack server with specified name\n", argv[0]);
				return -1;
			case 'v' :
				fprintf(stderr, "luaJACK version 0.1\n");
				fprintf(stderr, "Copyright (C) UtoECat 2023. All rights reserved.\n");
				fprintf(stderr, "GNU GPL License. No any warrianty!\n");
				return -1;
			default:
				fprintf(stderr, "bad key %c!\n", argv[1][1]);
				return -1;
		}
	}
	if (argc == 3) {
		server_name = argv[2];
	}
	return 0;
}

int checkjack(jack_client_t* cli, jack_status_t stat) {
	if (!J) {
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

int proc_func(jack_nframes_t cnt, void* ud) {
	lua_State* L = (lua_State*)ud;
	lua_getfield(L, LUA_GLOBALSINDEX, "process");
	lua_pushinteger(L, cnt);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error catched during process function! : %s\n", lua_tostring(L, -1));
		mtx_unlock(&wait_mutex);
		return 1;
	}
	return 0;
}

int main (int argc, char** argv) {
	LUAJIT_VERSION_SYM();
	if (parseargs(argc, argv)) return -1;
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	openjacklib(L);
	mtx_init(&wait_mutex, 0);

	// loading script
	if (luaL_loadfile(L, argv[1]) != LUA_OK) {
		fprintf(stderr, "Can't open lua file! :(\n");
		return -2;
	}

	// opening JACK client
	jack_status_t stat;
	J = jack_client_open(argv[1], JackServerName, &stat, server_name);
	if (checkjack(J, stat)) return -3;
	fprintf(stderr, "JACK client created successfully!\n");

	// creating ports in the script...
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error catched during first execution! : %s\n", lua_tostring(L, -1));
		goto first_error;
	}

	// get process function
	lua_getfield(L, LUA_GLOBALSINDEX, "process");
	if (!lua_isfunction(L, -1)) {
		fprintf(stderr, "Error : no process function setuped by script!\n");
		goto first_error;
	}
	lua_gc(L, LUA_GCCOLLECT, 0);

	jack_set_process_callback(J, proc_func, (void*)L);
	jack_activate(J);

	// костыли и велосипеды (C)
	mtx_lock(&wait_mutex);
	while (mtx_lock(&wait_mutex) != 0) {};
	
	jack_deactivate(J);
	first_error:
	mtx_destroy(&wait_mutex);
	jack_client_close(J);
	fprintf(stderr, "JACK : Jack client closed!\n");
	lua_close(L);
	return 0;
}
