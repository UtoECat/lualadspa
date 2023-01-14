local input  = newport("input", true)
local output = newport("output", false)
local ffi = require "ffi"

function process(cnt)
	-- realtime thread here!
	local inb = ffi.cast("float*", input:buffer(cnt))
	local oub = ffi.cast("float*", output:buffer(cnt))
	for i = 0, cnt-1 do
		oub[i] = inb[i] * 10
	end
end
