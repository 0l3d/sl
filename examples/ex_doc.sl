# Document Extractor for .SL Files
# 
# You can test the document extraction by passing this file to itself:
# sl ex_doc.sl ex_doc.sl
# This will extract the documentation from this file.
#
# Normal Text:
#
# Document Usage: #? This is document text
# Extractor Output: | This is document text
#
# Headers:
#
# Document Usage: ## Something
# Extractor Output: (Bold Text) Something
#
# Blinking Text:
#
# Document Usage: #% DISCLAIMER: Be careful when using this function.
# Extractor Output: (Blinking Background and Bold Text) ! DISCLAIMER: Be careful when using this function.
# 
# Example Usage
##EX Doc 
#?Document Extractor for SL.
#%Usage: sl ex_doc.sl file.sl

use("io", "string", "types", "errors", "list", "file", "sys", "console")

var file_name = sys.get_arg(2)
var file_str = file.read_to_str($file_name)
errors.panic($file_str)

var lines = string.split($file_str, "\n")

while List.iter($lines) then 
	var line = List.next($lines)
	if string.contains($line, '#') then 
		var index = string.index_of($line, '#')
		var char = string.char_at($line, $index + 1)
		if $char equ '?' then 
			console.reset_style($STYLE_BOLD)
			var str_after_q = string.slice($line, $index + 2, string.len($line))
			io.print("| ", $str_after_q, "\n")
		elif $char equ '#' then 
			console.reset_style($STYLE_BLINK)
			console.set_style($STYLE_BOLD)
			var str_after_q = string.slice($line, $index + 2, string.len($line))
			io.print($str_after_q, "\n")
		elif $char equ '%' then 
			console.set_style($STYLE_BLINK)
			var str_after_q = string.slice($line, $index + 2, string.len($line))
			io.print("! ", $str_after_q, "\n")
		end 
	end				
end

console.reset_color()