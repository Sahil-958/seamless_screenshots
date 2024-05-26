# Aestishot Bash Script C++ Port

This is a current work in progress. The goal is to port the Aestishot Bash Script to C++.

Use following command to compile the program:
```bash
g++ -o <NAME_OF_COMPILED_BIN> <ProgramFile.cpp> `Magick++-config --cxxflags --cppflags --ldflags --libs`
```

Most of the functionality is already implemented. The only thing that is missing is the gradient background for the padding.

As most of the functionality is already implemented, but there is a need of
exception handling and error checking and better thread management. The program is not yet stable.

Also, i need to refactor the code and make it more readable and maintainable. As
currently the code is a mess. 
