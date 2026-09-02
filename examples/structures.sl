use("io", "structures")

var mystruct = structures.create("name", "Address", "ID")
$mystruct.name = "Tom"
$mystruct.Address = "Berkeley, CA"
$mystruct.ID = 10

io.print(
        "Name: ", $mystruct.name, "\n",
        "Address: ", $mystruct.Address, "\n",
        "ID: ", $mystruct.ID, "\n"
        )


def hidden_structure then
        var other_struct = structures.create("name", "Address", "ID")
        $other_struct.name = "Maria"
        $other_struct.Address = "Greensburg, Indiana"
        $other_struct.ID = 11
        # WORKS:
        io.print("Hidden Struct Name: ", $other_struct.name, "\n")
end
hidden_structure()

# ERROR:
# io.print($other_struct.name) 
