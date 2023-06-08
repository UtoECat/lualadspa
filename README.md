# LUALADSPA

## General info

Lualadspa is a very powerful ladspa plugin, that gives you ability to write a lot of kinds of ladspa audio plugins in Lua - the best dynamic language, in my opinion.

Your plugins are simple lua files, that contain all info about plugin and all plugin's methods. Version check and plugin information validation are already implemented.

![plugin information](doc/img1.png)

**WARNING! This project is still in early development, aka not very usable in production!** But it would be wonderful, if you will contribute in this project in some way. TODO list may be found in `TODO.md file`.

## Luau, available libs, API and so on...

## Example

See `./plugins/mixer.lua` for exaple of such plugin.

# Building/Instalation instruction

todo

# For Contributors

## Implementation details

Your plugin file could be loaded in two states : master state and plugin state. Theese states are invinsible for plugin developer as long, as he
not tries to do some dirty stuff :)
Main state is created to parse/get all main information and provide it to LADSPA_* interfaces for LADSPA HOSTS. Also it keeps precompiled main chunk.
Each time when new instance of your plugin is created - it is created in PLUGIN state itself, with invinsible, unavailable from Lua directly, link to the master state.

Actually, for efficiency, master state are used just as anchors for all GC objects, that represents plugin info, while this plugin info is already getted from state and saved for easy access in readonly pointers, so it can
be used without expensive locks in multithreaded enviroment.

Master state can even dissapear at some time in future, but i am not sure...

Master state has another one purpose : it runs internal bytecode, that does
all this dirty "getting/caching values from lua state to C", validates plugin info and so on. It was really much easier to implement in lua, than in C/C++.

Also, this gives us ability to limit maximal memory usage per plugin state.
I am still not sure what this limit should be. Now it's 32 Mb.

`_G.ladspa` table contains some useful functions for your plugins - version info, creating/resizing CUSTOM AUDIO BUFFERS (*needs testing*), getting current samplerate and so on.
They are actively uses registry table internally.

**AND REALLY IMPORTANT NOTE :** lualadspa uses **NOT VANILLA LUA, BUT LUAU -** customized lua distributon for SAFE embedding + with much better VM/Compiler, while keeping a whole codebase on C/C++ => still being as portable, as C/C++ (this is where luajit sucks :D).

While safe embedding is still just cool addition, faster interpreter is really important, to minimize audio flows/gaps and other shit.
I know that luajit WILL be faster, but at the same time... There may be some dragons. 

## TODO
todo

i want coffee
