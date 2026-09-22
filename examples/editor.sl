use("console", "io", "types", "sys", "string", "list", "errors", "file")

var file_name = sys.get_arg(2)
if errors.bool($file_name) then
    $file_name = "test.sl"
end

var ext = string.split($file_name, ".")
var sl_highlighting = List.new()
if not(errors.bool($ext)) then
    var exts = List.get($ext, List.len($ext) - 1)
    if $exts equ "sl" then
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
            "continue")
    elif $exts equ "c" then
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
            "continue")
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

        List.free($spltnewlines)
    end
end

console.enter_alt_screen()
console.clear()

var screen_width = console.get_width() - 1
var screen_height = console.get_height() - 1

var status_message = "nomsg"

var cursor_x = 0
var cursor_y = 0

var rendering_start_line = 0
var total_renderable = $screen_height - 1
var rendering_end_line = $rendering_start_line + $total_renderable

var actual_y = $rendering_start_line + $cursor_y

def render_indicator then
    console.cursor_position(0, $screen_height)
    console.clear_line()
    console.background_color($COLOR_WHITE)
    console.foreground_color($COLOR_BLACK)

    io.print(
        "CURSOR x:",
        $cursor_x,
        " y:",
        $actual_y,
        " msg:",
        $status_message,
        " File Name: ",
        $file_name
    )

    console.reset_color()
end

var smellslikeyouchangedsomethingspirit = true

def set_cursor_pos then
    console.cursor_position($cursor_x, $cursor_y)
end

var buffer = ""


def line_renderer then
    var i = $rendering_start_line
    var screen_y = 0
    while $screen_y < $total_renderable then
        console.cursor_position(0, $screen_y)
        console.clear_line()
        var line = List.get($lines, $i)
        if not(errors.bool($line)) then
            var highlighting = string.split($line, " ")
            errors.panic($highlighting)
            while List.iter($highlighting) then
                var item = List.next($highlighting)
                while List.iter($sl_highlighting) then
                    if $item equ List.next($sl_highlighting) then
                        console.foreground_color($COLOR_MAGENTA)
                    end
                end
                io.print($item + " ")
                console.reset_color()
            end
            List.free($highlighting)
        end
        $i = $i + 1
        $screen_y = $screen_y + 1
    end
end


def render_screen then
    console.begin_update()
    console.cursor_visibility(false)
    if $smellslikeyouchangedsomethingspirit then
        line_renderer()
        $smellslikeyouchangedsomethingspirit = false
    end
    render_indicator()

    if $cursor_x < $screen_width and $cursor_y < $screen_height then
        console.cursor_position($cursor_x, $cursor_y)
    else
        console.cursor_position(0, $screen_height - 1)
    end

    console.cursor_visibility(true)
    io.fflush()
    console.end_update()
end

set_cursor_pos()
console.raw_mode(true)

def new_line -> middler, length then
    var actual_y = $rendering_start_line + $cursor_y
    var left_part = ""
    var right_part = ""

    if $middler then
        if $cursor_x > 0 then
            $left_part = string.slice($buffer, 0, $cursor_x)
            if errors.bool($left_part) then
                $left_part = ""
            end
        end

        if $cursor_x < $length then
            $right_part = string.slice($buffer, $cursor_x, $length)
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
    else
        $cursor_y = $cursor_y + 1
    end

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

    $smellslikeyouchangedsomethingspirit = true
end

def backspace_b then
    var actual_y = $rendering_start_line + $cursor_y

    if $cursor_x > 0 then
        var remove_index = $cursor_x - 1
        var new_buffer = string.remove_at($buffer, $remove_index)

        if errors.bool($new_buffer) then
            $status_message = "Could not delete character."
        else
            $cursor_x = $cursor_x - 1
            $buffer = $new_buffer

            if errors.bool(List.set($lines, $actual_y, $buffer)) then
                List.push($lines, $buffer)
            end
        end
        $smellslikeyouchangedsomethingspirit = true
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
            else
                var merged_line = $prev_line + $current_line
                List.set($lines, $prev_y, $merged_line)
                List.remove($lines, $actual_y)
            end

            $cursor_x = $prev_len

            if $cursor_y > 0 then
                $cursor_y = $cursor_y - 1
            else
                if $rendering_start_line > 0 then
                    $rendering_start_line = $rendering_start_line - 1
                    $rendering_end_line = $rendering_start_line + $total_renderable
                end
            end

            $smellslikeyouchangedsomethingspirit = true
        else
            $status_message = "Theres no text to delete."
        end
    end
end

def ascii_entered then
    var actual_y = $rendering_start_line + $cursor_y

    $cursor_x = $cursor_x + 1

    if errors.bool(List.set($lines, $actual_y, $buffer)) then
        List.push($lines, $buffer)
    end
    $smellslikeyouchangedsomethingspirit = true
end

def page_down then
    var len = List.len($lines)

    if $rendering_start_line + $total_renderable < $len then
        $rendering_start_line = $rendering_start_line + 5
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
    $smellslikeyouchangedsomethingspirit = true
end

def page_up then
    if $rendering_start_line > 0 then
        $rendering_start_line = $rendering_start_line - 5
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
    $smellslikeyouchangedsomethingspirit = true
end

def ascii_entered_middle -> charkey, length then
    var left = ""
    var right = ""

    if $cursor_x > 0 then
        $left = string.slice($buffer, 0, $cursor_x)
        if errors.bool($left) then
            $left = ""
        end
    end

    if $cursor_x < $length then
        $right = string.slice($buffer, $cursor_x, $length)
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
    $cursor_x = $cursor_x + 1
    $smellslikeyouchangedsomethingspirit = true
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

            if types.is_char($ckey) then
                if $ckey equ 'q' and $mod equ $MOD_CTRL then
                    console.leave_alt_screen()
                    break
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
                        if $len > $cursor_x then
                            ascii_entered_middle($ckey, $len)
                        else
                            $buffer = $buffer + types.char_to_str($ckey)
                            ascii_entered()
                        end
                    end
                end

            elif types.is_int($ckey) then
                if $ckey equ $KEY_BACKSPACE then
                    backspace_b()
                elif $ckey equ $KEY_TAB then
                    var spaces = "    "
                    var left = ""
                    var right = ""

                    if $cursor_x > 0 then
                        $left = string.slice($buffer, 0, $cursor_x)
                        if errors.bool($left) then
                            $left = ""
                        end
                    end

                    if $cursor_x < $len then
                        $right = string.slice($buffer, $cursor_x, $len)
                        if errors.bool($right) then
                            $right = ""
                        end
                    end

                    var actual_tab_y = $rendering_start_line + $cursor_y
                    $buffer = $left + $spaces + $right
                    $cursor_x = $cursor_x + 4

                    if errors.bool(List.set($lines, $actual_tab_y, $buffer)) then
                        List.push($lines, $buffer)
                    end
                    $smellslikeyouchangedsomethingspirit = true

                elif $ckey equ $KEY_ENTER then
                    var middle = false

                    if $len > $cursor_x then
                        $middle = true
                    end

                    new_line($middle, $len)
                    $smellslikeyouchangedsomethingspirit = true
                elif $ckey equ $KEY_HOME then
                    $cursor_x = 0

                elif $ckey equ $KEY_END then
                    var current_line = List.get($lines, $actual_y)

                    if not(errors.bool($current_line)) then
                        var len_str = string.len($current_line)

                        if not(errors.bool($len_str)) then
                            $cursor_x = $len_str
                        end
                    end

                elif $ckey equ $KEY_LEFT then
                    if $cursor_x > 0 then
                        $cursor_x = $cursor_x - 1
                    else
                        $status_message = "You are already in the beginning of the line."
                    end

                elif $ckey equ $KEY_PAGE_DOWN then
                    page_down()

                    if $cursor_y < $screen_height then
                        $cursor_y = $total_renderable
                    end

                elif $ckey equ $KEY_PAGE_UP then
                    page_up()

                    if $cursor_y > 0 then
                        $cursor_y = 0
                    end

                elif $ckey equ $KEY_RIGHT then
                    var current_line = List.get($lines, $actual_y)

                    if not(errors.bool($current_line)) then
                        var len = string.len($current_line)

                        if not(errors.bool($len)) then
                            if $len > $cursor_x then
                                $cursor_x = $cursor_x + 1
                            else
                                $status_message = "You are already in the end of the line."
                            end
                        end
                    end
                elif $ckey equ $KEY_DOWN then
                    var len = List.len($lines)

                    if $cursor_y + 1 < $screen_height then
                        if $rendering_start_line + $cursor_y + 1 < $len then
                            $cursor_y = $cursor_y + 1

                            var next_actual_y = $rendering_start_line + $cursor_y
                            var next_line = List.get($lines, $next_actual_y)

                            if not(errors.bool($next_line)) then
                                var next_len = string.len($next_line)

                                if not(errors.bool($next_len)) then
                                    if $next_len < $cursor_x then
                                        $cursor_x = $next_len
                                    end
                                end
                            end
                        else
                            $status_message = "Theres no extra line, press enter for new-line"
                        end
                    else
                        if $rendering_start_line + $screen_height < $len then
                            $rendering_start_line = $rendering_start_line + 1
                            $rendering_end_line = $rendering_start_line + $total_renderable
                            $smellslikeyouchangedsomethingspirit = true
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
                            $smellslikeyouchangedsomethingspirit = true
                        else
                            $status_message = "You are already in first line."
                        end
                    end

                    var prev_actual_y = $rendering_start_line + $cursor_y
                    var prev_line = List.get($lines, $prev_actual_y)

                    if not(errors.bool($prev_line)) then
                        var prev_len = string.len($prev_line)

                        if not(errors.bool($prev_len)) then
                            if $prev_len < $cursor_x then
                                $cursor_x = $prev_len
                            end
                        end
                    end
                end
            end
        end

        set_cursor_pos()
        render_screen()
    elif $event equ $WINDOW_RESIZE then
        update_screen_size()
        render_screen()
    end
end
