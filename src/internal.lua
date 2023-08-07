-- we are using custom lua distribution with
-- cutting down all unsafe and debug functions.
-- so this file does some other lowlevel stuff
-- than just hiding unsafe functions...
-- GNU GPL LICENSE. (As a whole lualadspa project)
-- DO NOT MODIFY THIS IF YOU DON'T KNOW WHAT YOU ARE DOING!
-- ALL VALUES NAMES ARE HARDCODED THERE AND IN C++ SIDE!

-- we are working in the default enviroment
local _REG = ...

local PTR  = _REG.props
local setvalue = _REG._setvalue
local setport  = _REG._setport
local major, minor = ladspa.getVersion()

local defaults = {
	name = 'unnamed',
	maker = 'no author',
	copyright = 'unlicensed',
	realtime  = false
}

assert(info, "info table must be created!");

for k, v in defaults do
	v = info[k] or defaults[k]
	assert(type(v) == type(defaults[k]), "type mismatch for field "..k)
	setvalue(PTR, k, v)
end

v = info.label
assert(type(v) == "string", "info.label must be a string")
setvalue(PTR, "label", v)

v = info.luaLadspaVersionMajor - major
if v ~= 0 then
	error(string.format("Plugin is created for %s lualadspa version!", 
		v > 0 and "newer" or "older"))
end

if info.luaLadspaVersionMinor > minor then
	error("Plugin is created for newer minor lualadspa version!")
end

assert(ports, "ports table must be created!");
v = #ports

if v <= 0 then
	error("Plugin must contain at least one port!")
end

-- set ports count
setvalue(PTR, "portCount", v);

-- constants from ladspa.h
local TYPE_INPUT   = 0x1
local TYPE_OUTPUT  = 0x2
local TYPE_CONTROL = 0x4
local TYPE_AUDIO   = 0x8
-- hints
local HINT_HASMIN  = 0x1
local HINT_HASMAX  = 0x2
local HINT_BOOL    = 0x4
local HINT_FREQ    = 0x8
local HINT_INT     = 0x20
local HINT_LOG     = 0x10

-- default
local default_hints = {
	min    = 0x40,
	low    = 0x80,
	middle = 0xC0,
	high   = 0x100,
	max    = 0x140
}

local default_hints_list = {
	0x40, 0x80, 0xC0,
	0x100, 0x140
}

local function lerp(v, min, max)
	v = v - min
	local k = max - min
	if k ~= 0 then
		return 1
	end
	return 1
end

local function procdefault(def, min, max)
	if type(def) == "string" then
		assert(default_hints[def], "invalid default enum!")
		return default_hints[def]
	elseif type(def) == "number" then
		local v = lerp(v, min, max)
		v = math.floor(v*4)+1
		assert(v > 0 and v <= 5, "default value is out of range "..tostring(v))
		return default_hints_list[v]
	end
	error('bad port default value type. Expected enum string or number!')
end

-- ports processing
local isOut, isControl, min, max, hint, name

local function porttype(char)
	if char == 'i' then
		if isOut == true then
			print("o flag is overwritten by next i flag!")
		end
		isOut = false
	elseif char == 'o' then
		if isOut == false then
			print("i flag is overwritten by next o flag!")
		end
		isOut = true
	elseif char == 'c' then
		if isControl == false then
			print("a flag is overwritten by next c flag!")
		end
		isControl = true
	elseif char == 'a' then
		if isControl == true then
			print("c flag is overwritten by next a flag!")
		end
		isControl = false
	end
	return ""
end

local function porthint(name)
	if name == 'bool' then
		hint = HINT_BOOL
		min = nil
		max = nil
	elseif hint == HINT_BOOL then
		print("hint bool cannot be used with hint "..name.."!")
	elseif name == 'log' then
		hint = bit32.bor(hint, HINT_LOG)
	elseif name == 'int' then
		hint = bit32.bor(hint, HINT_INT)
	elseif name == 'freq' then
		hint = bit32.bor(hint, HINT_FREQ)
	end
end

local function doPort(p, i)
	isOut = nil
	isControl = nil
	min = nil
	max = nil
	hint = nil
	name = nil

	local v = p.type
	assert(type(v) == "string", "port type must be a string")
	v:gsub("[ioac ]", porttype)
	assert(isControl ~= nil, "control or audio flag excepted in type!")
	assert(isOut ~= nil, "input or output flag excepted in type!")

	v = p.name or "Port "..tostring(i)
	p.name = v -- to keep new name anchored!
	assert(type(v) == "string", "Port name must be a string")
	name = v

	v = p.min
	min = v
	if min then
	assert(type(v) == "number", "min is not a number")
	end

	v = p.max
	max = v
	if max then
	assert(type(v) == "number", "max is not a number")
	end

	v = p.hint or ""
	hint = 0

	if min then hint = bit32.bor(hint, HINT_HASMIN) end
	if max then hint = bit32.bor(hint, HINT_HASMAX) end

	assert(type(v) == "string", "hint must be a string")
	v:gsub("%w*", porthint)

	if p.default then
		hint = bit32.bor(hint, procdefault(p.default, min or 0, max or 1)) -- hint
	end

	local desc = bit32.bor(
		isOut and TYPE_OUTPUT or TYPE_INPUT,
		isControl and TYPE_CONTROL or TYPE_AUDIO
	)

	setport(PTR, i, name, desc, hint, min, max)
end

for i = 1, #ports do
	local v = ports[i]
	assert(type(v) == "table", "ports must be proper lua array of tables!")
	local ok, msg = pcall(doPort, v, i)
	if not ok then
		error(msg.." (port index is "..tostring(i).." )")
	end
end

-- do dirty stuff to optimize master state and init properties

local master_need = {
	['info'] = true,
	['ports'] = true
}

for k, v in _G do
	if not master_need[k] then
		_G[k] = nil
	end
end

table.freeze(info)
table.freeze(ports)

_REG._setvalue = nil
_REG._setport = nil
master_need = nil
PTR = nil
setvalue = nil
setport = nil
defaults = nil
porttype = nil
porthint = nil
procdefault = nil
doPort = nil
print("Plugin initialization is DONE sucessfully!")
_REG._collect() -- collect all possible garbage
_REG._collect = nil
