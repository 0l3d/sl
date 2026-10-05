# This example is for Windows
use("dyn", "io", "errors", "string")

# KERNEL LIB 
var user32 = dyn.open_lib("KernelBase.dll")
# KERNEL LIB

var GetStdHandle = dyn.find_symbol($user32, "GetStdHandle")

var handle = dyn.call($GetStdHandle, 4096, $DYN_POINTER, 
	-11 # For value info: https://learn.microsoft.com/en-us/windows/console/getstdhandle
)

var WriteConsoleA = dyn.find_symbol($user32, "WriteConsoleA")
var message = "Hello, World!"

# Bytes written needs to be a reference.
# To be compatible with C, we need to create a structured variable
# that acts like a regular variable because its size is the same.
# This means we can create C-compatible variables like this.
var charsWritten = dyn.create_struct($DYN_SIZEOF_UINT, 
					1, 
					$DYN_STRUCT_REF, 
					$DYN_TYPE_UINT, $DYN_SIZEOF_UINT * 0,  1
)
# C COMPATIBLE VARIABLE END

# MICROSOFT DOCUMENTATION: 
# BOOL WINAPI WriteConsole(
#   _In_             HANDLE  hConsoleOutput,
#   _In_       const VOID    *lpBuffer,
#   _In_             DWORD   nNumberOfCharsToWrite,
#   _Out_opt_        LPDWORD lpNumberOfCharsWritten,
#   _Reserved_       LPVOID  lpReserved
# );
# We are using the ASCII version, so we use WriteConsoleA.
# If we used the wide-character version, we would use WriteConsoleW.
# Also BOOL is a int. (For more info: https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types)
# WriteConsoleA call 
dyn.call($WriteConsoleA, 4096, $DYN_INTEGER, 
						 $handle,
						 $message,
						 string.len($message),
						 $charsWritten, dyn.ptr(0))
# End of WriteConsoleA call

var chars_written = dyn.get_field($charsWritten, 0, $DYN_TYPE_UINT)
io.print("\nNumber of Chars written: ", $chars_written, "\n")
# For more type information:
# PROJECT_TREE\libs\dyncall\dyncall\dyncall_signature.h


dyn.free_struct($charsWritten)

dyn.free($user32)
