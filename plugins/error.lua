-- This AND ONLY THIS file is released UNLICENSED into PUBLIC DOMAIN,
-- PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- See http://creativecommons.org/licenses/publicdomain for more info.

-- Testing for errors in activate/deactivate/run functions.
info = {
	name = "Error Generator", 
	label = "plugsinerr",
	maker = "No Sanity Community",
	copyright = "none",
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


function run(sz)
	local out  = buffers[2]
	local in1  = buffers[1]

	if math.random(1, 3) == 1 then
		error("oh no")
	end

	for i = 1, sz do
		out[i] = in1[i] * (math.random() * 2.0 - 1.0)
	end
end

function activate()
	error "frick"
end

function deactivate()
	error "nobody needs you"
end
