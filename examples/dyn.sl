# For *LINUX* but works on every platform, variables: $SYSTEM_*, eq: $SYSTEM_WIN, $SYSTEM_FREEBSD, $SYSTEM_HAIKU
use("dyn")

var lib = dyn.open_lib("libc.so.6")

var printf = dyn.find_symbol($lib, "printf")

dyn.call($printf, 4096, $DYN_INTEGER, "Hello, from SL!\n")

dyn.free($lib)
