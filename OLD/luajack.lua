local _debug = require "debug"

debug = nil
package.loaded.debug = nil
package.preload.debug = nil

-- 2 args required : table with lowlevel functions and table with args
local LOW, ARGS = ...
