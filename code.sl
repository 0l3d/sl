import lib.sl

use("io")

var enable_notify = true
io.print("Hello world", "\n", 10, "\n", true, "\n", false, "\n", 10.1, "\n", 'A', "\n")
$enable_notify = false # setting false again
io.print("10 + 200 = ", add(10, 200), "\n")
