// lexer's include file
enum TokenType {
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_SYMBOL_IDENTIFIER,
    TOKEN_STRING_LITERAL,
    TOKEN_NUMBER,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN
};

// function factorial(int n)
// Token: "function", Type: KEYWORD_FUNCTION  
// Token: "factorial", Type: SYMBOL_IDENTIFIER  
// Token: "(", Type: SYMBOL_OPEN_PARENTHESIS  
// Token: "int", Type: SYMBOL_IDENTIFIER  
// Token: "n", Type: SYMBOL_IDENTIFIER  
// Token: ")", Type: SYMBOL_CLOSING_PARENTHESIS  
