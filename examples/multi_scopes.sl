use("io", "types")

io.print("$first_number + 10 + $second_number = ",
{ 
    var out = types.str_to_int(io.input("Enter first number: "))
    return $out 
} 
+ 
10 
+ 
{ 
    var out = types.str_to_int(io.input("Enter second number: "))
    return $out 
}, 
"\n")
