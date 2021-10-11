%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */
 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */

, {adjust(); return Parser::COMMA;}
: {adjust(); return Parser::COLON;}
; {adjust(); return Parser::SEMICOLON;}
\( {adjust(); return Parser::LPAREN;}
\) {adjust(); return Parser::RPAREN;}
\[  {adjust(); return Parser::LBRACK;}
\]  {adjust(); return Parser::RBRACK;}
\{ {adjust(); return Parser::LBRACE;}
\} {adjust(); return Parser::RBRACE;}
\. {adjust(); return Parser::DOT;}
\+ {adjust(); return Parser::PLUS;}
\- {adjust(); return Parser::MINUS;}
\* {adjust(); return Parser::TIMES;}
\/ {adjust(); return Parser::DIVIDE;}
= {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

[0-9]+ {adjust(); return Parser::INT;}
[a-zA-Z][a-zA-Z0-9_]* {adjust(); return Parser::ID;}


\" {
  begin(StartCondition__::STR);
  adjust();
  string_buf_ = "";
}


<STR> {
  \" {
    begin(StartCondition__::INITIAL);
    std::string str = matched();
    adjustStr();
    setMatched(string_buf_);
    return Parser::STRING;
  }

  \\\" {
    std::string str = matched();
    string_buf_+='\"';
    adjustStr();
  }

  \\\\ {
    std::string str = matched();
    string_buf_+='\\';
    adjustStr();
  }

  \\n {
    std::string str = matched();
    string_buf_+= '\n'; 
    adjustStr();
  }
  \\t {
    std::string str = matched();
    string_buf_+= '\t'; 
    adjustStr();
  }
  \\[0-9]{3} {
    string_buf_+=(char)atoi(matched().c_str()+1);
    adjustStr();
  }
  \\[[:blank:]\n]+\\ {
    std::string str = matched();
    adjustStr();
  }

  \\\^[A-Z] {
    string_buf_ += matched()[2] - 'A' + 1;
    adjustStr(); 
  }

  \\.|. {
    std::string str = matched();
    adjustStr();
    string_buf_+= str;
  }
    
}


"/*" {
  comment_level_++;
  begin(StartCondition__::COMMENT);
  adjust();
}

<COMMENT> {
  "*/" {
    comment_level_--;
    if(comment_level_==1) {
      begin(StartCondition__::INITIAL);
    }
    adjustStr();
  }

  "/*" {
    comment_level_++;
    adjustStr();
  }

  .|\n {adjustStr();}
}





 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
