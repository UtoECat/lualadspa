-- Volume normalizer
local input0 = newport("IN0", true)
local input1 = newport("IN1", true)
local outpul = newport("OUL", false)
local outpur = newport("OUR", false)
local ffi = require "ffi"

local max = 1.0
local minspd = 0.0001
local vol = 1.0
local spd = 0.01
local grow = 0.15
local fade = 2

local function procpair(buff, src, sz) 
	local v
	for i = 0, sz-1 do
		v = src[i] * vol
		if math.abs(v) > max then
			vol = math.abs(1/src[i])
			spd = spd / fade
			v = src[i] * vol
		end
		buff[i] = v
	end
end

function process(sz)
	-- realtime thread here!
	local bufl = ffi.cast("float*", outpul:buffer(sz))
	local bufr = ffi.cast("float*", outpur:buffer(sz))
	local src1 = ffi.cast("float*", input0:buffer(sz))
	local src2 = ffi.cast("float*", input1:buffer(sz))

	procpair(bufl, src1, sz)
	procpair(bufr, src2, sz)

	if spd < minspd then
		spd = minspd
	end

	spd = math.abs(spd* (1+grow))
	vol = vol + (spd)
end
