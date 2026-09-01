use(
    "io", 
    "list"
)

var mylist = List.new()

List.push($mylist, "Jason", 
        "Stanley", 
        "Scott", 
        "Alice", 
        "Max", 
        "Goodman", 
        "White", 
        "Mary", 
        "Barbara"
)

while List.iter($mylist) then
    var next = List.next($mylist)
    if $next equ "Max" then 
        io.print("I got Max Mayfield!\n")
    elif $next equ "White" then 
        io.print("I got Mr. White\n")
    end
end
