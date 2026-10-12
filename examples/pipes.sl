# See documentation with ex_doc.sl
##Pipes
#?Pipe Operator passes the output of one function as the input to the next function. 
#?Makes code more readable and easier to follow.
#%Usage:
#?var name = "Enter name: " |> io.input 
#%Not included in the base language. (An extension of sl_lang)
use("io", "types")

var name = "Enter name: " |> io.input
var call_number = "Enter call number: " 
				  |> io.input 
				  |> types.str_to_int

"Name: " + $name + " " |> io.print("Call Number: ", $call_number, "\n")


##More advanced usage (similar to F#)
#?Define a function called toInt that takes a value as input.
#?Define another function called input that returns a string.
#?Combine them like this:
#?var number1 = input("Enter num1: ") |> toInt 
#?var number2 = input("Enter num2: ") |> toInt 
#?$number1 + $number2 |> io.print("\n")
#%Function definitions are inside the script. Take a look!
def toInt -> val then 
	if $val |> types.is_string then 
		return types.str_to_int($val)
	end
end

def input -> str then 
	return io.input($str)
end

# F# Like usage:

var number1 = input("Enter num1: ") |> toInt 
var number2 = input("Enter num2: ") |> toInt 
$number1 + $number2 |> io.print("\n")
#? 
##ExDOC Supported Script

