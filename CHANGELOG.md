# Changelog
  
## SL 2.0.0 - The Architect
  
So when we got to version 1.3.2, I sat down and wrote a mathematical example with a donut, and holy shit, it was so slow that we were talking about something like 0.5 FPS.
  
The language was normally pretty fast, it could open raylib, connect to SSL, and do all of that without being THAT much slower than C, but when it came to mathematical operations that had to be calculated constantly, or while loops that just kept running forever, it was losing so much time that the whole thing was basically choking on itself. And the funny part is that the language didnt even have that many features yet, we couldnt even directly index strings, we had to use a function for that. Not because it was impossible to implement or anything, it was just designed that way because the language was supposed to stay simple, which is a perfectly fine design choice until you start staring at 0.5 FPS and questioning your entire life.
  
Anyway, today marks the 4th day since my last commit to SL, and during these 4 days I have been writing a virtual machine for this language. Yes, an actual VM, because apparently having a scripting language that reads everything in a straight line and parses things as it goes wasnt enough for me anymore, so I decided to make my life significantly worse for a while.
  
And here we are, SL 2.0.0.
  
I started adding optimizations, started using enums for tokens and doing things properly, and somehow we ended up here. The language now runs on a VM, and after 4 days of fighting with instructions, execution logic, indexing, compatibility, and whatever else decided to break at 3 AM, it is finally done.
  
The VM uses 32 bit indexing, the instructions are 32 bit, everything is 32 bit because why the hell not. Of course this means we can theoretically have more than 4 billion indexes, and I am planning to handle the bigger values using opcodes called BIG, because apparently 4 billion wasnt enough and we need to prepare for problems that I will probably never live long enough to encounter.
  
While writing the VM I honestly started losing awareness of what I was doing at some point. You know that feeling where you look at your code and you can still read it, but your brain has already left the building and is watching you from outside? Yeah, that was basically the experience. Thankfully, it is finally over.
  
I also wrote the documentation, updated the standard library functions to work with the VM, added a bunch of VM related features, and tried to make the whole thing as flexible as possible. So besides the VM itself, here is what else I added for people who want to hack SL, build their own dialects, mess around with the compiler, or just do things that I probably didnt originally expect anyone to do.
  
First of all, THE VIRTUAL MACHINE HAS BEEN ADDED.
  
I know, shocking. A virtual machine in a language that used to just read everything directly and parse it as it went. What a development.
  
Some compatibility opcodes have also been added, including things like dynamically added linked functions and other stuff with names that probably sound much more complicated than they actually are.
  
So why are these even necessary?
  
When code is compiled, the functions that exist inside the source code are known in advance and can be stored directly in the compiled program. This means their indexes are known, the compiler can put them into the function pool, and everything works as expected.
  
But when a linked function is added later, it obviously wasnt inside that original pool when the code was compiled. This means a CALL instruction could end up pointing at the wrong index, or the VM could just tell you that the function doesnt exist in the first place.
  
And that would be kind of annoying, considering the whole point of linked functions is to let you do things dynamically.
  
So I added a dedicated instruction for this situation. If something goes wrong during VM execution, it can report the error at runtime.
  
Now, when running regular SL source code, the code is still compiled before execution, but the runtime can keep track of the original source code and show you where the error happened. So you can actually see which part of your code decided to ruin your day.
  
However, if you compile your code into bytecode, save it, and run that bytecode later, the original source code might no longer be available. In that case, the VM can still report the error as plain text, but it cant point to the exact location in the original source code.
  
Basically, the VM knows something went wrong, it just doesnt necessarily know which line of your source code decided to ruin your day. Again.
  
New compiler toolchain functions have been added.
  
These are an absolute goldmine for hackability. I tried to expose them as flexibly as I could, because I really want people to be able to mess with the language instead of being forced to use it exactly the way I personally use it.
  
If you want to build your own tools, experiment with compilation, inspect instructions, or create your own weird little SL based project, there is now a lot more room to do that. I dont want SL to be just a language where you write code and hope that everything works, I want people to be able to actually get inside the thing and do whatever they want with it.
  
Within reasonable limits, of course. I am still the person who has to debug whatever happens next.
  
New flags have been added to sl_lang.
  
And by sl_lang I mean my own implementation of SL, just in case that wasnt obvious.
  
There are now flags for compiling, disassembling, and running code, among other things. You can check the usage by simply running ./sl without any arguments, or take a look at the language reference if you prefer reading documentation instead of discovering things by accidentally breaking them.
  
Both options are valid, I guess.
  
You can also use the -c or /c flag to compile SL source files into the SLX format. And before anyone asks, no, the file doesnt actually have to end with .slx. You can use whatever extension you want, the extension is not what makes the file bytecode. SLX is simply the name I gave to the format itself, not some sacred file extension that must be followed or the VM refuses to acknowledge your existence.
  
The language is now significantly faster in while loops.
  
And I mean it, this thing actually became insane.
  
The kind of code that used to spend most of its time sitting inside loops doing mathematical operations can now run through the VM instead of constantly paying the cost of the old execution approach. This is the part I am probably the happiest about, because the whole reason I started looking into this was that damn donut example.
  
I wanted to make the language faster, and somehow I ended up spending four days writing an entire virtual machine, updating the standard library, writing documentation, adding compiler tools, and questioning the decisions that led me to this point.
  
So yeah, mission accomplished?
  
There are probably a few things I forgot to mention here, because after four days of working on the same thing my memory has become about as reliable as a variable that was never initialized, but these are the main things I can remember from this little coding adventure.
  
If you actually read this entire changelog, thank you. Seriously. I dont know why you did it, but I appreciate it.
  
The SL binary size has increased a little, which is expected considering that we now have an actual VM and all the stuff that comes with it. However, it is still smaller than Lua, so I dont think we did too badly here.
  
It is also a little slower than Lua at the moment, but considering that SL is still a relatively small language with a networking library, a console library, and the ability to communicate with C functions directly without having to go through some interface layer, I think that is a reasonable tradeoff for now.
  
And besides, the entire point of this update was to make execution faster, more structured, and easier to extend, not to magically beat every language in existence after four days of me slowly turning into a VM instruction myself.
  
I didnt really think too much when choosing the release name, I am an architect after all! Yes, its a Matrix reference, but I am not a program (yet.)
