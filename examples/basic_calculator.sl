use("io", "string", "list", "types")

io.print("Welcome to Basic Calculator.\n")
io.print("Expression Syntax: n<space><op><space>n...\n")
var input = io.input("EXPR>")

def tokenizer -> expr_string then 
    var i = 0
    var len = string.split($expr_string, " ")
    return $len
end


var nodes = tokenizer($input)
List.push($nodes, "+")
var nums = List.new()
var last_op = "+" 
var num = 0
while List.iter($nodes) then 
    var token = List.next($nodes) 
    if types.is_digit($token) then
        $num = types.str_to_int($token)
    else
        if $last_op equ "+" then
            List.push($nums, $num)
        elif $last_op equ "-" then 
            List.push($nums, -1 * $num)
        elif $last_op equ "*" then 
            List.push($nums, List.pop($nums) * $num)
        elif $last_op equ "/" then 
            List.push($nums, List.pop($nums) / $num)
        end
        $num = 0
        $last_op = $token
    end
end


var result = 0

while List.iter($nums) then 
    $result = $result + List.next($nums)
end

io.print("Result: ", $result, "\n")
