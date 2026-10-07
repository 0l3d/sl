# Example for LINUX and WINDOWS only.
# Im not freeing all structs on Close in this example but you need to free them.

import sl_binding.sl

var raylib = RayLIB:new()

var rtext = RText:new()
$rtext.text = "Hello, World!"
$rtext.x = 20
$rtext.y = 20 
$rtext.size = 20 
$rtext.color = $COLOR_WHITE 

raylib:Init(300, 400, "RayLIB Example")
while raylib:Check() then 
	raylib:BeginDraw()
	raylib:ClearBackground($COLOR_BLACK)
	raylib:AddRText($rtext)	
	raylib:EndDraw()
end

raylib:Close()


