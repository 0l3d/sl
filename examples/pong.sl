use("console", "io", "collections", "enums", "sys", "types", "time", "extra")

# Player Algorithm
Collections.create_collection("Player", "v:Y", "v:ch", "v:score")
# Player Algorithm

# Console Algorithm 

def Console_init then 
	Collections.set_attr($self, "height", console.get_height() - 1)
	Collections.set_attr($self, "width", console.get_width() - 1)
	Collections.set_attr($self, "cursor_x", 0)
	Collections.set_attr($self, "cursor_y", 0)
	console.enter_alt_screen()
	console.raw_mode(true)
	console.cursor_visibility(false)
end

def Console_close then 
	console.leave_alt_screen()
	console.raw_mode(false)
	console.cursor_visibility(true)
end

def Console_set_curr -> x, y then
	console.cursor_position($x, $y)
	Collections.set_attr($self, "cursor_x", $x)
	Collections.set_attr($self, "cursor_y", $y)
end

def Console_event_handler then 
	var event = console.get_event()
	Collections.set_attr($self, "console_key", 0)
	Collections.set_attr($self, "console_mod", 0)
	Collections.set_attr($self, "console_event", $NO_KEY;)
	if $event equ $KEY_EVENT then 
		var ckey = console.key_event.get_key()
		var mod = console.key_event.get_mod()
		if types.is_char($ckey) then
			if $ckey equ 'q' and $mod equ $MOD_CTRL then
				console.reset_scroll_area()
				console.autowrap(true)
				console.leave_alt_screen()
				console.raw_mode(false)
				sys.exit(1)
			else
				Collections.set_attr($self, "console_event", $CHAR;)
			end
		else
			Collections.set_attr($self, "console_event", $INT;)
		end
		Collections.set_attr($self, "console_key", $ckey)
		Collections.set_attr($self, "console_mod", $mod)
	elif $event equ $MOUSE_EVENT then 
		Collections.set_attr($self, "console_key", 0)
		Collections.set_attr($self, "console_mod", 0)
	end
end

def Console_print -> text then 
	var x = Collections.get_attr($self, "cursor_x")
	var y = Collections.get_attr($self, "cursor_y")
	console.write_at($x, $y, $text)
end

enums.create_enum("CHAR", "INT", "NO_KEY")

Collections.create_collection("Console", 
				"v:height", 
				"v:width", 
				"v:cursor_x",
				"v:cursor_y",
				"v:console_event",
				"v:console_key",
				"v:console_mod",
				"f:Console_init:init",
				"f:Console_close:close",
				"f:Console_event_handler:handle_events",
				"f:Console_set_curr:set_cursor",
				"f:Console_print:print")

# Console Algorithm


# Ball

enums.create_enum("LEFT", "RIGHT")
enums.create_enum("UP", "DOWN", "NODIR")

def Ball_ch_direction then
	var direct = Collections.get_attr($self, "direction")
	if $direct equ $LEFT; then 
		Collections.set_attr($self, "direction", $RIGHT;)
	else 
		Collections.set_attr($self, "direction", $LEFT;)
	end
	Collections.set_attr($self, "ydir", $NODIR;)
end

def Ball_ch_ydir then 
	var direct = Collections.get_attr($self, "ydir")
	if $direct equ $UP; then 
		Collections.set_attr($self, "ydir", $DOWN;)
	elif $direct equ $DOWN; then
		Collections.set_attr($self, "ydir", $UP;)
	else 
		var selection = $NODIR;
		var rand = rand.random(0, 1)
		if $rand equ 0 then 
			$selection = $UP;
		else
			$selection = $DOWN;
		end
		Collections.set_attr($self, "ydir", $selection)
	end
end

Collections.create_collection("Ball",
				"v:ch",
				"v:direction",
				"v:ydir",
				"v:x",
				"v:y",
				"f:Ball_ch_direction:ch_direction",
				"f:Ball_ch_ydir:ch_ydir")

# Ball

var game = Console:new()
game:init()
# game:set_cursor(0, $game.height / 2)

var player1 = Player:new() # wasd 	RIGHT
var player2 = Player:new() # arrow keys LEFT
var ball = Ball:new()
$ball.direction = $RIGHT;
$ball.ydir = $NODIR;
$ball.ch = "@"
$ball.x = $game.width / 3
$ball.y = $game.height / 2

def DrawBall then 
	# 			@
	# Ball Drawing Logic:  @@@
	# 			@

	game:set_cursor($ball.x, $ball.y)
	game:print("@@@")
	game:set_cursor($ball.x + 1, $ball.y - 1)
	game:print("@")
	game:set_cursor($ball.x + 1, $ball.y + 1)
	game:print("@")
end

def BallUpdate then 
	if $ball.x equ 0 then
		ball:ch_direction()
	elif $ball.x equ $game.width - 2 then 
		ball:ch_direction()
	end
	
	if $ball.direction equ $RIGHT; then 
		$ball.x = $ball.x - 1 
	elif $ball.direction equ $LEFT; then 
		$ball.x = $ball.x + 1
	end

	if $ball.y equ 0 then
		ball:ch_ydir()
	elif $ball.y equ $game.height then 
		ball:ch_ydir()
	end
	

	if $ball.ydir equ $UP; then 
		$ball.y = $ball.y - 1 
	elif $ball.ydir equ $DOWN; then 
		$ball.y = $ball.y + 1
	else
		ball:ch_ydir()
	end
end

var PLAYER_HEIGHT = 8
var PLAYER_WEIGHT = 2

$player1.ch = "#"
$player2.ch = "&"

# Players only need Y direction 

def DrawPlayers then 
	# PLAYER 1 
	var i = -1 while ({$i = $i + 1 return $i < $PLAYER_HEIGHT}) then 
		game:set_cursor(0, $player1.Y + $i)
		game:print("##")
	end
	# PLAYER 2
	var i = -1 while ({$i = $i + 1 return $i < $PLAYER_HEIGHT}) then 
		game:set_cursor($game.width - 1, $player2.Y + $i)
		game:print("&&")
	end
end

def CheckCollision then 
	if $ball.x equ 0 then 
		if $ball.y > $player1.Y + $PLAYER_HEIGHT then 
			$player2.score = $player2.score + 1
		elif $ball.y < $player1.Y then
			$player2.score = $player2.score + 1 
		end
	end

	if $ball.x equ $game.width - 2 then 
		if $ball.y > $player2.Y + $PLAYER_HEIGHT then
			$player1.score = $player1.score + 1
		elif $ball.y < $player2.Y then 
			$player1.score = $player1.score + 1
		end
	end
end

# DrawPlayers()

def ScoreTable then 
	game:set_cursor($game.width / 3 - 3, $game.height)
	io.print("CTRL+Q For Quit, Player 1 Score: ", $player1.score ," | Player 2 Score: ", $player2.score)
end

def DrawScreen then
	console.begin_update()
	console.clear()
	DrawPlayers()
	ScoreTable()
	DrawBall()
	console.end_update()
end

DrawScreen()

var FRAME_MS = 33
var frame_start = 0
var elapsed = 0

while true then 
	$frame_start = time.ticks()
	game:handle_events()
	if $game.console_event equ $CHAR; then 
		if $game.console_key equ 's' then 
			if $player1.Y + $PLAYER_HEIGHT < $game.height then 
				$player1.Y = $player1.Y + 1
			end
		elif $game.console_key equ 'w' then 
			if $player1.Y > 0 then
				$player1.Y = $player1.Y - 1
			end
		end		
	elif $game.console_event equ $INT; then 
		if $game.console_key equ $KEY_DOWN then 
			if $player2.Y + $PLAYER_HEIGHT < $game.height then 
				$player2.Y = $player2.Y + 1
			end
		elif $game.console_key equ $KEY_UP then 
			if $player2.Y > 0 then
				$player2.Y = $player2.Y - 1
			end
		end	
	end
	BallUpdate()
	DrawScreen()
	CheckCollision()
	$elapsed = time.ticks() - $frame_start
	if $elapsed < $FRAME_MS then
		time.sleep($FRAME_MS - $elapsed)
	end
end


game:close()





