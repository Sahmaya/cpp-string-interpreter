# C++ String Language Interpreter
 
A complete interpreter for a custom string-processing language, built from scratch in C++ as part of my Programming Languages, Algorithms, and Concurrency Course.
 
## Overview
 
The interpreter reads commands from stdin, processes them through a three-stage pipeline, and executes them immediately. It was built to demonstrate understanding of how programming languages work at an implementation level, from raw text through to execution.
 
## Example Programs
 
```
set one "hello";
set two "world";
set sentence one + SPACE + two;
print sentence;
printwords sentence;
printwordcount sentence;
reverse sentence;
list;
```
 
## Supported Commands
 
| Command | Description |
|---------|-------------|
| set id expression ; | Assign value to variable |
| append id expression ; | Append value to variable |
| print expression ; | Print value |
| printlength expression ; | Print string length |
| printwords expression ; | Print each word on a new line |
| printwordcount expression ; | Print number of words |
| reverse id ; | Reverse word order |
| list ; | List all variables and values |
| exit ; | Terminate the interpreter |
 
Expressions are values joined by + for concatenation. Special constants SPACE, TAB and NEWLINE represent whitespace characters.
 
## Architecture
 
The interpreter uses a classic three-stage pipeline:
 
### 1. Lexer (Tokeniser)
Reads raw input text and breaks it into tokens using regex pattern matching. Each token is classified by type: keywords, identifiers, string literals, constants, operators and terminators.
 
String literals use the regex pattern `"(?:[^"\\]|\\.)*"` to correctly handle escaped quotes inside strings, so `"say \"hi\""` is treated as one literal rather than ending at the first escaped quote.
 
Keywords are matched with boundary checking to prevent `setColour` from being tokenised as keyword SET followed by identifier `Colour`.
 
### 2. Parser (Recursive Descent)
Validates the grammatical structure of each statement. One function per grammar rule, each looking at the next token to decide which rule applies.
 
```
program    := { statement }
statement  := set id expression ;
            | append id expression ;
            | print expression ;
            | reverse id ;
            | list ;
            | exit ;
expression := value { + value }
value      := id | literal | constant
```
 
### 3. Interpreter (Execution Engine)
Maintains a symbol table (std::map) mapping variable names to string values and executes each command as the parser processes it.
 
Word boundary detection uses the regex `[a-zA-Z0-9]+(?:['\-][a-zA-Z0-9]+)*` which correctly handles contractions and hyphenated words as single words.
 
## Error Handling
 
On invalid input, the interpreter prints a descriptive error message and skips all tokens until the next semicolon before resuming. This means bad input never crashes the program and all errors in a program are reported in a single run.
 
## How to Run
 
```bash
g++ -std=c++11 -o interpreter strings_interpreter.cpp
./interpreter
```
 
Then type commands directly or pipe a file:
 
```bash
./interpreter < program.txt
```
 
## Known Limitations
 
Multiline string literals are not currently supported.
 
## Key Concepts Demonstrated
 
- Lexical analysis with regex pattern matching
- Recursive descent parsing
- Symbol table implementation with std::map
- Error recovery without crashing
- Word boundary detection with regex
 
