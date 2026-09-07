use("io", "string", "types", "errors", "list")
# lets explain this function with simple comments
def TokenizeIT -> # argument declaration
                  plain_string # our argument
                  then # we got the definition of function, then:
    var tp = 0 # this is our pointer for string
    var return_tokens = List.new() # this is the token list
    var tk_ch = string.char_at($plain_string, $tp)  # this is token char
    var bufstr = "" # and this is a temporary buffer
    # lets figure out this expression
    while types.is_char($tk_ch) # if tk_ch variable is a char and
    and types.char_to_int($tk_ch) neq 0 # if tk_ch is not equal to 0 (like \0 in C)
    then                  # then
        if types.is_space($tk_ch) then  # if our tk_ch is a space then we need to push buffer to list
            List.push($return_tokens, $bufstr) # push function
            $bufstr = "" # and we empty the buffer
        elif $tk_ch equ '+' or
             $tk_ch equ '-' or
             $tk_ch equ '/' or
             $tk_ch equ '*' or
             $tk_ch equ '(' or
             $tk_ch equ ')' or
             $tk_ch equ '^' or
             $tk_ch equ '%' or
             $tk_ch equ ',' then # if our tk_ch is a special token (math, parenthesis, etc)
            List.push($return_tokens, $bufstr) # then we need to push what we got in bufstr
            List.push($return_tokens, $tk_ch) # also we need to push our special token
            $bufstr = "" # then again, removing everything from temp buffer
        elif $tk_ch equ '"' then # if our tk_ch is a string quote
            $tp = $tp + 1 # then just go one forward, cuz we are entering a string literal
            $tk_ch = string.char_at($plain_string, $tp) # then get char of this new pos
            while $tk_ch neq '"' then  # then then then then, as long as tk_ch is not the closing quote
                $bufstr = $bufstr + types.char_to_str($tk_ch) # push every single char to bufstr
                $tp = $tp + 1 # and go forward
                $tk_ch = string.char_at($plain_string, $tp) # get char
            end # do it until tk_ch hits another quote
            List.push($return_tokens, $bufstr) # and push what we got inside the string
            $bufstr = "" # again, empty it, you know
        else # alright, if its anything else (not a special token, string or space, maybe a keyword), we accumulate it
            if types.is_space($tk_ch) equ false then # yeah its english, you know what it means
                $bufstr = $bufstr + types.char_to_str($tk_ch) # and we push every char we got to bufstr
            end
        end
        $tp = $tp + 1 # you know that
        $tk_ch = string.char_at($plain_string, $tp) # you know
    end
    List.push($return_tokens, $bufstr) # end of tokenizing, whatever is left in bufstr, just push it to the list
    return $return_tokens # and return our tokens list
end

# lets prepare our code
var mystring =
"""function(10 + 10, 10 ^ 1, ("Hello, World")"""
# triple quotes mean: get everything until you see triple quotes again. that means:
# we can use normal string quotes inside it
# crazy right?
# but this is not the end

var list = TokenizeIT($mystring) # just using our new born beautiful, greatest, master function

while List.iter($list) then
    # alright thats cool isnt it.
    io.print("TOKEN: ", List.next($list), "\n") # yeah pretty sure we are using List.next
    # if we dont use List.next, the loop will go on forever, cuz List.next means: lets go,
    # i get one item, you get the next item, and if we still have items, loop again
end
