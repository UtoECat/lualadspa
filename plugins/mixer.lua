-- Simple Example mixer plugin for Lualadspa plugin developers.
-- You can share, use, copy, paste this file and edit it for your needs.
--
-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.


-- PLUGIN MUST SET "info" as a table with info about this plugin instance,
-- and ALSO DEFINE (BUT NOT EXECUTE!!!) ALL PLUGIN GLOBAL FUNCTIONS
-- WARNING: this table becomes READONLY after initialization ends!
info = {
	-- Name of the plugin. Optional.
	name = "Simple Mixer Example", 

	-- Unique Label of the plugin. IMPORTANT
	-- This string should identify your plugin and should not change
	-- between different plugin versions.
	-- MUST NOT contain special/nonprintable/space characters
	label = "plug1",
	
	-- Author/authors names. Optional
	maker = "LuaLadspa Community",

	-- Copyrights/License of the plugin. Optional
	copyright = "unlicensed",

	-- Is plugin inputs/outputs should not being cached and not be in
	-- big latency. Optional.
	-- Useful when your plugin is not a slow mess, and it requires
	-- realtime dependency, default value is false
	realtime = true,

	-- IMPORTANT.
	-- Specifies luaLadspa version this pluggin was written for.
	-- Higher values than current stops to parse this plugin, prints error,
	-- and continue to parse next plugins in plugins directory (if any).
	-- Lower values than current MAY continue to work, if previous version
	-- was not implement some compability-breaking features.
	-- It's guaranteed, that newer MAJOR versions (number before first dot)
	-- WILL ALWAYS HAVE COMPABILITY-BREAKING CHANGES (because of that theese
	-- releases are REALLY RARE)
	luaLadspaVersionMinor = 0,
	luaLadspaVersionMajor = 0
} -- _G.info

-- IMPORTANT : gives ports types, names and info, used by plugin
-- implementation. types is REQUIRED field for ports!
-- ports MUST be a valid lua ARRAY, without holes.
-- WARNING: This table becomes READONLY after initialization ends!
ports = {
	{
		-- works like a PortDescriptor in ladspa. Represedted by
		-- string with options :
		-- i - input port (aka recieves data)
		-- o - output port (send data, but not recieves)
		-- c - control port. Port used to control plugin behaviour.
		-- a - audio port. Port is transfers real audio data.
		-- options i and o CANNOT be mixed, but one of them MUST exist.
		type = "ia",

		-- user-frendly port name.
		name = "Input Channel 1", 

		-- hints of what data this port uses
		-- minimal value acceptable by this port.
		-- If nil specified, no minimal bound should be appied.
		-- Ignored if hint 'bool' is given
		min  = nil,

		-- maximal value acceptable by this port.
		-- If nil specified, no maximal bound should be appiled.
		-- Ignored if hint 'bool' is given
		max  = nil,

		-- Helps plugin executor (and user) to understand what kind of
		-- data ports uses/inputs/outputs. THIS DOES NOT MAKES ANY
		-- RESTRICTIONS FOR IMPLEMENTATION, HOSTS! Just useful extra info.
		-- Avaliable values are theese (may be mixed, separated by spaces) :
		-- 'bool' : data represented as logical flag. less or equal 0
		-- 									is false, else true. MUST BE EXCLUSIVE!
		-- 'freq' : data should be interpreted as multiplies
		-- 									of the samplerate in range (0.0 .. 1.0), in Hz.
		-- 	'int' : data should be represented as integer value
		-- 									(with stripped fractional part). (-inf .. +inf)
		--	'log' : data should be represented in logarithmic form.
		hint = nil,

		-- RESERVED FIELD. May become used in future, throws error if
		-- setted to any value
		default = nil,
	}, { 
		-- input port 2
		type = "ia",
		name = "Input Channel 2",
		min  = nil,
		max  = nil,
	}, {
		-- mixer output (you can just ignore values, that you set to nil)
		type = "oa",
		name = "Output",
	}
} -- _G.ports

-- If IMPORTANT TABLES, FIELDS are NOT EXIST, or they are SETTED TO 
-- INVALID VALUES, plugin parsing STOPS, and continues from the next plugin.
-- When plugin parsing STOPS, this means that this plugin WILL NOT be
-- returned to HOST, when he enumerates list of available plugins AT ALL.
-- theese kind of bugs should not be printed using host's perror(), so
-- there is where lualadspa log file used to save all theese errors.
--
-- Also, you don't need to manually restart your audio plugin host to
-- develop your plugins. LuaLadspa comes with CLI utility to enumerate, 
-- install (from archive or directory), update(not implemented) 
-- and TEST some plugins.
-- Plugin testing is an operation, when lualadspa CLI tries to parse/
-- execute your plugin with zero/noisy inputs for plugin, and checking
-- for proper value bounding in output. Even if bounding is only "GUI" 
-- feature, it's unproper work may create issues in buggy/badly tested GUI
-- generators.
-- This CLI also prints all warnings/errors into your console, so you don't
-- need to manually check and refresh log file in notepad :)

-- And yeah, 99% of this file are comments. Lualadspa configs, in fact, are
-- really compact in most cases. But there we touch ALL of them, so this is
-- why it's so big.
-- Also it's important to mention, (for those of yours that don't know lua
-- syntax) that there is no ordering limitation for fields in created
-- config tables (except for ports, since {a, b, c} construction is
-- threated as array, and elements are insterted into it in DIRECT order
-- aka. a at index 1, b at index 2, c at index 3 and so on.)

--[[
-- Plugin implementation (can be above too :))
--]]

function run(sz)
	-- do NOT put this just in main chunk function!
	-- (or do this at least in activate, by sharing this local globally)
	local out = buffers[3]
	local in1 = buffers[1]
	local in2 = buffers[2]
	-- mixing work
	for i = 1, sz do
		out[i] = (in1[i] + in2[i]) / 2
	end
end

-- There is some globals, that are acessable for your plugin
-- implementation :
-- - All safe standard lua functions (except for `io` and `os`) in constant,
-- proxied through current global table, table.
-- - buffers, added to all your ports when plugin starts to work (with 
-- this ports table becomes READONLY!). Represented as userdata, that
-- gives you acess to buffer data array (for audio port), or to single
-- value (for control port)
-- - plugin info constant global table (yeah, for important reason :))
-- - lualadspa functions in constant `ladspa` module.

-- YOUR implementation MAY WANT TO define next global functions :
-- activate()   - HOST tell this plugin that he will use it from this point.
--              this is place where you should to make your plugin ready
--              to process data from scratch (nothing should be saved from
--              previous deactivate()). 
--              (all tables are ALREADY READNLY HERE!)
--
-- deactivate() - HOST tell this plugin, that he will NOT use it from 
--              this point. Your plugin should release all state-dependent
--              resources, flags and counters (even if HOST may reuse this
--              plugin again, by calling activate() again)
--
-- cleanup()    - called when HOST finalizes this plugin instance. HOST
--              WILL call deactivate() before call to cleanup IF plugin was
--              activated.
--
-- And, in contrast, implementaion MUST DEFINE THIS FUNCTION :
-- run(size)    - called when HOST wants plugin to process all input data,
-- 							- and return results to the output.
-- 							- Also gives size of all buffers as first argument

-- Also, it's important to mention, that memory, available to plugin
-- implementation (and info + internal part too) is limited by 128 Mb. 
-- You CAN'T pass this limit at all. And this limit may change in some
-- rare situations to lower or higher values (maybe in future versions).

