use("io", "string", "list")
var i = -1 while ({ $i = $i + 1 return $i < 1000}) then
   io.print("i: ", $i, "\n")
end
var name = "" 
io.print("Your names are \n", {$name = io.input("Enter your names: ")}) 
$name 
|> string.split(" ")
|> $name |> {
	while List.iter($name) then
		var item = List.next($name)
		io.print($item, ":","\n")
		if $item equ "0l3d" then  
   			io.print("M/D/C/G: 16G/512G/I5 12450HX/ARC A530M\n")
		elif $item equ "unknown" then 
			":D" |> io.print
		else
			io.print("No name? Get one.\n")
		end
	end
}


