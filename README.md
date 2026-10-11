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
