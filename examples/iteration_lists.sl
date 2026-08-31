use("io", "list")

var mylist = List.new()

List.push($mylist, "Jason")
List.push($mylist, "Stanley")
List.push($mylist, "Scott")
List.push($mylist, "Alice")
List.push($mylist, "Max")
List.push($mylist, "Goodman")
List.push($mylist, "White")
List.push($mylist, "Mary")
List.push($mylist, "Barbara")

while List.iter($mylist) then
    var next = List.next($mylist)
    if $next equ "Max" then 
        io.print("I got Max Mayfield!\n")
    elif $next equ "White" then 
        io.print("I got Mr. White\n")
    end
end
