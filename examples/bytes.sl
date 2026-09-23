use("types", "file", "io")


var outer = types.pack("RAW STR", 1, 2, 0x0A)
file.write("bytes.out", $outer)
