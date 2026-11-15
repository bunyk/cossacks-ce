# Cossacks Porting

We are porting old windows game Cossacks to Linux. The game was written in C++ and used DirectX, now it is using SDL3.

## Workflows
Do NOT run the produced executable directly!
The game is interactive and will hang. Ask user to run and provide logs.

When in need to figure out how something happens we could use plain old printf to STDOUT.

Once done, run `echo -ne '\007'` to notify me.

## Code changes
Try to minimize code changes, especially movements and additions. We try to keep code same, but only required code.

When investigating code, leave comments about the code purpose,
meaning of function parameters, possible values for enums, etc. once you are 
sure about them.
So code we investigated is more readable than before, and requires less investigation later.

Do not write comments like "un-guarded", "ported", especially for code that is deleted. We test and make sure it works, if it does not - we could compare with old code in git.


# Book
Also in book/ folder we are working on a book that explains RTS programming using Cossacks game engine as an example.

Book references parts of the code, but code changes, bugs are fixed, parts are ported, comments added. Instead of referencing parts of code by line number (as {filename}:{line} or similar), just name functions and methods, or if short - include short listing.
