-- Simple mixer
local input0 = newport("IN0", true)
local input1 = newport("IN1", true)
local output = newport("OUT", false)
local ffi = require "ffi"

function process(sz)
	-- realtime thread here!
	local buff = ffi.cast("float*", output:buffer(sz))
	local src1 = ffi.cast("float*", input0:buffer(sz))
	local src2 = ffi.cast("float*", input1:buffer(sz))

	for i = 0, sz-1 do
		buff[i] = (src1[i] + src2[i]) / 2.0
	end
end
