# Class-like system but more simpler
# Called: Collections
import App.sl

use("io")

var application = App:new()
$application.title = "Calculator"
$application.width = 512 
$application.height = 512

io.print(
        "App Title: ", application:get_title(), "\n",
        "App Size: ", application:get_size(), "\n"
        )

application:change_width(200, 200)


io.print("-------- CHANGED --------------\n")
io.print(
        "App Title: ", application:get_title(), "\n",
        "App Size: ", application:get_size(), "\n"
        )


