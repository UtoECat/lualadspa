-- GNU GPL License
-- Copyritht (C) UtoECat 2022-2023
-- see lualadspa LICENSE File for more information

info = {
	name = "Volume Normalizer", 
	label = "volnormalizer",
	maker = "UtoECat",
	copyright = "GNU GPL",
	realtime = true,
	luaLadspaVersionMinor = 1,
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
		name = "Volume",
		min  = 0.01,
		max  = 1.5,
		default = 1
	}, {
		type = "ic",
		name = "Coefficient",
		min  = 0.005,
		max  = 0.09,
		default = "low"
	}
}

local function sign(v)
	if v > 0 then return 1
	elseif v < 0 then return -1
	else return 1 end
end

local lim -- imported from run. volume limit
local ak -- imported from run. coefficient

local amp_cache = {
	0, 0
}

local function proc(out, input, sz, id)
	local maxv = 0
	local amp = amp_cache[id]

	for i = 1, sz do
		local v = input[i]
		local sign = sign(v)
		v = math.abs(v * amp)

		if v > 1.0 then -- overdrive detected
			local old = v
			v = (math.log(v)*sign+1) * lim
			amp = amp / old - 0.001 -- bound amplifier back
		end
		maxv = maxv + v
		out[i] = v * sign * lim
	end
	maxv = (1 - maxv / sz)
	amp  = amp + maxv * ak
	amp_cache[id] = math.max(amp, 0.0001)
end

function run(sz)
	local ou1 = buffers[3]
	local ou2 = buffers[4]
	local in1 = buffers[1]
	local in2 = buffers[2]

	-- here is a way to get control values :D
	lim = buffers[5][1]
	ak = buffers[6][1]

	-- work
	proc(ou1, in1, sz, 1)
	proc(ou2, in2, sz, 2)
end

