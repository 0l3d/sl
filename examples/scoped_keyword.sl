use("io", "list")

var b = 0


var out = then 
    if $b equ 0 then 
        var mylist = List.new() 
    end
    List.push($mylist, "Hello, World")
    return $mylist
end


if (then 
    while List.iter($out) then 
        var item = List.next($out)
        if $item equ "Hello, World" then 
            return true
        end 
    end
end) then 
    io.print("CONDITION IS TRUE")
else 
    io.print("CONDITION IS FALSE")
end


