use("io", "list")

var mylist = List.new()

List.push($mylist, "Jason")
List.push($mylist, "Stanley")
List.push($mylist, "Scott")
List.push($mylist, "Alice")
List.push($mylist, "Max")
List.push($mylist, "Mary")
List.push($mylist, "Barbara")


var i = List.len($mylist)
while $i > 0 then 
    io.print(List.next($mylist), "\n")
    $i = $i - 1
end
