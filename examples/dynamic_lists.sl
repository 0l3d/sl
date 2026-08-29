# For importing dynamic lists 
use(
    "io", # For print 
    "list" # For dynamic list
)

var mylist = List.new() # without arguments, its dynamic list.

List.push($mylist, "Hello, World")
List.push($mylist, 10)
List.push($mylist, true)
List.push($mylist, 'A')

# Stack with List
io.print("Peek: ", List.peek($mylist), "\n")
var last_one = List.pop($mylist)
io.print("Pop: ", $last_one, "\n")
$last_one = List.pop($mylist)
io.print("Pop, Pop: ", $last_one, "\n")
io.print("Peek: ", List.peek($mylist), "\n")

# Classic List
List.set($mylist, 
        0,  # index
        "Hello")

io.print("0. index: ", List.get($mylist, 
            0 # index
            ), "\n")

io.print("Len: ", List.len($mylist), "\n")
