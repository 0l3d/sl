use("io", "list")

var mylist = List.new()

List.push($mylist, "Jason")
List.push($mylist, "Stanley")
List.push($mylist, "Scott")
List.push($mylist, "Alice")
List.push($mylist, "Max")
List.push($mylist, "Mary")
List.push($mylist, "Barbara")

while List.iter($mylist) then
    if List.next($mylist) equ "Max" then 
        io.print("I got Max Mayfield!\n")
    end
end
