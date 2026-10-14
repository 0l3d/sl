# Use ex_doc.sl to see documentation styled.
##Advanced Pipe Example 
##Define a variable like this: 
#?var mystr = """\n
#?Hello, World\n
#?Hello, Universe\n
#?Hello, EVERYONE!\n
#?"""
##Then use a pipe like this:
#?$mystr 
#?|> string.split("\n") 
#?|> List.get(1) 
#?|> string.slice(1, 6)
#?|> io.print("\n") 
#%Note: Pipes are not part of the base language; they are an extension of sl_lang.
##This expression looks like this without pipes:
#?io.print(string.slice(List.get(string.split($mystr, "\n"), 1), 1, 6), "\n")
use("io", "string", "types", "list")

var mystr = """\n
Hello, World\n
Hello, Universe\n
Hello, EVERYONE!\n
"""

$mystr 
|> string.split("\n") 
|> List.get(1) 
|> string.slice(1, 6)
|> io.print("\n")

#?
##ExDOC Supported Script
