# Original: https://github.com/0l3d/letsconvert
# Unlike the original one, this version is cross platform with SL.
use("io", "types")
var num_str = io.input("Enter a Number: ")


# Out string 
var nums = ""

# Hexadecimal
var num = types.str_to_int($num_str)
while $num > 0 then
    var rem = $num % 16
    if $rem > 9 then 
        $nums = types.char_to_str(
                types.int_to_char($rem + 55)
                ) + $nums
    else 
        $nums = types.int_to_str($rem) + $nums
    end
    $num = $num / 16
end

io.print("HEX: 0x",$nums, "\n")

$nums = ""

# OCTAL
var num = types.str_to_int($num_str)
while $num > 0 then
    var rem = $num % 8
    $nums = types.int_to_str($rem) + $nums
    $num = $num / 8
end


io.print("OCTAL: 0o",$nums, "\n")

$nums = ""

# BINARY
var num = types.str_to_int($num_str)
while $num > 0 then
    var rem = $num % 2
    $nums = types.int_to_str($rem) + $nums
    $num = $num / 2
end


io.print("BINARY: 0b",$nums, "\n")

