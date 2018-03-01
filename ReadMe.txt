
Use the C++ language like a scripting language.

This tool arose when i felt that i would like to write my quick and dirty scripts not only in
bash, perl or php, but also in C++ .

But C++ programs are compiled with build systems that need to be instructed via make files or other
project files about how to transform multiple source files into a program.

So i wrote this tool that can be referenced as a shebang, and if invoked this way it compiles the
file and exec's the resulting program. Thus it forms a "script interpreter" that executes a C++ file.
The tool is named "CppRunner" and i like to refer to the C++ file it compiles as a C++ script.


Current limitations:
 - Does not yet consider files #included to detect when a recompile is necessary.
 - Maybe does not follow good security practises.
 - Refuses operation if the C++ file is not in a certain directory.
 - One cannot specify certain compiler options like which library to link to the program.


To do its task CppRunner needs write access in the directory the C++ source file resides.
It writes the resulting executable there, and also an intermediate file. The intermediate file
is necessary because a C++ compiler would not compile the file if it finds the shebang line.
Therefore the first thing CppRunner does when it decides that a recompile is necessary is to
create a copy of the C++ script that has the shebang omitted. It uses #line directives to
make the compiler refer to the original file in error messages.

A hello world C++ script looks like this:


#!/usr/bin/cpprunner

#include <math.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  printf("Hello Wrld!\n");

return 0;
}


Thus a C++ script is actually a normal single-file C++ program with an additional shebang line
at the top. That's all.

Since CppRunner and thus the compiler is invoked via a shebang line it is not necessary for the
C++ script file to have a certain file ending like .cpp or .cxx, etc.

To enable correct language feature in an editor, e.g. syntax coloring, text editors should detect
the shebang line, like is done with other scripting languages.
Below is instructions on how to extend vim to do this.


To run such a C++ script, do the same as with any other scripting language: set its x flag (chmod u+x)
and refer to it as a command (./MyHelloWorldScript).
What happens then is that the operating system sees the shebang line and invokes CppRunner, passing
it the C++ file as a parameter. CppRunner then checks if a compiled version of the script exists and
which one is younger. If the C++ script file is younger, it must be compiled. CppRunner then creates
a copy of the script file without the shebang line removed and calls g++ to compile it. g++ is a
child process of CppRunner. If the compilation succeeds, CppRunner uses the exec system call to replace
itself with the compiled binary. Thus the actual program runs in the very process that the operating
system has created to execute the C++ script.


To activating syntax highlighting in VIM for C++ scripts:


 In   /usr/share/vim/current/scripts.vim   there is a if-elseif section for various interpreter names
 found in the shebang line (first line starting with '#!'). Add this to the elseif section and you'll
 have C++ syntax highlighting for your cpprunner scripts.

elseif s:name =~ 'cpprunner'
  set ft=cpp
  " TBD: Would actually have to examine the second line if it has a //#! to see which compiler is actually
  "      called and in that case may set to another language.



