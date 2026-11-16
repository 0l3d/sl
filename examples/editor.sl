#!/usr/bin/env sl
# For posix based systems.
use("console", "io", "types", "sys", "string", "list", "errors", "file", "time")

# EDITOR SETTINGS

# Bottom indicator background color
var indicator_background = $COLOR_WHITE

# Bottom indicator text color
var indicator_foreground = $COLOR_BLACK

# Keyword color
var keyword_highlighting = $COLOR_MAGENTA

# Variable ($name) color
var variable_highlighting = $COLOR_YELLOW

# Selection colors
var selection_background = $COLOR_WHITE
var selection_foreground = $COLOR_BLACK

# Page DOWN/UP Size 
var scroll = 20

# Enable highlighting
var enable_highlighting = true

# Editor background color 
var background_color = $COLOR_BLACK

# Editor background switch
var background_color_enabled = false

# EDITOR SETTINGS

var file_name = sys.get_arg(2)
var sl_runtime = sys.get_arg(0)
var highlight_vars = false
if errors.bool($file_name) then
    $file_name = "test.sl"
end

if $enable_highlighting then 
    var ext = string.split($file_name, ".")
    var sl_highlighting = List.new()
    if not(errors.bool($ext)) then
        var exts = List.get($ext, List.len($ext) - 1)
        if $exts equ "sl" or $exts equ "SL" then
            $highlight_vars = true
            List.push(
            $sl_highlighting,
            "var",
            "if",
            "elif",
            "else",
            "then",
            "def",
            "use",
            "end",
            "return",
            "while",
            "break",
            "continue",
            "equ",
            "eqg",
            "eql",
            "neq")
        elif $exts equ "c" or $exts equ "C" or $exts equ "h" or $exts equ "H" then
            List.push(
            $sl_highlighting,
            "int",
            "void",
            "struct",
            "enum",
            "float",
            "double",
            "char",
            "size_t",
            "if",
            "else",
            "return",
            "while",
            "break",
            "for",
            "unsigned",
            "switch",
            "static",
            "short",
            "const",
            "auto",
            "long",
            "continue",
            "#define",
            "#include",
            "#ifdef",
            "#ifndef",
            "#endif",
            "#else",
            "#undef",
            "#pragma",
            "#error",
            "#warning")
        end
    end
end
var lines = List.new()

var read = file.read_to_str($file_name)


if not(errors.bool($read)) then
    var spltnewlines = string.split($read, "\n")
    if not(errors.bool($spltnewlines)) then
        while List.iter($spltnewlines) then
            var line = List.next($spltnewlines)

            if not(errors.bool($line)) then
                List.push($lines, $line)
            end
        end

    end
end

console.enter_alt_screen()
console.autowrap(false)
console.clear()

var screen_width = console.get_width() - 1
var screen_height = console.get_height() - 1

console.scroll_area(0, $screen_height - 1)

var status_message = "nomsg"

var cursor_x = 0
var cursor_y = 0

var rendering_start_line = 0
var total_renderable = $screen_height - 1
var rendering_end_line = $rendering_start_line + $total_renderable

var actual_y = $rendering_start_line + $cursor_y
var actual_x = 0

var view_x = 0
var view_col = 0
var scr_x = 0
var need_full = false
var sel_on = false
var sel_drawn = false
var sel_ay = 0
var sel_ax = 0
var sel_sy = 0
var sel_sx = 0
var sel_ey = 0
var sel_ex = 0
var clip = List.new()
var search_mode = false
var search_buf = ""
var search_query = ""

def render_indicator then
    console.cursor_position(0, $screen_height)
    console.clear_line()
    console.background_color($indicator_background)
    console.foreground_color($indicator_foreground)

    if $search_mode then
        io.print(" SEARCH: ", $search_buf, "_  | Enter: find next | Esc: cancel")
    else
    io.print(
        "CURSOR x:",
        $cursor_x,
        " y:",
        $actual_y,
        " | msg:",
        $status_message,
        " | File Name: ",
        $file_name,
        " | TIME: ",
        time.string("%H:%M"),
		" | F2 for Help "
    )
    end

    console.reset_color()
end

var smellslikeyouchangedsomethingspirit = 2

def set_cursor_pos then
    console.cursor_position($cursor_x, $cursor_y)
end

var buffer = ""

def update_cursor_x then
    var cx = 0
    var i = 0
    var buf_len = string.len($buffer)
    if errors.bool($buf_len) then
        $buf_len = 0
    end
    var limit = $actual_x
    if $limit > $buf_len then
        $limit = $buf_len
        $actual_x = $limit
    end
    while $i < $limit then
        var c = string.char_at($buffer, $i)
        var cs = types.char_to_str($c)
        if $cs equ "\t" then
            var next_tab_stop = $cx + 4
            var remainder = $next_tab_stop % 8
            if $remainder neq 0 then
                $cx = $next_tab_stop + (8 - $remainder)
            else
                $cx = $next_tab_stop
            end
        else
            $cx = $cx + 1
        end
        $i = $i + 1
    end
    $cursor_x = $cx
end

def str_part -> s, a, b then
    if $b > $a then
        var r = string.slice($s, $a, $b)
        if errors.bool($r) then
            return ""
        end
        return $r
    end
    return ""
end

def col_at -> n then
    var cx = 0
    var i = 0
    var buf_len = string.len($buffer)
    if errors.bool($buf_len) then
        $buf_len = 0
    end
    var limit = $n
    if $limit > $buf_len then
        $limit = $buf_len
    end
    while $i < $limit then
        var c = string.char_at($buffer, $i)
        var cs = types.char_to_str($c)
        if $cs equ "\t" then
            var next_tab_stop = $cx + 4
            var remainder = $next_tab_stop % 8
            if $remainder neq 0 then
                $cx = $next_tab_stop + (8 - $remainder)
            else
                $cx = $next_tab_stop
            end
        else
            $cx = $cx + 1
        end
        $i = $i + 1
    end
    return $cx
end

def cur_line then
    var l = List.get($lines, $rendering_start_line + $cursor_y)
    if errors.bool($l) then
        return ""
    end
    return $l
end

def set_line -> idx, text then
    if errors.bool(List.set($lines, $idx, $text)) then
        List.push($lines, $text)
    end
    return 0
end

def goto_pos -> y, x then
    if $y < $rendering_start_line or $y eqg $rendering_start_line + $total_renderable then
        var st = $y - ($total_renderable / 2)
        if $st < 0 then
            $st = 0
        end
        $rendering_start_line = $st
        $rendering_end_line = $st + $total_renderable
    end
    $cursor_y = $y - $rendering_start_line
    $actual_x = $x
    $buffer = cur_line()
    update_cursor_x()
    $need_full = true
    return 0
end

def fix_hscroll then
    $buffer = cur_line()
    update_cursor_x()
    var old_view = $view_x
    if $actual_x < $view_x then
        $view_x = $actual_x - 10
        if $view_x < 0 then
            $view_x = 0
        end
    end
    var diff = $cursor_x - col_at($view_x)
    if $diff eqg $screen_width then
        $view_x = $actual_x - $screen_width + 11
        if $view_x > $actual_x then
            $view_x = $actual_x
        end
        if $view_x < 0 then
            $view_x = 0
        end
        $diff = $cursor_x - col_at($view_x)
        while $diff eqg $screen_width and $view_x < $actual_x then
            $view_x = $view_x + 1
            $diff = $cursor_x - col_at($view_x)
        end
    end
    $view_col = col_at($view_x)
    $scr_x = $cursor_x - $view_col
    if $old_view neq $view_x then
        $smellslikeyouchangedsomethingspirit = 2
    end
    return 0
end

def sel_normalize then
    if $sel_on then
        var cy = $rendering_start_line + $cursor_y
        var anchor_first = false
        if $sel_ay < $cy then
            $anchor_first = true
        elif $sel_ay equ $cy then
            if $sel_ax < $actual_x then
                $anchor_first = true
            end
        end
        if $anchor_first then
            $sel_sy = $sel_ay
            $sel_sx = $sel_ax
            $sel_ey = $cy
            $sel_ex = $actual_x
        else
            $sel_sy = $cy
            $sel_sx = $actual_x
            $sel_ey = $sel_ay
            $sel_ex = $sel_ax
        end
        if $sel_sy equ $sel_ey and $sel_sx equ $sel_ex then
            $sel_on = false
        end
    end
    return 0
end

def tok_kind -> tok then
    if $highlight_vars then
        if types.char_to_int(string.char_at($tok, 0)) equ 36 then
            return 2
        end
    end
    if types.is_int(List.find($sl_highlighting, $tok)) then
        return 1
    end
    return 0
end

def set_token_color -> kind then
    if $kind equ 2 then
        console.foreground_color($variable_highlighting)
    else
        console.foreground_color($keyword_highlighting)
    end
    return 0
end

def render_highlighted -> str then 
    var string_indexer = 0 
    var string_len = string.len($str)
    var temp_buffer = ""
    var print_buffer = ""    
    while $string_indexer < $string_len then
        var charc = string.char_at($str, $string_indexer)
        var char_str = types.char_to_str($charc)

        var is_delimiter = false
        if types.is_space($charc) then
            $is_delimiter = true
        elif $char_str equ "(" or $char_str equ ")" or $char_str equ "{" or $char_str equ "}" or $char_str equ "[" or $char_str equ "]" or $char_str equ ";" or $char_str equ "," or $char_str equ ":" then
            $is_delimiter = true
        end

        if $is_delimiter then
            if string.len($temp_buffer) > 0 then
                var tk = tok_kind($temp_buffer)
                if $tk > 0 then
                    if string.len($print_buffer) > 0 then
                        io.print($print_buffer)
                        $print_buffer = ""
                    end
                    set_token_color($tk)
                    io.print($temp_buffer)
                    
                    console.reset_color()
                    if $background_color_enabled then
                        console.background_color($background_color)
                    end
                else
                    $print_buffer = $print_buffer + $temp_buffer
                end
                $temp_buffer = ""
            end

            $print_buffer = $print_buffer + $char_str
        else
            $temp_buffer = $temp_buffer + $char_str
        end 

        $string_indexer = $string_indexer + 1
    end
    
    if string.len($temp_buffer) > 0 then
        var tk = tok_kind($temp_buffer)
        if $tk > 0 then
            if string.len($print_buffer) > 0 then
                io.print($print_buffer)
                $print_buffer = ""
            end
            set_token_color($tk)
            io.print($temp_buffer)
            console.reset_color()
            if $background_color_enabled then
                console.background_color($background_color)
            end
        else
            $print_buffer = $print_buffer + $temp_buffer
        end
    end
    
    if string.len($print_buffer) > 0 then
        io.print($print_buffer)
    end
end

def out_text -> s then
    if string.len($s) > 0 then
        if $enable_highlighting then
            render_highlighted($s)
        else
            io.print($s)
        end
    end
    return 0
end

def draw_line -> line, y then
    var n = string.len($line)
    var a = 0
    var b = 0
    if $sel_on then
        if $y eqg $sel_sy and $y eql $sel_ey then
            if $y equ $sel_sy then
                $a = $sel_sx
            end
            if $y equ $sel_ey then
                $b = $sel_ex
            else
                $b = $n
            end
        end
    end
    if $b > $a then
        var s1 = $a
        if $s1 < $view_x then
            $s1 = $view_x
        end
        var s2 = $b
        if $s2 < $view_x then
            $s2 = $view_x
        end
        out_text(str_part($line, $view_x, $a))
        if $b > $s1 then
            console.background_color($selection_background)
            console.foreground_color($selection_foreground)
            io.print(str_part($line, $s1, $b))
            console.reset_color()
            if $background_color_enabled then
                console.background_color($background_color)
            end
        end
        out_text(str_part($line, $s2, $n))
    else
        out_text(str_part($line, $view_x, $n))
    end
    return 0
end

def line_renderer -> all then
    var i = $rendering_start_line
    var screen_y = 0
    if $background_color_enabled then 
        console.background_color($background_color)
    end
    
    if $all equ 2 then 
        while $screen_y < $total_renderable then
            console.cursor_position(0, $screen_y)
            console.clear_line()             
            var line = List.get($lines, $i)
            if not(errors.bool($line)) then
                draw_line($line, $i)
            end
            
            
            $i = $i + 1
            $screen_y = $screen_y + 1
        end
    elif $all equ 1 then 
        var actual_y = $rendering_start_line + $cursor_y
        $screen_y = $cursor_y
        console.cursor_position(0, $screen_y)
        console.clear_line()         
        var line = List.get($lines, $actual_y)
        if not(errors.bool($line)) then
            draw_line($line, $actual_y)
        end
    end
end

def render_screen then
    console.begin_update()
    
    console.cursor_visibility(false)

    fix_hscroll()
    sel_normalize()
    if $sel_on or $sel_drawn or $need_full then
        $smellslikeyouchangedsomethingspirit = 2
    end
    $sel_drawn = $sel_on
    $need_full = false
    
    if $smellslikeyouchangedsomethingspirit > 0 then
        line_renderer($smellslikeyouchangedsomethingspirit)
        $smellslikeyouchangedsomethingspirit = 0
    end

    render_indicator()
    
    if $scr_x < $screen_width and $cursor_y < $screen_height then
        console.cursor_position($scr_x, $cursor_y)
    else
        console.cursor_position(0, $screen_height - 1)
    end
    
    console.cursor_visibility(true)
    io.flush()
    console.end_update()
end

set_cursor_pos()
console.raw_mode(true)

def new_line -> middler, length then
    var actual_y = $rendering_start_line + $cursor_y
    var left_part = ""
    var right_part = ""

    if $middler then
        if $actual_x > 0 then
            $left_part = string.slice($buffer, 0, $actual_x)
            if errors.bool($left_part) then
                $left_part = ""
            end
        end

        if $actual_x < $length then
            $right_part = string.slice($buffer, $actual_x, $length)
            if errors.bool($right_part) then
                $right_part = ""
            end
        end
    else
        $left_part = $buffer
        $right_part = ""
    end

    if errors.bool(List.set($lines, $actual_y, $left_part)) then
        List.push($lines, $left_part)
    end

    var next_y = $actual_y + 1
    var total_lines = List.len($lines)

    if $next_y eqg $total_lines then
        List.push($lines, $right_part)
    else
        List.push($lines, "")
        var i = List.len($lines) - 1
        while $i > $next_y then
            var prev_i = $i - 1
            var prev_val = List.get($lines, $prev_i)
            List.set($lines, $i, $prev_val)
            $i = $i - 1
        end
        List.set($lines, $next_y, $right_part)
    end

    if $cursor_y eqg $total_renderable then
        $rendering_start_line = $rendering_start_line + 1
        console.scroll_up(1)        
        $smellslikeyouchangedsomethingspirit = 2 
    else
        console.save_cursor()
        console.cursor_position(0, $cursor_y + 1)
        console.insert_line(1)         
        console.load_cursor()
        
        $cursor_y = $cursor_y + 1
        
        var current_y = $cursor_y
        $cursor_y = $current_y - 1
        line_renderer(1)
        $cursor_y = $current_y
        line_renderer(1)
        
        $smellslikeyouchangedsomethingspirit = 0
    end    
    $actual_x = 0
    $cursor_x = 0
    $buffer = ""
    $rendering_end_line = $rendering_start_line + $total_renderable
end

def update_screen_size then
    $screen_width = console.get_width() - 1
    $screen_height = console.get_height() - 1
    $total_renderable = $screen_height - 1
    $rendering_end_line = $rendering_start_line + $total_renderable

    if $cursor_y eqg $total_renderable then
        $cursor_y = $total_renderable - 1
    end

    if $cursor_y < 0 then
        $cursor_y = 0
    end
    $smellslikeyouchangedsomethingspirit = 2
end

def backspace_b then
    var actual_y = $rendering_start_line + $cursor_y

    if $actual_x > 0 then
        var remove_index = $actual_x - 1
        var new_buffer = string.remove_at($buffer, $remove_index)

        if errors.bool($new_buffer) then
            $status_message = "Could not delete character."
        else
            $actual_x = $actual_x - 1
            $buffer = $new_buffer

            if errors.bool(List.set($lines, $actual_y, $buffer)) then
                List.push($lines, $buffer)
            end
            update_cursor_x()
        end
        $smellslikeyouchangedsomethingspirit = 1
    else
        if $actual_y > 0 then
            var prev_y = $actual_y - 1
            var prev_line = List.get($lines, $prev_y)
            var current_line = List.get($lines, $actual_y)

            if errors.bool($prev_line) then
                $prev_line = ""
            end
            if errors.bool($current_line) then
                $current_line = ""
            end

            var prev_len = string.len($prev_line)
            if errors.bool($prev_len) then
                $prev_len = 0
            end

            if $current_line equ "" then
                List.remove($lines, $actual_y)
                $buffer = $prev_line
            else
                var merged_line = $prev_line + $current_line
                List.set($lines, $prev_y, $merged_line)
                List.remove($lines, $actual_y)
                $buffer = $merged_line
            end
            
            $actual_x = $prev_len
            update_cursor_x()

            if $cursor_y > 0 then
                $cursor_y = $cursor_y - 1
            else
                if $rendering_start_line > 0 then
                    $rendering_start_line = $rendering_start_line - 1
                    $rendering_end_line = $rendering_start_line + $total_renderable
                end
            end

            $smellslikeyouchangedsomethingspirit = 2
        else
            $status_message = "Theres no text to delete."
        end
    end
end

def ascii_entered then
    var actual_y = $rendering_start_line + $cursor_y

    $actual_x = $actual_x + 1
    update_cursor_x()

    if errors.bool(List.set($lines, $actual_y, $buffer)) then
        List.push($lines, $buffer)
    end
    $smellslikeyouchangedsomethingspirit = 1
end

def page_down then
    var len = List.len($lines)
    var old_start = $rendering_start_line

    if $rendering_start_line + $total_renderable < $len then
        $rendering_start_line = $rendering_start_line + $scroll
        
        if $rendering_start_line + $total_renderable > $len then
            $rendering_start_line = $len - $total_renderable
            if $rendering_start_line < 0 then
                $rendering_start_line = 0
            end
        end
    end

    if $old_start neq $rendering_start_line then
        $smellslikeyouchangedsomethingspirit = 2
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
end

def page_up then
    var old_start = $rendering_start_line

    if $rendering_start_line > 0 then
        $rendering_start_line = $rendering_start_line - $scroll
        if $rendering_start_line < 0 then
            $rendering_start_line = 0
        end
    end

    if $old_start neq $rendering_start_line then
        $smellslikeyouchangedsomethingspirit = 2
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
end

def ascii_entered_middle -> charkey, length then
    var left = ""
    var right = ""

    if $actual_x > 0 then
        $left = string.slice($buffer, 0, $actual_x)
        if errors.bool($left) then
            $left = ""
        end
    end

    if $actual_x < $length then
        $right = string.slice($buffer, $actual_x, $length)
        if errors.bool($right) then
            $right = ""
        end
    end

    var full_buffer = $left + types.char_to_str($charkey) + $right
    var actual_y = $rendering_start_line + $cursor_y

    $buffer = $full_buffer

    if errors.bool(List.set($lines, $actual_y, $buffer)) then
        List.push($lines, $buffer)
    end
    $actual_x = $actual_x + 1
    update_cursor_x()
    
    $smellslikeyouchangedsomethingspirit = 1
end


def insert_line_at -> idx, text then
    List.push($lines, "")
    var i = List.len($lines) - 1
    while $i > $idx then
        var prev_val = List.get($lines, $i - 1)
        List.set($lines, $i, $prev_val)
        $i = $i - 1
    end
    List.set($lines, $idx, $text)
    return 0
end

def sel_delete then
    sel_normalize()
    if $sel_on then
        var first_line = List.get($lines, $sel_sy)
        var last_line = List.get($lines, $sel_ey)
        if errors.bool($first_line) then
            $first_line = ""
        end
        if errors.bool($last_line) then
            $last_line = ""
        end
        var left_part = str_part($first_line, 0, $sel_sx)
        var right_part = str_part($last_line, $sel_ex, string.len($last_line))
        set_line($sel_sy, $left_part + $right_part)
        var k = $sel_ey
        while $k > $sel_sy then
            List.remove($lines, $k)
            $k = $k - 1
        end
        $sel_on = false
        goto_pos($sel_sy, $sel_sx)
    end
    return 0
end

def sel_copy then
    sel_normalize()
    if $sel_on then
        $clip = List.new()
        var y = $sel_sy
        while $y eql $sel_ey then
            var l = List.get($lines, $y)
            if errors.bool($l) then
                $l = ""
            end
            var a = 0
            var b = string.len($l)
            if $y equ $sel_sy then
                $a = $sel_sx
            end
            if $y equ $sel_ey then
                $b = $sel_ex
            end
            List.push($clip, str_part($l, $a, $b))
            $y = $y + 1
        end
        $status_message = "Copied " + types.int_to_str(List.len($clip)) + " line(s) to editor buffer."
    else
        $status_message = "Nothing selected."
    end
    return 0
end

def clip_paste then
    var n = List.len($clip)
    if $n equ 0 then
        $status_message = "Editor buffer is empty."
        return 0
    end
    if $sel_on then
        sel_delete()
    end
    var cy = $rendering_start_line + $cursor_y
    var cur = cur_line()
    var left_part = str_part($cur, 0, $actual_x)
    var right_part = str_part($cur, $actual_x, string.len($cur))
    var first_piece = List.get($clip, 0)
    if $n equ 1 then
        set_line($cy, $left_part + $first_piece + $right_part)
        goto_pos($cy, $actual_x + string.len($first_piece))
    else
        set_line($cy, $left_part + $first_piece)
        var k = 1
        while $k < $n then
            var piece = List.get($clip, $k)
            if $k equ $n - 1 then
                insert_line_at($cy + $k, $piece + $right_part)
            else
                insert_line_at($cy + $k, $piece)
            end
            $k = $k + 1
        end
        goto_pos($cy + $n - 1, string.len(List.get($clip, $n - 1)))
    end
    $need_full = true
    $status_message = "Pasted."
    return 0
end

def find_in_line -> line, query, start_x then
    var ll = string.len($line)
    var ql = string.len($query)
    if $ql equ 0 or $ql > $ll then
        return 0 - 1
    end
    var i = $start_x
    while $i + $ql eql $ll then
        var j = 0
        var same = true
        while $j < $ql and $same then
            if types.char_to_int(string.char_at($line, $i + $j)) neq types.char_to_int(string.char_at($query, $j)) then
                $same = false
            end
            $j = $j + 1
        end
        if $same then
            return $i
        end
        $i = $i + 1
    end
    return 0 - 1
end

def search_next then
    var total = List.len($lines)
    var y = $rendering_start_line + $cursor_y
    var start_x = $actual_x + 1
    if $sel_on then
        $start_x = $actual_x
    end
    var n = 0
    while $n eql $total then
        var l = List.get($lines, $y)
        if not(errors.bool($l)) then
            var hit = find_in_line($l, $search_query, $start_x)
            if $hit eqg 0 then
                goto_pos($y, $hit + string.len($search_query))
                $sel_ay = $y
                $sel_ax = $hit
                $sel_on = true
                $status_message = "Found: " + $search_query
                return 0
            end
        end
        $y = $y + 1
        if $y eqg $total then
            $y = 0
        end
        $start_x = 0
        $n = $n + 1
    end
    $status_message = "Not found: " + $search_query
    return 0
end

def search_open then
    $search_mode = true
    $search_buf = ""
    return 0
end

def search_cancel then
    $search_mode = false
    $status_message = "Search cancelled."
    return 0
end

def search_commit then
    $search_mode = false
    if string.len($search_buf) > 0 then
        $search_query = $search_buf
    end
    if string.len($search_query) equ 0 then
        $status_message = "Search cancelled."
    else
        search_next()
    end
    return 0
end

def search_backspace then
    var n = string.len($search_buf)
    if $n > 0 then
        $search_buf = str_part($search_buf, 0, $n - 1)
    end
    return 0
end

def search_key -> ckey, mod then
    if types.is_char($ckey) then
        var code = types.char_to_int($ckey)
        if $code equ 13 or $code equ 10 then
            search_commit()
        elif $code equ 27 then
            search_cancel()
        elif $code equ 8 or $code equ 127 then
            search_backspace()
        elif $code equ 102 and $mod equ $MOD_CTRL then
            search_cancel()
        elif $code eqg 32 and $mod neq $MOD_CTRL then
            $search_buf = $search_buf + types.char_to_str($ckey)
        end
    else
        if $ckey equ $KEY_ENTER or $ckey equ $KEY_F3 then
            search_commit()
        elif $ckey equ $KEY_BACKSPACE then
            search_backspace()
        elif $ckey equ $KEY_ESCAPE then
            search_cancel()
        end
    end
    return 0
end

def change_window_reset then 
	console.reset_scroll_area()
	console.leave_alt_screen()
	console.raw_mode(false)
	console.autowrap(true)
end

def change_window_goback then 
	console.enter_alt_screen()
	console.raw_mode(true)
	console.autowrap(false)
	console.scroll_area(0, $screen_height - 1)
	$smellslikeyouchangedsomethingspirit = 2
	render_screen()
end

def editor_help then 
	change_window_reset()
	io.print("\n", 
	"CTRL + S -> Save \n",
	"CTRL + Q -> Quit \n",
	"CTRL + F -> Search (Enter: find next, Esc: cancel) \n",
	"F3 -> Find next \n",
	"SHIFT + ARROWS / HOME / END / PAGE -> Select \n",
	"CTRL + C -> Copy selection (editor buffer) \n",
	"CTRL + X -> Cut selection \n",
	"CTRL + V or F4 -> Paste \n",
	"BACKSPACE -> Delete selection \n",
	"F6 -> Run Code \n",
	"PRESS ENTER TO BACK TO THE EDITOR.\n")
	io.flush()
	io.input()
	change_window_goback()
end

def run_code -> code_path then
	change_window_reset()
	io.print("\nPROGRAM START\n")
	io.flush()
	sys.system($sl_runtime + " " + $code_path)
	io.print("\nPROGRAM FINISHED\nPRESS ENTER TO BACK TO THE EDITOR.\n")
	io.getchar()
	change_window_goback()
end

render_screen()

while true then
    $actual_y = $rendering_start_line + $cursor_y

    $buffer = List.get($lines, $actual_y)

    if errors.bool($buffer) then
        $buffer = ""
    end

    var len = string.len($buffer)
    var event = console.get_event()

    if $event equ $KEY_EVENT then
        if console.key_event.is_pressed() then
            $status_message = "nomsg"

            var ckey = console.key_event.get_key()
            var mod = console.key_event.get_mod()

            if types.is_int($ckey) then
                if $search_mode equ false then
                    if $ckey equ $KEY_LEFT or $ckey equ $KEY_RIGHT or $ckey equ $KEY_UP or $ckey equ $KEY_DOWN or $ckey equ $KEY_HOME or $ckey equ $KEY_END or $ckey equ $KEY_PAGE_UP or $ckey equ $KEY_PAGE_DOWN then
                        if $mod equ $MOD_SHIFT then
                            if $sel_on equ false then
                                $sel_on = true
                                $sel_ay = $actual_y
                                $sel_ax = $actual_x
                            end
                        else
                            $sel_on = false
                        end
                    end
                end
            end

            if $search_mode then
                search_key($ckey, $mod)
            elif types.is_char($ckey) then
                if $ckey equ 'q' and $mod equ $MOD_CTRL then
                    console.reset_scroll_area()
                    console.autowrap(true)
                    console.leave_alt_screen()
                    console.raw_mode(false)
                    break
                elif $ckey equ 'f' and $mod equ $MOD_CTRL then
                    search_open()
                elif $ckey equ 'c' and $mod equ $MOD_CTRL then
                    sel_copy()
                elif $ckey equ 'x' and $mod equ $MOD_CTRL then
                    sel_copy()
                    if $sel_on then
                        sel_delete()
                    end
                elif $ckey equ 'v' and $mod equ $MOD_CTRL then
                    clip_paste()
                elif $ckey equ 's' and $mod equ $MOD_CTRL then
                    var all_buf = ""

                    while List.iter($lines) then
                        var save_line = List.next($lines)

                        if not(errors.bool($save_line)) then
                            $all_buf = $all_buf + $save_line + "\n"
                        end
                    end

                    file.write_from_str($file_name, $all_buf)
                    $status_message = "File saved to: " + $file_name

                else
                    if $ckey neq '\0' then
                        if $sel_on then
                            sel_delete()
                            $len = string.len($buffer)
                        end
                        if $len > $actual_x then
                            ascii_entered_middle($ckey, $len)
                        else
                            $buffer = $buffer + types.char_to_str($ckey)
                            ascii_entered()
                        end
                    end
                end
            elif types.is_int($ckey) then
                if $ckey equ $KEY_BACKSPACE then
                    if $sel_on then
                        sel_delete()
                    else
                        backspace_b()
                    end
     
				elif $ckey equ $KEY_TAB then
					if $sel_on then
						sel_delete()
						$len = string.len($buffer)
					end
					var spaces = "\t"
					var left = ""
					var right = ""
						
					if $actual_x > 0 then
						$left = string.slice($buffer, 0, $actual_x)
						if errors.bool($left) then
							$left = ""
						end
					end
					
					if $actual_x < $len then
						$right = string.slice($buffer, $actual_x, $len)
						if errors.bool($right) then
							$right = ""
						end
					end
					
					var actual_tab_y = $rendering_start_line + $cursor_y
					$buffer = $left + $spaces + $right
					
					$actual_x = $actual_x + 1
					update_cursor_x()
					
					if errors.bool(List.set($lines, $actual_tab_y, $buffer)) then
						List.push($lines, $buffer)
					end
					$smellslikeyouchangedsomethingspirit = 1
				elif $ckey equ $KEY_F4 then
					clip_paste()
				elif $ckey equ $KEY_F3 then
					if string.len($search_query) equ 0 then
						search_open()
					else
						search_next()
					end
				elif $ckey equ $KEY_F2 then
					editor_help()
				elif $ckey equ $KEY_F6 then
					# Running SL CODE!
					run_code($file_name)	
                elif $ckey equ $KEY_ENTER then
                    if $sel_on then
                        sel_delete()
                        $len = string.len($buffer)
                    end
                    var middle = false

                    if $len > $actual_x then
                        $middle = true
                    end

                    new_line($middle, $len)
                    $smellslikeyouchangedsomethingspirit = 2
                elif $ckey equ $KEY_HOME then
                    $actual_x = 0
                    update_cursor_x()

                elif $ckey equ $KEY_END then
                    var current_line = List.get($lines, $actual_y)

                    if not(errors.bool($current_line)) then
                        var len_str = string.len($current_line)

                        if not(errors.bool($len_str)) then
                            $actual_x = $len_str
                            $buffer = $current_line
                            update_cursor_x()
                        end
                    end

                elif $ckey equ $KEY_LEFT then
                    if $actual_x > 0 then
                        $actual_x = $actual_x - 1
                        update_cursor_x()
                    else
                        $status_message = "You are already in the beginning of the line."
                    end

                elif $ckey equ $KEY_PAGE_DOWN then
                    page_down()

                    var len_pd = List.len($lines)
                    if $cursor_y < $total_renderable then
                        $cursor_y = $total_renderable
                    end

                    if $rendering_start_line + $cursor_y eqg $len_pd then
                        $cursor_y = $len_pd - $rendering_start_line - 1
                        if $cursor_y < 0 then
                            $cursor_y = 0
                        end
                    end
                    
                    var pd_actual_y = $rendering_start_line + $cursor_y
                    var pd_line = List.get($lines, $pd_actual_y)
                    if not(errors.bool($pd_line)) then
                        var pd_len = string.len($pd_line)
                        if not(errors.bool($pd_len)) then
                            if $pd_len < $actual_x then
                                $actual_x = $pd_len
                            end
                        else
                            $actual_x = 0
                        end
                        $buffer = $pd_line
                        update_cursor_x()
                    else
                        $actual_x = 0
                        $buffer = ""
                        update_cursor_x()
                    end

                elif $ckey equ $KEY_PAGE_UP then
                    page_up()

                    if $cursor_y > 0 then
                        $cursor_y = 0
                    end 
                    
                    var pu_actual_y = $rendering_start_line + $cursor_y
                    var pu_line = List.get($lines, $pu_actual_y)
                    if not(errors.bool($pu_line)) then
                        var pu_len = string.len($pu_line)
                        if not(errors.bool($pu_len)) then
                            if $pu_len < $actual_x then
                                $actual_x = $pu_len
                            end
                        else
                            $actual_x = 0
                        end
                        $buffer = $pu_line
                        update_cursor_x()
                    else
                        $actual_x = 0
                        $buffer = ""
                        update_cursor_x()
                    end               
                elif $ckey equ $KEY_RIGHT then
                    var current_line = List.get($lines, $actual_y)

                    if not(errors.bool($current_line)) then
                        var len = string.len($current_line)

                        if not(errors.bool($len)) then
                            if $len > $actual_x then
                                $actual_x = $actual_x + 1
                                $buffer = $current_line
                                update_cursor_x()
                            else
                                $status_message = "You are already in the end of the line."
                            end
                        end
                    end
                elif $ckey equ $KEY_DOWN then
                    var len_dn = List.len($lines)

                    if $cursor_y + 1 < $screen_height then
                        if $rendering_start_line + $cursor_y + 1 < $len_dn then
                            $cursor_y = $cursor_y + 1

                            var next_actual_y = $rendering_start_line + $cursor_y
                            var next_line = List.get($lines, $next_actual_y)

                            if not(errors.bool($next_line)) then
                                var next_len = string.len($next_line)
                                if not(errors.bool($next_len)) then
                                    if $next_len < $actual_x then
                                        $actual_x = $next_len
                                    end
                                else
                                    $actual_x = 0
                                end
                                $buffer = $next_line
                                update_cursor_x()
                            else
                                $actual_x = 0
                                $buffer = ""
                                update_cursor_x()
                            end
                        else
                            $status_message = "Theres no extra line, press enter for new-line"
                        end
                    else
                        if $rendering_start_line + $screen_height < $len_dn then
                            $rendering_start_line = $rendering_start_line + 1
                            $rendering_end_line = $rendering_start_line + $total_renderable
                            
                            console.cursor_position(0, $total_renderable - 1)
                            console.line_down()
                            
                            var temp_y = $cursor_y
                            $cursor_y = $total_renderable - 1
                            line_renderer(1) 
                            $cursor_y = $temp_y
                            
                            $smellslikeyouchangedsomethingspirit = 0

                            var next_actual_y_scrolled = $rendering_start_line + $cursor_y
                            var next_line_scrolled = List.get($lines, $next_actual_y_scrolled)
                            if not(errors.bool($next_line_scrolled)) then
                                var next_len_scrolled = string.len($next_line_scrolled)
                                if not(errors.bool($next_len_scrolled)) then
                                    if $next_len_scrolled < $actual_x then
                                        $actual_x = $next_len_scrolled
                                    end
                                else
                                    $actual_x = 0
                                end
                                $buffer = $next_line_scrolled
                                update_cursor_x()
                            else
                                $actual_x = 0
                                $buffer = ""
                                update_cursor_x()
                            end
                        else
                            $status_message = "Theres no extra line, press enter for new-line"
                        end
                    end
                elif $ckey equ $KEY_UP then
                    if $cursor_y > 0 then
                        $cursor_y = $cursor_y - 1
                    else
                        if $rendering_start_line > 0 then
                            $rendering_start_line = $rendering_start_line - 1
                            $rendering_end_line = $rendering_start_line + $total_renderable
                            console.cursor_position(0, 0)
                            console.line_up()                            
                            var temp_y = $cursor_y
                            $cursor_y = 0
                            line_renderer(1)
                            $cursor_y = $temp_y
                            
                            $smellslikeyouchangedsomethingspirit = 0                       
                        else
                            $status_message = "You are already in first line."
                        end
                    end

                    var prev_actual_y = $rendering_start_line + $cursor_y
                    var prev_line = List.get($lines, $prev_actual_y)

                    if not(errors.bool($prev_line)) then
                        var prev_len = string.len($prev_line)

                        if not(errors.bool($prev_len)) then
                            if $prev_len < $actual_x then
                                $actual_x = $prev_len
                            end
                        else
                            $actual_x = 0
                        end
                        $buffer = $prev_line
                        update_cursor_x()
                    else
                        $actual_x = 0
                        $buffer = ""
                        update_cursor_x()
                    end
                end
            end
        end

        render_screen()
    elif $event equ $WINDOW_RESIZE then
        update_screen_size()
        render_screen()
    end
end

