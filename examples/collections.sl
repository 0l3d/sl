# Class-like system but more simpler
# Called: Collections

use("io","collections")

def App_change_width -> width, height then
    Collections.set_attr($self, "title", "Changed Title")
    Collections.set_attr($self, "width", $width)
    Collections.set_attr($self, "height", $height)
end

def App_get_title then
    return Collections.get_attr($self, "title")
end

def App_get_size -> width, height then
    return 
    Collections.get_attr($self, "width") 
    +
    Collections.get_attr($self, "height")
end

Collections.create_collection("App", 
                "v:title", "v:width", "v:height", 
                "f:App_change_width:change_width", 
                "f:App_get_title:get_title", 
                "f:App_get_size:get_size")

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


