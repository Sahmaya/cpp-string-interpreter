

//Assignment 1
// Submitted by Sahmaya Anderson-Edwards


// This program is an interpreter for a simple string-processing language.
// It reads commands from stdin, breaks them into tokens (Lexer), checks the grammar (Parser),
// and runs the commands straight away (Interpreter).

// Design decisions:
//   -  String literals use the regex "(?:[^"\\]|\\.)*" to handle escaped quotes inside strings
//     so something like "say \"hi\"" doesn't end the literal too early.
//   -  When invalid input is typed, the interpreter prints an error and skips everything up to
//     the next semicolon before trying again so this way it never crashes on bad input.


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <algorithm>
 

// Token types
// Using an enum for readable names 
 
enum TokenType {
    TK_APPEND, TK_EXIT, TK_LIST, TK_PRINT,
    TK_PRINTLENGTH, TK_PRINTWORDS, TK_PRINTWORDCOUNT,
    TK_REVERSE, TK_SET,
    TK_CONSTANT, TK_END, TK_PLUS,
    TK_ID, TK_LITERAL,
    TK_EOF
};
 
// Puts the token type and the raw matched text together
struct Token {
    TokenType tokenType;
    std::string rawText;
};
 

// Lexer 
// Pairing a token type with its regex string
struct PatternEntry {
    TokenType tokenType;
    std::string regexPattern;
};
 
// Here the lexer tries patterns top-to-bottom and takes the first match. 
//Longer keywords (printlength) have to come before shorter ones (print) otherwise "printlength" would get matched as "print" with leftover "length".etc


static const std::vector<PatternEntry> TOKEN_PATTERNS = {
    { TK_PRINTLENGTH,    "printlength"               },
    { TK_PRINTWORDCOUNT, "printwordcount"             },
    { TK_PRINTWORDS,     "printwords"                 },
    { TK_APPEND,         "append"                     },
    { TK_EXIT,           "exit"                       },
    { TK_LIST,           "list"                       },
    { TK_PRINT,          "print"                      },
    { TK_REVERSE,        "reverse"                    },
    { TK_SET,            "set"                        },
    { TK_CONSTANT,       "SPACE|TAB|NEWLINE"          },
    { TK_END,            ";"                          },
    { TK_PLUS,           "\\+"                        },
    // opening quote then anything until closing quote
    { TK_LITERAL,        "\"(?:[^\"\\\\]|\\\\.)*\""   },
    // TK_ID last so keywords aren't seen as variable names
    { TK_ID,             "[a-zA-Z][a-zA-Z0-9]*"       },
};
 
 
class Lexer {
public:
    // Converts the input string into a list of tokens.
    // It scans left to rightand at each position tries every pattern until one matches. Throws if not.
    std::vector<Token> tokeniseInput(const std::string& inputText) {
        std::vector<Token> tokenList;
        size_t currentPosition = 0;
 
        while (currentPosition < inputText.size()) {
 
            // skip space between tokens
            while (currentPosition < inputText.size() && std::isspace((unsigned char)inputText[currentPosition]))
                ++currentPosition;
 
            if (currentPosition >= inputText.size()) break;
 
            bool matchFound = false;
 
            for (const auto& patternEntry : TOKEN_PATTERNS) {
                // anchor with ^ so we only match at the current position
                std::regex compiledRegex("^(?:" + patternEntry.regexPattern + ")", std::regex::ECMAScript);
                std::smatch regexMatch;
                std::string remainingInput = inputText.substr(currentPosition);
 
                if (std::regex_search(remainingInput, regexMatch, compiledRegex)) {
 
                    size_t matchEndPosition = currentPosition + regexMatch[0].length();
 
                    // Boundary check for keywords. 
                  //e.g. "setColour" shouldn't match "set"
                    if (patternEntry.tokenType >= TK_APPEND && patternEntry.tokenType <= TK_SET) {
                        if (matchEndPosition < inputText.size() &&
                            std::isalnum((unsigned char)inputText[matchEndPosition])) {
                            continue;
                        }
                    }
 
                    tokenList.push_back({ patternEntry.tokenType, regexMatch[0].str() });
                    currentPosition = matchEndPosition;
                    matchFound = true;
                    break;
                }
            }
 
            if (!matchFound) {
                throw std::runtime_error(
                    std::string("Unrecognised character: '") + inputText[currentPosition] + "'"
                );
            }
        }
 
        // EOF marker so the parser knows when tostop
        tokenList.push_back({ TK_EOF, "" });
        return tokenList;
    }
};
 
 

//Converting a token into its actual string value
 
static std::string convertLiteralToString(const std::string& rawLiteralToken) {
    std::string resultString;
    size_t characterIndex = 1; 
 
    while (characterIndex < rawLiteralToken.size() - 1) { 
        if (rawLiteralToken[characterIndex] == '\\' && characterIndex + 1 < rawLiteralToken.size() - 1) {
            char escapedCharacter = rawLiteralToken[characterIndex + 1];
            switch (escapedCharacter) {
                case '"':  resultString += '"';  break;
                case '\\': resultString += '\\'; break;
                case 'n':  resultString += '\n'; break;
                case 't':  resultString += '\t'; break;
                case 'r':  resultString += '\r'; break;

                default:   resultString += escapedCharacter; 
                break;
            }
            characterIndex += 2;
        } else {
            resultString += rawLiteralToken[characterIndex];
            ++characterIndex;
        }
    }
    return resultString;
}
 
 



// Extracting all the words from a string
// A word is a run of letters/digits. Hyphens and apostrophes are included inside a word when they sit between alphanumeric  on both sides so "let's" and "runner-up" each count as one word
 
static std::vector<std::string> extractWordsFromString(const std::string& inputString) {
    std::vector<std::string> wordList;

    std::regex wordPattern("'?[A-Za-z0-9]+(?:['\\-][A-Za-z0-9]+)*");
 
    auto matchIteratorStart = std::sregex_iterator(inputString.begin(), inputString.end(), wordPattern);
    auto matchIteratorEnd = std::sregex_iterator();
 
    for (auto iterator = matchIteratorStart; iterator != matchIteratorEnd; ++iterator) {
        wordList.push_back((*iterator).str());
      }
    
  return wordList;


}
 
 

// Interpreter 
class Interpreter {
public:
    // std::map keeps keys sorted in order 
    std::map<std::string, std::string> symbolTable;
 
    // Takes a token list, resets position, then loops calling parseStatement() until EOF. 
    // Errors are caught here to prevent any chrashes 
    
    void run(const std::vector<Token>& tokenStream) {
        allTokens = tokenStream;
        currentTokenIndex = 0;
 
        while (getCurrentToken().tokenType != TK_EOF) {
            try {
                parseStatement();
            } catch (const std::exception& parseError) {
                std::cerr << "Error: " << parseError.what() << std::endl;
                recoverFromError();
            }
        }
    }
 



private:
    std::vector<Token> allTokens;
    size_t currentTokenIndex;
 


    // returns the token at current position 
    Token& getCurrentToken() {
        return allTokens[currentTokenIndex];
     }
 




     // Goes past the current token and returns it.Throws if the token type doesn't match 
    Token consumeToken(TokenType expectedType = TK_EOF) {
        Token currentToken = getCurrentToken();
        if (expectedType != TK_EOF && currentToken.tokenType != expectedType) {
            throw std::runtime_error(
                "Expected token type " + std::to_string(expectedType) +
                " but got '" + currentToken.rawText + "'"
            );
        }
        ++currentTokenIndex;
        return currentToken;
    }
 
    // Skips tokens until there is a semicolon in case sentences get broken
    void recoverFromError() {
       
        while (getCurrentToken().tokenType != TK_END &&
               getCurrentToken().tokenType != TK_EOF) {
            ++currentTokenIndex;
        }
      
      
          if (getCurrentToken().tokenType == TK_END)
            ++currentTokenIndex;
    }
 
 



    
    // parsing the tokens
    
    void parseStatement() {
        TokenType statementKeyword = getCurrentToken().tokenType;
 
        if (statementKeyword == TK_APPEND) {
            consumeToken(TK_APPEND);
            std::string variableName = consumeToken(TK_ID).rawText;
            std::string expressionValue = parseExpression();
            consumeToken(TK_END);
            executeAppend(variableName, expressionValue);
 
        } else if (statementKeyword == TK_LIST) {
            consumeToken(TK_LIST);
            consumeToken(TK_END);
            executeList();
 
        } else if (statementKeyword == TK_EXIT) {
            consumeToken(TK_EXIT);
            consumeToken(TK_END);
            std::cout << "Exiting." << std::endl;
            std::exit(0);
 
        } else if (statementKeyword == TK_PRINT) {
            consumeToken(TK_PRINT);
            std::string expressionValue = parseExpression();
            consumeToken(TK_END);
            std::cout << expressionValue << std::endl;
 
        } else if (statementKeyword == TK_PRINTLENGTH) {
            consumeToken(TK_PRINTLENGTH);
          std::string expressionValue = parseExpression();
            consumeToken(TK_END);
            std::cout << "Length is: " << expressionValue.size() << std::endl;
 
        } else if (statementKeyword == TK_PRINTWORDS) {
            consumeToken(TK_PRINTWORDS);
            std::string expressionValue = parseExpression();
               consumeToken(TK_END);
            std::vector<std::string> extractedWords = extractWordsFromString(expressionValue);
            std::cout << "Words are:" << std::endl;
          
               for (const std::string& singleWord : extractedWords)
                std::cout << singleWord << std::endl;
 
        } else if (statementKeyword == TK_PRINTWORDCOUNT) {
            consumeToken(TK_PRINTWORDCOUNT);
            std::string expressionValue = parseExpression();
            consumeToken(TK_END);
            std::cout << "Wordcount is: " << extractWordsFromString(expressionValue).size() << std::endl;
 
        } else if (statementKeyword == TK_SET) {
            consumeToken(TK_SET);
            std::string variableName = consumeToken(TK_ID).rawText;
            std::string expressionValue = parseExpression();
            consumeToken(TK_END);
            symbolTable[variableName] = expressionValue;
        } else if (statementKeyword == TK_REVERSE) {
            consumeToken(TK_REVERSE);
            std::string variableName = consumeToken(TK_ID).rawText;
            consumeToken(TK_END);
            executeReverse(variableName);
 
        } else {
            throw std::runtime_error(
                "Unexpected token at start of statement: '" + getCurrentToken().rawText + "'"
            );
        }
    }
 






    // expression := value { plus value }
    // Parses the first value and then it keeps concatenating more
    // The + handles things like "hello" + SPACE + "world"
    std::string parseExpression() {
        std::string concatenatedResult = parseValue();
 
        while (getCurrentToken().tokenType == TK_PLUS) {
            consumeToken(TK_PLUS);
            concatenatedResult += parseValue();
        }
        return concatenatedResult;
    }
 
    // value := id | constant | literal
    // A value is either a variable (looked up in the symbol table),
    // a constant (SPACE/TAB/NEWLINE), or a string literal.
    std::string parseValue() {
        TokenType valueType = getCurrentToken().tokenType;
 
        if (valueType == TK_ID) {
            std::string variableName = consumeToken(TK_ID).rawText;
            auto lookupResult = symbolTable.find(variableName);
            if (lookupResult == symbolTable.end())
                throw std::runtime_error("Undefined variable: '" + variableName + "'");
            return lookupResult->second;
 
        } else if (valueType == TK_CONSTANT) {
            std::string constantName = consumeToken(TK_CONSTANT).rawText;
            if (constantName == "SPACE")   return " ";
            if (constantName == "TAB")     return "\t";
            if (constantName == "NEWLINE") return "\n";
            return "";
 
        } else if (valueType == TK_LITERAL) {
            std::string rawLiteralText = consumeToken(TK_LITERAL).rawText;
            return convertLiteralToString(rawLiteralText);
 
        } else {
            throw std::runtime_error(
                "Expected a value (variable, constant, or string literal) but got '" +
                getCurrentToken().rawText + "'"
            );
        }
    }
 
 




    // Statement executors
    // append: adds to the end of the existing variables. 
    //Errors if variable doesn't exist
    void executeAppend(const std::string& variableName, const std::string& newValue) {
        auto lookupResult = symbolTable.find(variableName);
        if (lookupResult == symbolTable.end())
            throw std::runtime_error("Undefined variable: '" + variableName + "'");
        lookupResult->second += newValue;
    }
 
    // list: prints all of the variables in a map to keep them sorted 
    void executeList() {
        std::cout << "Identifier list (" << symbolTable.size() << "):" << std::endl;
        for (const auto& tableEntry : symbolTable)
            std::cout << "  " << tableEntry.first << ": \"" << tableEntry.second << "\"" << std::endl;
    }
 
    // reverse: splits value into words, reverses their order and then joins back with spaces
    void executeReverse(const std::string& variableName) {
        auto lookupResult = symbolTable.find(variableName);
        if (lookupResult == symbolTable.end())
            throw std::runtime_error("Undefined variable: '" + variableName + "'");
 
        std::vector<std::string> wordList = extractWordsFromString(lookupResult->second);
        std::reverse(wordList.begin(), wordList.end());
        std::string reversedString;
        for (size_t wordIndex = 0; wordIndex < wordList.size(); ++wordIndex) {
            if (wordIndex > 0) reversedString += ' ';
            reversedString += wordList[wordIndex];
        }
        lookupResult->second = reversedString;
    }
};
 
// replace curly quotes with straight quotes to prevent curly quotes from breaking the program
static std::string fixQuotes(const std::string& input) {
    std::string result = input;
    // left curly quote " is UTF-8: E2 80 9C
    // right curly quote " is UTF-8: E2 80 9D
    std::string lq = "\xe2\x80\x9c";
    std::string rq = "\xe2\x80\x9d";
    size_t pos;
    while ((pos = result.find(lq)) != std::string::npos)
        result.replace(pos, 3, "\"");
    while ((pos = result.find(rq)) != std::string::npos)
        result.replace(pos, 3, "\"");
    return result;
}

// Main function to read input line by line and create a buffer.It parses once the buffer contains a semicolon 
int main() {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "  159.341 Assignment 1 Semester 1 2026  " << std::endl;
    std::cout << "  Submitted by: Sahmaya Anderson-Edwards, 24012404  " << std::endl;
    std::cout << "----------------------------------------" << std::endl;
 
    Interpreter myInterpreter;
    Lexer myLexer;
    std::string inputBuffer;
    std::string currentLine;
 
    while (std::getline(std::cin, currentLine)) {
        inputBuffer += currentLine + '\n';
 
        // no semicolon means its not a complete statement yet
        if (inputBuffer.find(';') == std::string::npos)
            continue;
 
        std::vector<Token> tokenList;
        try {
            tokenList = myLexer.tokeniseInput(fixQuotes(inputBuffer));
        } catch (const std::exception& lexError) {
            std::cerr << "Lex error: " << lexError.what() << std::endl;
            size_t semicolonPosition = inputBuffer.find(';');
            if (semicolonPosition != std::string::npos)
                inputBuffer = inputBuffer.substr(semicolonPosition + 1);
            else
                inputBuffer.clear();
            continue;
        }
 
        // find the last semicolon so thateverything up to it gets processed now and anything after stays in the buffer as the start of the next statement
        int lastSemicolonIndex = -1;
        for (int tokenIndex = 0; tokenIndex < (int)tokenList.size(); ++tokenIndex) {
            if (tokenList[tokenIndex].tokenType == TK_END)
                lastSemicolonIndex = tokenIndex;
        }
 
        if (lastSemicolonIndex == -1) continue;
 
        std::vector<Token> tokensToProcess(tokenList.begin(), tokenList.begin() + lastSemicolonIndex + 1);
        tokensToProcess.push_back({ TK_EOF, "" });
 


        // reconstruct remainder from the  tokens after the last semicolon
        std::string remainderBuffer;
        for (int tokenIndex = lastSemicolonIndex + 1; tokenIndex < (int)tokenList.size() - 1; ++tokenIndex)
            remainderBuffer += tokenList[tokenIndex].rawText + ' ';
        inputBuffer = remainderBuffer;
 
        myInterpreter.run(tokensToProcess);
    }
 
    return 0;
}
