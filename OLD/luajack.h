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

#pragma once 
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/luajit.h>
#include <jack/jack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for spin locks
#include <pthread.h>

typedef pthread_spinlock_t spinlock_t;

// externs
extern lua_State* mainState;
extern lua_State* hashTable;
extern spinlock_t hashLock;
extern int   client_active;

// opens lua "jack library". These functions are INTERNAL and must not
// be used directly! This function is called in newState() already!
extern luaL_openjack(lua_State* L);

// initializes main lua state :)
int lj_initState(const char** argv);

// state preinitialization
lua_State* newState();
