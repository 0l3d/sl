use("dyn", "io") 

def simple_func -> id, score, name then 
	io.print("SCRIPT SIDE: ", $id, " Name: ", $name, " Score: ", $score, "\n")
	return $id * 100
end

var lib = dyn.open_lib("./test.so")

var runnable = dyn.find_symbol($lib, "run_native_loop")

var callable = dyn.create_callback("idZ)i", "simple_func")

dyn.call($runnable, 4096, $DYN_NORETURN, $callable)

dyn.free_callback($callable)

dyn.free($lib)

