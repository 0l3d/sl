use("dyn", "types", "collections", "sys", "io")

# RAYLIB
if $SYSTEM_LINUX then
	var lib = dyn.open_lib("./libraylib.so.6.0.0")
	var InitWindow = dyn.find_symbol($lib, "InitWindow")
	var SetTargetFPS = dyn.find_symbol($lib, "SetTargetFPS")
	var WindowShouldClose = dyn.find_symbol($lib, "WindowShouldClose")
	var BeginDrawing = dyn.find_symbol($lib, "BeginDrawing")
	var ClearBackground = dyn.find_symbol($lib, "ClearBackground")
	var DrawText = dyn.find_symbol($lib, "DrawText")
	var EndDrawing = dyn.find_symbol($lib, "EndDrawing")
	var CloseWindow = dyn.find_symbol($lib, "CloseWindow")
else 
	io.print("Not supported on this platform!")
	io.flush()
	sys.exit(1)
end
# RAYLIB

def init_window -> width, height, title then 
	dyn.call($InitWindow, 4096, $DYN_NORETURN, $width, $height, $title)
end

def close_window then 
	dyn.call($CloseWindow, 4096, $DYN_NORETURN)
end

def target_fps -> fps then 
	dyn.call($SetTargetFPS, 4096, $DYN_NORETURN, $fps)
end

def check_win then 
	if dyn.call($WindowShouldClose, 4096, $DYN_INTEGER) equ 0 then 
		return true
	else
		return false
	end
end

def begin_drawing then 
	dyn.call($BeginDrawing, 4096, $DYN_NORETURN)
end

def end_drawing then 
	dyn.call($EndDrawing, 4096, $DYN_NORETURN)
end

def clear_background -> color then 
	dyn.call($ClearBackground, 4096, $DYN_NORETURN, $color)
end

def add_text -> rtext then 
	var text = Collections.get_attr($rtext, "text")
	var x = Collections.get_attr($rtext, "x")
	var y = Collections.get_attr($rtext, "y")
	var size = Collections.get_attr($rtext, "size")
	var color = Collections.get_attr($rtext, "color")
	dyn.call($DrawText, 4096, $DYN_NORETURN, $text, $x, $y, $size, $color)
end


Collections.create_collection("RayLIB",
			"v:title",
			"v:width",
			"v:height",
			"f:init_window:Init",
			"f:close_window:Close",
			"f:target_fps:TargetFPS",
			"f:check_win:Check",
			"f:begin_drawing:BeginDraw",
			"f:end_drawing:EndDraw",
			"f:add_text:AddRText",
			"f:clear_background:ClearBackground"
)

Collections.create_collection("RText", "v:text", "v:x", "v:y", "v:size", "v:color")


var COLOR_WHITE = dyn.create_struct($DYN_SIZEOF_UCHAR * 4, 4, $DYN_STRUCT_VAL, 
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 0, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 1, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 2, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 3, 1)
dyn.set_field($COLOR_WHITE, $DYN_SIZEOF_UCHAR * 0, types.int_to_char(245))
dyn.set_field($COLOR_WHITE, $DYN_SIZEOF_UCHAR * 1, types.int_to_char(245))
dyn.set_field($COLOR_WHITE, $DYN_SIZEOF_UCHAR * 2, types.int_to_char(245))
dyn.set_field($COLOR_WHITE, $DYN_SIZEOF_UCHAR * 3, types.int_to_char(255))

var COLOR_BLACK = dyn.create_struct($DYN_SIZEOF_UCHAR * 4, 4, $DYN_STRUCT_VAL, 
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 0, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 1, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 2, 1,
		$DYN_TYPE_UCHAR, $DYN_SIZEOF_UCHAR * 3, 1)
dyn.set_field($COLOR_BLACK, $DYN_SIZEOF_UCHAR * 0, types.int_to_char(0))
dyn.set_field($COLOR_BLACK, $DYN_SIZEOF_UCHAR * 1, types.int_to_char(0))
dyn.set_field($COLOR_BLACK, $DYN_SIZEOF_UCHAR * 2, types.int_to_char(0))
dyn.set_field($COLOR_BLACK, $DYN_SIZEOF_UCHAR * 3, types.int_to_char(255))

