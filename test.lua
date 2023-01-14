-- Noise generator :)
local output = newport("output", false, true)
local ffi = require "ffi"

function process(sz)
	-- realtime thread here!
	local buff = ffi.cast("float*", output:buffer(sz))
	for i = 0, sz-1 do
		buff[i] = math.random(-100, 100) / 100
	end
end
