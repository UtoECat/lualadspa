-- You can share, use, copy, paste this file and edit it for your needs.
--
-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.


info = {
	name = "Simple Bad HighFreq Filter Example (Buffer and middle value based)", 
	label = "plugbadfilter",
	maker = "LuaLadspa Community",
	copyright = "unlicensed",
	realtime = true,
	luaLadspaVersionMinor = 0,
	luaLadspaVersionMajor = 0
}

ports = {
	{
		type = "ia",
		name = "Input Channel 1", 
	}, { 
		type = "ia",
		name = "Input Channel 2",
	}, {
		type = "oa",
		name = "Output Channel 1",
	}, {
		type = "oa",
		name = "Output Channel 2",
	}, {
		type = "ic",
		name = "Strength",
		min  = 1,
		max  = 50
	}
} 

local tmp1 = {0, 0, 0, 0, pos = 0}
local tmp2 = {0, 0, 0, 0, pos = 0}

local function filter(input, tmp, tmplen)
	local pos = tmp.pos
	local val = 0
	
	-- add value to the buffer
	tmp[pos] = input

	-- sum everything
	for i = 1, tmplen do
		val = val + tmp[i]
	end

	-- get middle value
	val = val / tmplen

	-- increase position
	if pos + 1 >= tmplen then
		tmp.pos = 1
	else
		tmp.pos = pos + 1
	end
	return val
end

function run(sz)
	local ou1 = buffers[3]
	local ou2 = buffers[4]
	local in1 = buffers[1]
	local in2 = buffers[2]

	-- here is a way to get control values :D
	local len = math.floor(buffers[5][1])

	-- filter buffer setup
	if len <= 0 then
		len = 1
	end

	if len > #tmp1 then
		for i = #tmp1, len do
			tmp1[i] = 0
			tmp2[i] = 0
		end
	end

	-- work
	for i = 1, sz do
		ou1[i] = filter(in1[i], tmp1, len)
		ou2[i] = filter(in2[i], tmp2, len)
	end
end

