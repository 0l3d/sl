use("console", "io", "types", "errors", "sys")

console.clear()

var end_x = console.get_width()-1
var starting_x_y = console.get_height()-1
var starting_y_x = console.get_width()-1
var end_y = console.get_height()-1

# WINDOW
var counter = 0
while $counter neq $end_x then 
	console.cursor_position($counter, 0)
	io.print("-")
	$counter = $counter + 1
end

$counter = 0
while $counter neq $end_x then 
	console.cursor_position($counter, $starting_x_y)
	io.print("-")
	$counter = $counter + 1
end

$counter = 0
while $counter neq $end_y then 
	console.cursor_position(0, $counter)
	io.print("|")
	$counter = $counter + 1
end

$counter = 0
while $counter neq $end_y then 
	console.cursor_position($end_x, $counter)
	io.print("|")
	$counter = $counter + 1
end


io.flush()

# WINDOW

console.raw_mode(true)

# HELLO WORLD AND QUIT MESSAGE 
var middle_x = $end_x / 3 + ($end_x / 14)
var middle_y = $end_y / 2 

console.cursor_position($middle_x, $middle_y)
io.print("Hello, World!")
console.cursor_position($middle_x, $middle_y + 1)
io.print("Type CTRL+Q for Quit.")
io.flush()
# HELLO WORLD AND QUIT MESSAGE


while true then 
	var event = console.get_event()
	if $event equ $KEY_EVENT then 
		if console.key_event.is_pressed() then 
			var ckey = console.key_event.get_key()
			var mod = console.key_event.get_mod()
			if types.is_char($ckey) then 
				if $ckey equ 'q' and $mod equ $MOD_CTRL then 
					sys.exit(1)
				end
			end
		end
	elif $event equ $MOUSE_EVENT then 
		io.print("Mouse event catched.")
		io.flush()
	end
end
