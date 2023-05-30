-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.

-- This plugins return SINE signal to the control port :)
info = {
	name = "Control Sine Generator", 
	label = "plugsingen",
	maker = "LuaLadspa Community",
	copyright = "none",
	realtime = true,
	luaLadspaVersionMinor = 0,
	luaLadspaVersionMajor = 0
}

ports = {
	{
		type = "ic",
		name = "Frequency",
		hint = "freq log",
		min  = 0.0,
		max  = 1.00
	}, {
		type = "ic",
		name = "Minimal",
		min  = -1.0,
		max  = 1.0
	},{
		type = "ic",
		name = "Maximal",
		min  = -1.0,
		max  = 1.00
	}, {
		type = "oc",
		name = "Output Control"
	}
}

local clock = 0

function run(sz)
	-- here is a way to get control values :D
	local freq = buffers[1][1] / sz
	local min  = buffers[2][1]
	local max  = buffers[3][1]
	local out  = buffers[4]
	local rate = ladspa.getSampleRate() / sz

	local diff = max - min
	out[1] = math.sin(clock*freq*rate)*diff + min
	clock = clock + 1/rate
end

