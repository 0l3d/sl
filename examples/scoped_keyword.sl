use("io", "list")

var b = 0


var out = {
    if $b equ 0 then 
        var mylist = List.new() 
    end
    List.push($mylist, "Hello, World")
    return $mylist
}


if ({ 
    while List.iter($out) then 
        var item = List.next($out)
        if $item equ "Hello, World" then 
            return true
        end 
    end
}) then 
    io.print("CONDITION IS TRUE")
else 
    io.print("CONDITION IS FALSE")
end


