# Example for qsort and *LINUX*
# Taken from: https://www.geeksforgeeks.org/c/qsort-function-in-c/
use("dyn", "io")


def qsort_algorithm -> a, b then 
	var x = dyn.get_ptr_field($a, 0, $DYN_TYPE_INT)
	var y = dyn.get_ptr_field($b, 0, $DYN_TYPE_INT)
	if $x < $y then 
		return -1
	elif $x > $y then 
		return 1
	end
	return 0
end

var libc = dyn.open_lib("libc.so.6")

# C-Compatible Array

var arr = dyn.create_struct(
			$DYN_SIZEOF_INT * 5,
			1,
			$DYN_STRUCT_REF,
			$DYN_TYPE_INT, 0, 5)
# setting
dyn.set_field($arr, $DYN_SIZEOF_INT * 0, 5) # first
dyn.set_field($arr, $DYN_SIZEOF_INT * 1, 2)
dyn.set_field($arr, $DYN_SIZEOF_INT * 2, 3)
dyn.set_field($arr, $DYN_SIZEOF_INT * 3, 1)
dyn.set_field($arr, $DYN_SIZEOF_INT * 4, 4)

var qsort = dyn.find_symbol($libc, "qsort")


var qsort_alg = dyn.create_callback("pp)i", "qsort_algorithm")

dyn.call($qsort, 4096, $DYN_INTEGER, $arr, 5, $DYN_SIZEOF_INT, $qsort_alg)

io.print("ITEMS SORTED:\n")

var size = 0
var off = 0
var i = 0
while $i < 5 then 
	var item = dyn.get_field($arr, $off, $DYN_TYPE_INT)
	io.print("ITEM: ", $item, "\n")
	$off = $off + $DYN_SIZEOF_INT
	$i = $i + 1
end

dyn.free_struct($arr)
dyn.free_callback($qsort_alg)
dyn.free($libc)
