# Original implementation and examples:
# https://github.com/0l3d/brainsuck
use("io", "file", "types", "sys", "errors", "string", "list")

var file_name = ?(sys.get_arg(2))
var file_content = ?(file.read_to_str($file_name))

var memory = ?(List.new(30000)) # Memory Array
var where_is = 0

def suck_in then 
    ?(List.set($memory, $where_is, (?(List.get($memory, $where_is)) + 1) % 256))
end


def suck_de then 
    ?(List.set($memory, $where_is, (?(List.get($memory, $where_is)) - 1) % 256))
end

def suck_move_right then 
    if $where_is < List.len($memory) - 1 then
        $where_is = $where_is + 1
    end
end

def suck_move_left then 
    if $where_is > 0 then
        $where_is = $where_is - 1
    end
end


def get_current then
    return ?(List.get($memory, $where_is))
end

def set_data -> data then
    ?(List.set($memory, $where_is, $data))
end


var i = 0
var code_len = string.len($file_content)
while $i < $code_len then
    var gchar = string.getchar($file_content, $i)
    if $gchar equ '+' then
        suck_in()
    elif $gchar equ '-' then
        suck_de()
    elif $gchar equ '>' then 
        suck_move_right()
    elif $gchar equ '<' then 
        suck_move_left()
    elif $gchar equ '.' then 
        var c = get_current()
        io.print(types.int_to_char($c))
    elif $gchar equ ',' then 
        var c = sys.getchar()
        set_data(types.char_to_int($c))
    elif $gchar equ '[' then
        if get_current() equ 0 then
            var bracket_count = 1
            while $bracket_count > 0 and $i < $code_len - 1 then
                $i = $i + 1
                var ggchar = string.char_at($file_content, $i)
                if $ggchar equ '[' then 
                    $bracket_count = $bracket_count + 1
                elif $ggchar equ ']' then 
                    $bracket_count = $bracket_count - 1
                end
            end
        end
    elif $gchar equ ']' then
        if get_current() neq 0 then
            var bracket_count = 1
            while $bracket_count > 0 and $i > 0 then
                $i = $i - 1
                var ggchar = string.char_at($file_content, $i)
                if $ggchar equ ']' then 
                    $bracket_count = $bracket_count + 1
                elif $ggchar equ '[' then 
                    $bracket_count = $bracket_count - 1
                end
            end
        end
    end
    $i = $i + 1
end

