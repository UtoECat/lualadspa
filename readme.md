# LuaJACK
LuaJACK allows you write *simple* jack clients on lua.   
*Simple* there means that they will only handle realtime audio data process function and can create some ports at stage of initialization...

# luajack usage
```
Copyright (C) UtoECat 2023. All rights reserved.
GNU GPL License. No any warrianty!
usage:
	./luajack [code.lua] 			- runs file
	./luajack -h 				- shows help
	./luajack -v 				- shows version
	./luajack [code.lua] [server name]	- runs file for jack server with specified name
```
# API

`port = newport(name :string, isInput :boolean, isTerminal :boolean)`    
Creates new JACK port.
- **port** is returned magical user-data object :p
- **name** is name of JACK port. Must be unique for one client...
- **isInput** specifies is this port will be used as input for audio data, or as ouput of audio data.
- **isTerminal** specifies is port not depends of other ports data.   
**warning** : if port was not opened, this function throws an error      
**warning** : you can't create more ports when JACK client is activated (in process function :) )

`port:close()`    
Closes opened JACK port. 

`val = port:connected()`    
Returns true if port is connected to any other port.
- **val** is returned boolean type     
**warning** : if port was closed, this function returns nil

`lud = port:buffer(size :integer)`   
Returns light user data, pointed to buffer of port.
- **size** is size of all buffers, passed as first argument in `process` function!     
**IMPORTANT NOTE!** Buffer location may be **CHANGED** on next JACK tick, so you **MUST** recall it agan, and again in `process` function, and **NOT** keep this lightuserdata anywhere else!

`rate = getrate()`    
Returns JACK Server samplerate (integer).   

Aaaand...    
`function process(size :integer) --[[ realtime code ]] end`    
Function that **MUST** be declared by script. After initialization this function will be runned in *realtime thread*, that means you **SHOULD NOT CALL ANY LOCKABLE FUNCTIONS HERE**, because it can protuce gaps in audioflow. (and JACK Server can **KILL** your script :) )

# Tips

It's useful to cast returned lightuserdata by `port:buffer()` to `float*` type.
```lua
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
```
