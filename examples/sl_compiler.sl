# what is this project anyway
# im lookin to build a compiler that takes .sl code down to bytecode or raw assembly
# so why would i write it in C again right? i got my own language now... fast, simple, gets the job done
use("io", "string", "types", "errors", "list", "collections")
def TokenizeIT -> plain_string then
    var tp = 0
    var return_tokens = List.new()
    var tk_ch = string.char_at($plain_string, $tp)
    var bufstr = ""
    while types.is_char($tk_ch) and types.char_to_int($tk_ch) neq 0 then
        if types.is_space($tk_ch) then
            if $bufstr neq "" then
                List.push($return_tokens, $bufstr)
                $bufstr = ""
            end
        elif $tk_ch equ '+' or
             $tk_ch equ '-' or
             $tk_ch equ '/' or
             $tk_ch equ '*' or
             $tk_ch equ '(' or
             $tk_ch equ ')' or
             $tk_ch equ '^' or
             $tk_ch equ '%' or
             $tk_ch equ ',' then
            if ($bufstr neq "") then
                List.push($return_tokens, $bufstr)
            end
            List.push($return_tokens, types.char_to_str($tk_ch))
            $bufstr = ""
        elif $tk_ch equ '"' then
            $tp = $tp + 1
            $tk_ch = string.char_at($plain_string, $tp)
            while $tk_ch neq '"' then
                $bufstr = $bufstr + types.char_to_str($tk_ch)
                $tp = $tp + 1
                $tk_ch = string.char_at($plain_string, $tp)
            end
            List.push($return_tokens, $bufstr)
            $bufstr = ""
        else
            if types.is_space($tk_ch) equ false and $tk_ch neq '\n' then
                $bufstr = $bufstr + types.char_to_str($tk_ch)
            end
        end
        $tp = $tp + 1
        $tk_ch = string.char_at($plain_string, $tp)
    end
    if $bufstr neq "" then
        List.push($return_tokens, $bufstr)
    end
    return $return_tokens
end

var mystring =
"""\n
var myvar = 10 + 10 + 10\n
var secondvar = myvar + 20\n
myvar = secondvar + 10 + 1\n
rdi = r11 + rdx\n
rsp = rdi + rsi\n
"""

# you might be askin why im usin collections here. ain't they just for objects?
# nah sir. collections work just like structs but way smoother to handle
# just spin up a new one and hit it with Collection_Name:new()
# beats wranglin C structs any day of the week
Collections.create_collection(
        "Code",
        "v:asm_tokens",
        "v:vhash",
        "v:nhash",
        "v:fhash",
        "v:output_code",
        "v:extrs",
        "v:stack_p"
)

var asm_code = Code:new()
# these all gotta be lists. for today tho im just handlin mov, add and a few basic vars (not sweatin full expressions yet)
$asm_code.asm_tokens = List.new()
$asm_code.vhash = List.new()
$asm_code.nhash = List.new()
$asm_code.fhash = List.new()
$asm_code.extrs = List.new()
$asm_code.stack_p = 8
# set string to output_code
$asm_code.output_code = ""

var list = TokenizeIT($mystring)

while List.iter($list)  # rockin that allman style for the then/end stuff
then
    var current_token = List.next($list)
    if $current_token equ "+" then
        List.push(
                $asm_code.asm_tokens,
                "add")
    elif $current_token equ "=" then
        List.push(
                $asm_code.asm_tokens,
                "mov")
    elif $current_token equ "var" then
        List.push($asm_code.nhash, List.next($list))
        List.push($asm_code.vhash, $asm_code.stack_p)
        List.push($asm_code.extrs, "[rsp-" + types.int_to_str($asm_code.stack_p) + "]")
        $asm_code.stack_p = $asm_code.stack_p + 8
    else
        var found = false
        var counter = 0
        while List.iter($asm_code.nhash) then
            if List.next($asm_code.nhash) equ $current_token then
                List.push($asm_code.extrs, "[rsp-" + types.int_to_str(List.get($asm_code.vhash, $counter)) + "]")
                $found = true
            end
            $counter = $counter + 1
        end
        if not($found) then
            List.push($asm_code.extrs, $current_token)
        end
    end
end


var instrc_timer = List.len($asm_code.asm_tokens) - 1

var temp_buffer = ""

while $instrc_timer eqg 0 then
    var current_instrc = List.get($asm_code.asm_tokens, $instrc_timer)
    if $current_instrc equ "add" then
        var thing_1 = List.pop($asm_code.extrs)
        var thing_2 = List.pop($asm_code.extrs)
        $temp_buffer = "add r11, " + $thing_2 + "\n" + $temp_buffer
        $temp_buffer = "add r11, " + $thing_1 + "\n" + $temp_buffer
        List.push($asm_code.extrs, "r11")
    elif $current_instrc equ "mov" then
        var thing_1 = List.pop($asm_code.extrs)
        var thing_2 = List.pop($asm_code.extrs)
        $asm_code.output_code = $temp_buffer + "mov " + $thing_2 + ", " + $thing_1 + "\n" + $asm_code.output_code
        $temp_buffer = ""
    end

    $instrc_timer = $instrc_timer - 1
end

io.print("Code: ", $mystring, "\n")

io.print("ASM OUTPUT (Experimental): \n", $asm_code.output_code)
