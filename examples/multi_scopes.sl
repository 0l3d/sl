use("io", "types")

io.print("$first_number + 10 + $second_number = ",
then 
    var out = types.str_to_int(io.input("Enter first number: "))
    return $out 
end 
+ 
10 
+ 
then 
    var out = types.str_to_int(io.input("Enter second number: "))
    return $out 
end, 
"\n")
