-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.

-- Testing for errors in activate/deactivate/run functions.
info = {
	name = "Memory Flood Generator", 
	label = "plugOOMerr",
	maker = "InSanity Community XD",
	copyright = "FUCK YOU License",
	realtime = true,
	luaLadspaVersionMinor = 0,
	luaLadspaVersionMajor = 0
}

ports = {
	{
		type = "ia",
		name = "Audio input",
		min  = -1.0,
		max  = 1.0
	},{
		type = "oa",
		name = "Audio output",
		min  = -1.0,
		max  = 1.00
	}
}

local anchor = {}

function run(sz)
	local out  = buffers[2]
	local in1  = buffers[1]

	for i = 1, 1000 do
		t = {}
		anchor[#anchor+1] = t
		for i = 1, 1000 do
			t[i] = tostring(i):rep(100) -- horrible things happens there
		end
	end

	print(t)
	if math.random(1, 3) == 1 then
		error("oh no")
	end

end

function activate()
	print "Hi!"
	error "frick"
end

function deactivate()
	print "Bye!"
	error "nobody needs you"
end
