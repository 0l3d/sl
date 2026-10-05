use("dyn")

var lib = dyn.open_lib("./test.so")


var function = dyn.find_symbol($lib, "print_struct")

var struct = dyn.create_struct(
	$DYN_SIZEOF_INT * 2, # How much? sizeof(int) * 2 = 8
	2,  # field count = 2 
	$DYN_STRUCT_VAL, # var or PTR: $DYN_STRUCT_PTR?
	$DYN_TYPE_INT, $DYN_SIZEOF_INT * 0, 1, # TYPE, OFFSET, LEN 
	$DYN_TYPE_INT, $DYN_SIZEOF_INT * 1, 1  # TYPE, OFFSET, LEN 
)
dyn.set_field($struct, $DYN_SIZEOF_INT * 0, 10)
dyn.set_field($struct, $DYN_SIZEOF_INT * 1, 20)

dyn.call($function, 4096, $DYN_NORETURN, $struct)

dyn.free_struct($struct)

dyn.free($lib)


# Explicit Typed:
# use("dyn")
#
# var lib = dyn.open_lib("./test.so")
#
#
# var function = dyn.find_symbol($lib, "print_struct")
#
# var struct = dyn.create_struct(
#         $DYN_SIZEOF_FLOAT * 2, # How much? sizeof(float) * 2 = 8
#         2,  # field count = 2
#         $DYN_STRUCT_VAL, # var or PTR: $DYN_STRUCT_PTR?
#         $DYN_TYPE_FLOAT, $DYN_SIZEOF_FLOAT * 0, 1, # TYPE, OFFSET, LEN
#         $DYN_TYPE_FLOAT, $DYN_SIZEOF_FLOAT * 1, 1  # TYPE, OFFSET, LEN
# )
# dyn.set_field($struct, $DYN_SIZEOF_FLOAT * 0, 'f' # Difference from regular usage.
#                                                 , 40.1) # We are explicitly casting the SL Variable to a C float.
# dyn.set_field($struct, $DYN_SIZEOF_FLOAT * 1, 'f'
#                                                 , 20.1)
#
# dyn.call($function, 4096, $DYN_NORETURN, $struct)
#
# dyn.free_struct($struct)
#
# dyn.free($lib)
