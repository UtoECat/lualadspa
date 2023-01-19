-- Simple mixer
local input0 = newport("IN0", true)
local output = newport("OUT", false)
local ffi = require "ffi"

function process(sz)
	-- realtime thread here!
	local buff = ffi.cast("float*", output:buffer(sz))
	local src1 = ffi.cast("float*", input0:buffer(sz))

	for i = 0, sz-1 do
		buff[i] = math.min(math.max(src1[i] * 10, -1.0), 1.0)
	end
end
