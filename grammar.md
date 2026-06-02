# Grammar Specification (EBNF)

## 1. Program Structure

```
program         = { declaration } ;
```

## 2. Declarations

```
declaration     = function_declaration
                | variable_declaration
                | struct_declaration
                | type_alias
                ;

function_declaration = type IDENTIFIER "(" parameter_list ")" compound_statement ;

variable_declaration = type IDENTIFIER array_suffix [ "=" expression ] ";" ;

struct_declaration   = "struct" IDENTIFIER "{" { variable_declaration } "}" ";" ;

type_alias           = "typedef" type IDENTIFIER ";" ;
```

## 3. Types

```
type            = base_type
                | struct_type
                | pointer_type
                | array_type
                ;

base_type       = "void"
                | "int"
                | "char"
                | "float"
                | "double"
                | "auto"
                ;

struct_type     = "struct" IDENTIFIER ;

pointer_type    = type "*" ;

array_type      = type array_suffix ;

array_suffix    = "[" [ expression ] "]" { "[" [ expression ] "]" } ;
```

## 4. Parameters

```
parameter_list  = parameter { "," parameter }
                | "void"
                | ε
                ;

parameter       = type IDENTIFIER array_suffix ;
```

## 5. Statements

```
statement       = compound_statement
                | expression_statement
                | if_statement
                | while_statement
                | do_while_statement
                | for_statement
                | switch_statement
                | break_statement
                | continue_statement
                | return_statement
                | variable_declaration
                ;

compound_statement  = "{" { statement } "}" ;

expression_statement = [ expression ] ";" ;

if_statement        = "if" "(" expression ")" statement
                      [ "else" statement ]
                    ;

while_statement     = "while" "(" expression ")" statement ;

do_while_statement  = "do" statement "while" "(" expression ")" ";" ;

for_statement       = "for" "(" [ expression ] ";" [ expression ] ";"
                      [ expression ] ")" statement
                    ;

switch_statement    = "switch" "(" expression ")" "{" { case_clause } "}" ;

case_clause         = "case" constant_expression ":" { statement }
                    | "default" ":" { statement }
                    ;

break_statement     = "break" ";" ;
continue_statement  = "continue" ";" ;
return_statement    = "return" [ expression ] ";" ;
```

## 6. Expressions

```
expression          = assignment_expression { "," assignment_expression } ;

assignment_expression = conditional_expression
                      | unary_expression assignment_operator assignment_expression
                      ;

assignment_operator = "=" | "+=" | "-=" | "*=" | "/=" | "%="
                    | "<<=" | ">>=" | "&=" | "^=" | "|="
                    ;

conditional_expression = logical_or_expression
                       | logical_or_expression "?" expression ":" conditional_expression
                       ;

logical_or_expression  = logical_and_expression { "||" logical_and_expression } ;
logical_and_expression = bitwise_or_expression { "&&" bitwise_or_expression } ;

bitwise_or_expression  = bitwise_xor_expression { "|" bitwise_xor_expression } ;
bitwise_xor_expression = bitwise_and_expression { "^" bitwise_and_expression } ;
bitwise_and_expression = equality_expression { "&" equality_expression } ;

equality_expression    = relational_expression { ("==" | "!=") relational_expression } ;
relational_expression  = shift_expression { ("<" | ">" | "<=" | ">=") shift_expression } ;
shift_expression       = additive_expression { ("<<" | ">>") additive_expression } ;
additive_expression    = multiplicative_expression { ("+" | "-") multiplicative_expression } ;
multiplicative_expression = cast_expression { ("*" | "/" | "%") cast_expression } ;

cast_expression        = unary_expression
                       | "(" type ")" cast_expression
                       ;

unary_expression       = postfix_expression
                       | "++" unary_expression
                       | "--" unary_expression
                       | unary_operator cast_expression
                       | "sizeof" unary_expression
                       | "sizeof" "(" type ")"
                       ;

unary_operator         = "&" | "*" | "+" | "-" | "~" | "!" ;

postfix_expression     = primary_expression
                       | postfix_expression "[" expression "]"
                       | postfix_expression "(" [ argument_expression_list ] ")"
                       | postfix_expression "." IDENTIFIER
                       | postfix_expression "->" IDENTIFIER
                       | postfix_expression "++"
                       | postfix_expression "--"
                       ;

primary_expression     = IDENTIFIER
                       | constant
                       | string_literal
                       | "(" expression ")"
                       | lambda_expression
                       ;

argument_expression_list = assignment_expression { "," assignment_expression } ;
```

## 7. Constants

```
constant            = integer_constant
                    | float_constant
                    | char_constant
                    | string_literal
                    ;

integer_constant    = DECIMAL_LITERAL | HEX_LITERAL | OCTAL_LITERAL ;
float_constant      = FLOAT_LITERAL ;
char_constant       = "'" CHARACTER "'" ;
string_literal      = '"' { CHARACTER } '"' ;
```

## 8. Lambda Expressions

```
lambda_expression   = "[" [ capture_list ] "]" "(" parameter_list ")" "->" type
                      compound_statement
                    ;

capture_list        = capture { "," capture } ;
capture             = "="
                    | "&"
                    | IDENTIFIER
                    | "&" IDENTIFIER
                    ;
```

## 9. Memory Management

```
allocation_expression = "malloc" "(" expression ")" ;
deallocation_statement = "free" "(" expression ")" ";" ;
```

## 10. Generic / Template Declarations

```
template_declaration = "template" "<" template_parameter_list ">"
                       ( function_declaration | struct_declaration )
                     ;

template_parameter_list = "typename" IDENTIFIER { "," "typename" IDENTIFIER } ;
```

## 11. Preprocessor

```
preprocessor        = "#include" < string_literal >
                    | "#define" IDENTIFIER [ replacement_list ]
                    | "#ifdef" IDENTIFIER
                    | "#ifndef" IDENTIFIER
                    | "#endif"
                    ;
```

## 12. Lexical Tokens

```
IDENTIFIER          = letter { letter | digit | "_" } ;
DECIMAL_LITERAL     = digit { digit } ;
HEX_LITERAL         = "0x" hex_digit { hex_digit } ;
OCTAL_LITERAL       = "0" octal_digit { octal_digit } ;
FLOAT_LITERAL       = digit { digit } "." digit { digit }
                      [ ("e" | "E") [ "+" | "-" ] digit { digit } ]
                    ;
CHARACTER           = printable_ascii
                    | escape_sequence
                    ;

letter              = "A".."Z" | "a".."z" ;
digit               = "0".."9" ;
hex_digit           = digit | "a".."f" | "A".."F" ;
octal_digit         = "0".."7" ;
escape_sequence     = "\\" ( "'" | '"' | "?" | "\\"
                      | "a" | "b" | "f" | "n" | "r" | "t" | "v"
                      | OCTAL_LITERAL
                      | "x" hex_digit hex_digit
                      )
                    ;

comment             = "//" { any_character } newline
                    | "/*" { any_character } "*/"
                    ;
```

## Operator Precedence (highest to lowest)

| Precedence | Category          | Operators              | Associativity |
|-----------|-------------------|------------------------|--------------|
| 1         | Postfix           | `[]` `()` `.` `->` `++` `--` | Left       |
| 2         | Unary             | `++` `--` `&` `*` `+` `-` `~` `!` `sizeof` | Right |
| 3         | Cast              | `(type)`               | Right        |
| 4         | Multiplicative    | `*` `/` `%`            | Left         |
| 5         | Additive          | `+` `-`                | Left         |
| 6         | Shift             | `<<` `>>`              | Left         |
| 7         | Relational        | `<` `>` `<=` `>=`      | Left         |
| 8         | Equality          | `==` `!=`              | Left         |
| 9         | Bitwise AND       | `&`                    | Left         |
| 10        | Bitwise XOR       | `^`                    | Left         |
| 11        | Bitwise OR        | `\|`                   | Left         |
| 12        | Logical AND       | `&&`                   | Left         |
| 13        | Logical OR        | `\|\|`                 | Left         |
| 14        | Conditional       | `?:`                   | Right        |
| 15        | Assignment        | `=` `+=` `-=` etc.     | Right        |
| 16        | Comma             | `,`                    | Left         |
