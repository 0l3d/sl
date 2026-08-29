use(
    "io", 
    "extra", 
    "types"
)

var rand = rand.random(0, 100)

var num = 0
while $num neq $rand then 
    $num = types.str_to_int(io.input("Enter number: "))
    if $num > $rand then 
        io.print("Try smaller.\n")
    end 

    if $num < $rand then
        io.print("Try bigger.\n")
    end
     if $num equ $rand then 
        break
    end
end

io.print("You made it! Number is : ", $rand, "\n")
