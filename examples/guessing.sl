var rand = random(0, 100)

var num = 0
while $num neq $rand then 
    $num = str_to_int(input("Enter number: "))
    if $num > $rand then 
        print("Try smaller.\n");
    end 
    if $num < $rand then 
        print("Try bigger.\n")
    end
end

print("You made it! Number is : ", $rand, "\n")
