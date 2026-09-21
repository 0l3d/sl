use("console", "io", "types", "sys", "string", "list", "errors", "file")

var file_name = sys.get_arg(2)

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

def set_cursor_pos then
    console.cursor_position($cursor_x, $cursor_y)
end

var sl_highlighting = List.new()

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
    "continue"
)

var buffer = ""


def line_renderer then
    var i = $rendering_start_line
    var screen_y = 0
    var total_lines = List.len($lines)
    var lines_len = string.len(types.int_to_str($total_lines))
    while $screen_y < $total_renderable then
        console.cursor_position(0, $screen_y)
        console.clear_line()

        var line = List.get($lines, $i)
        io.print($line)
        io.fflush()
        $i = $i + 1
        $screen_y = $screen_y + 1
    end
end


def render_screen then
    console.cursor_visibility(false)

    line_renderer()
    render_indicator()

    if $cursor_x < $screen_width and $cursor_y < $screen_height then
        console.cursor_position($cursor_x, $cursor_y)
    else
        console.cursor_position(0, $screen_height - 1)
    end

    console.cursor_visibility(true)
    io.fflush()
end

set_cursor_pos()
console.raw_mode(true)

def new_line -> middler, length then
    var actual_y = $rendering_start_line + $cursor_y

    if errors.bool(List.set($lines, $actual_y, $buffer)) then
        List.push($lines, $buffer)
    end

    if $cursor_y equ $screen_height then
        $rendering_start_line = $rendering_start_line + 1
    else
        $cursor_y = $cursor_y + 1
    end

    $cursor_x = 0
    $buffer = ""

    $rendering_end_line = $rendering_start_line + $total_renderable
end

def backspace_b then
    if $cursor_x > 0 then
        var actual_y = $rendering_start_line + $cursor_y
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
    else
        $status_message = "Theres no text to delete."
    end
end

def ascii_entered then
    var actual_y = $rendering_start_line + $cursor_y

    $cursor_x = $cursor_x + 1

    if errors.bool(List.set($lines, $actual_y, $buffer)) then
        List.push($lines, $buffer)
    end
end

def page_down then
    var len = List.len($lines)

    if $rendering_start_line + $total_renderable < $len then
        $rendering_start_line = $rendering_start_line + 1
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
end

def page_up then
    if $rendering_start_line > 0 then
        $rendering_start_line = $rendering_start_line - 1
    end

    $rendering_end_line = $rendering_start_line + $total_renderable
end

def ascii_entered_middle -> charkey, length then
    var right = string.slice($buffer, $cursor_x, $length)
    var left = string.slice($buffer, 0, $cursor_x)

    if errors.bool($right) or errors.bool($left) then
        $status_message = "Could not insert character."
    else
        var full_buffer = $left + types.char_to_str($charkey) + $right
        var actual_y = $rendering_start_line + $cursor_y

        $buffer = $full_buffer

        if errors.bool(List.set($lines, $actual_y, $buffer)) then
            List.push($lines, $buffer)
        end
    end
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
                        io.print($ckey)

                        if $len > $cursor_x then
                            ascii_entered_middle($ckey, $len)
                        else
                            $buffer = $buffer + types.char_to_str($ckey)
                        end

                        ascii_entered()
                    end
                end

            elif types.is_int($ckey) then
                if $ckey equ $KEY_BACKSPACE then
                    backspace_b()

                elif $ckey equ $KEY_TAB then
                    var spaces = "    "

                    var left = string.slice($buffer, 0, $cursor_x)
                    var right = string.slice($buffer, $cursor_x, $len)

                    if errors.bool($left) or errors.bool($right) then
                        $status_message = "Could not insert tab."
                    else
                        var actual_tab_y = $rendering_start_line + $cursor_y

                        $buffer = $left + $spaces + $right
                        $cursor_x = $cursor_x + 4

                        if errors.bool(List.set($lines, $actual_tab_y, $buffer)) then
                            List.push($lines, $buffer)
                        end
                    end

                elif $ckey equ $KEY_ENTER then
                    var middle = false

                    if $len > $cursor_x then
                        $middle = true
                    end

                    new_line($middle, $len)

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

        io.fflush()
        set_cursor_pos()
        render_screen()
    elif $event equ $WINDOW_RESIZE then 
        render_screen()
    end
end

