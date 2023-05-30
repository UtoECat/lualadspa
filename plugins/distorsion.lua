-- Simple Example mixer plugin for Lualadspa plugin developers.
-- You can share, use, copy, paste this file and edit it for your needs.
--
-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.


info = {
	name = "Simple Distorsion Example", 
	label = "plugdist",
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
		name = "Amplifier",
		min  = 1,
		max  = 30
	}, {
		type = "ic",
		name = "Cutoff",
	}
} 

local function sign(v)
	if v > 0 then return 1
	elseif v < 0 then return -1
	else return 0 end
end

local function power(v, amp, lim) 
		local s = sign(v)
		v = v * amp
		return math.min(math.abs(v), lim) * s
end

function run(sz)
	local ou1 = buffers[3]
	local ou2 = buffers[4]
	local in1 = buffers[1]
	local in2 = buffers[2]

	-- here is a way to get control values :D
	local amp = buffers[5][1]
	local cut = buffers[6][1]

	-- work
	for i = 1, sz do
		ou1[i] = power(in1[i], 1+amp, cut)
		ou2[i] = power(in2[i], 1+amp, cut)
	end
end

