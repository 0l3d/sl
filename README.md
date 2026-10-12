# $L
  
SL is a simple scripting language for C.  
  
Language Reference: `PROJECT_TREE/docs/language_ref.txt`  
Standard Library Reference: `PROJECT_TREE/docs/standard_library_ref.txt`  
  
## Features
- `if` support.
- `while` loop support.
- `def` function support.
- `var` dynamic variables.
- All math expressions except assignments. Check wiki if you wanna know why.

## Usage 

```bash
git clone https://github.com/0l3d/sl
cd sl/
make

./sl # for usage string.
```
  
You can edit SL files with basic syntax highlighting without using an external editor:  
```
./sl examples/editor.sl examples/brainfuck.sl # or your path 
# Cross Platform editor for SL.
```
  
### Useful tools in the Examples folder
  
Even though the Examples folder is mostly for testing the language, a lot of the scripts are actually small tools that might come in handy.  
  
Here are a few of them:  
  
- `basic_calculator.sl`: a single-line plain text math evaluator without parenthesis support (for example: 100 / 5 + 1)  
- `brainfuck.sl`: a complete Brainfuck interpreter.  
- `ex_doc.sl`: a tool that generates documentation from specially formatted comments in .sl scripts. For example: ## HEADER  
- `letsconvert.sl`: converts a decimal number to hexadecimal, octal and binary. 
- `editor.sl`: a TUI editor built specifically for SL, with syntax highlighting for both SL and C.  
- `todo.sl`: a simple CLI task manager that uses SL's built-in database modules.  
- `guessing_game.sl`: a number guessing game where you try to guess a randomly generated number between 0 and 100.  
  
## Platform compatibility 
  
`sl.c` and `sl.h` (the main library sources) are written in Standard C.  
(No POSIX, WinAPI, or other OS specific APIs are used.)  
  
However, `stdlib.h` may change in the future, but I aim to implement it for as many operating systems as possible.  
  
The Console API and Network API are supported on both posix and windows.

### Dynamic Loading (standard library dyn api)
  
Huge thanks to everyone who worked on dyncall and made such a great library.  
  
SL's dynamic loading support follows the same platforms supported by dyncall. I chose dyncall because libffi was too heavy for this project and would have been harder to embed into the repository, while dyncall is minimal and has very little impact on the binary size. Thanks to dyncall, you can load raylib functions and make games, use OpenGL, call system libraries, and pretty much anything else you can think of. Keep in mind that dynamic loading is unsafe and requires you to deal with C's memory management. you'll need to use the free functions provided by the library you're working with.  
  
## API
  
Check Language REF for API.  
  
## License

This project is licensed under the BSD3-Clause License.

# Author 
Created by **0l3d**
