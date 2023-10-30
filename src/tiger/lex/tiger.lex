%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

/* You can add lex definitions here. */
/* TODO: Put your lab2 code here */


/* 在正则表达式前面不加状态就相当于INITIAL ESC用于识别转义字符*/
%x COMMENT STR IGNORE STR_ESC STR_ESC_CNTL

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

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */
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
 /* operator */
 "," {adjust(); return Parser::COMMA;}
 ":" {adjust(); return Parser::COLON;}
 ";" {adjust(); return Parser::SEMICOLON;}
 "(" {adjust(); return Parser::LPAREN;}
 ")" {adjust(); return Parser::RPAREN;}
 "[" {adjust(); return Parser::LBRACK;}
 "]" {adjust(); return Parser::RBRACK;}
 "{" {adjust(); return Parser::LBRACE;}
 "}" {adjust(); return Parser::RBRACE;}
 "." {adjust(); return Parser::DOT;}
 "+" {adjust(); return Parser::PLUS;}
 "-" {adjust(); return Parser::MINUS;}
 "*" {adjust(); return Parser::TIMES;}
 "/" {adjust(); return Parser::DIVIDE;}
 "=" {adjust(); return Parser::EQ;}
 "<>" {adjust(); return Parser::NEQ;}
 "<" {adjust(); return Parser::LT;}
 "<=" {adjust(); return Parser::LE;}
 ">" {adjust(); return Parser::GT;}
 ">=" {adjust(); return Parser::GE;}
 "&" {adjust(); return Parser::AND;}
 "|" {adjust(); return Parser::OR;}
 ":=" {adjust(); return Parser::ASSIGN;}
 /* 处理变量名ID */
 [[:alpha:]]([[:alnum:]]|_)* {adjust(); return Parser::ID;}
 /* 处理整形字面量 */
 [0-9]+ {adjust(); return Parser::INT;}

 /* 以下为处理字符串字面量的逻辑 */
 //当识别到一个引号的时候进入状态STR, 准备开始识别字符串, 注意在字符串中识别的时候需要用adjustStr(),因为整个字符串是一个token, 只有在整个token被识别完的时候(即碰到结束的"时)才应该调用adjust()
 "\"" {adjust(); begin(StartCondition__::STR); }
 //对于进入STR后读到的内容都先写入Scanner类的string_buf_中, 在最后的时候利用setMatched()将整个string_buf_设置到这个STR token的value上
 <STR> {
  //遇到非转义字符时的处理
  ([[:print:]]{-}[\"\\]|[[:space:]])* {
    adjustStr();
    addStr2StrBuf(matched());
  } 
  //遇到转义字符时的处理, 进入处理转义字符的状态ESC
  \\ {adjustStr(); begin(StartCondition__::STR_ESC);}
  //遇到下一个"的时候结束这个字符串的识别, 注意这里仍旧是adjustStr因为这一个字符串作为token它的token position已经在开头记录了
  "\"" {adjustStr(); strOver(); begin(StartCondition__::INITIAL); return Parser::STRING;}
 }
 //在识别字符串的过程中对于转义字符的处理
 <STR_ESC>{
  n {adjustStr(); addChar2StrBuf('\n'); begin(StartCondition__::STR);}
  t {adjustStr(); addChar2StrBuf('\t'); begin(StartCondition__::STR);}
  //控制字符处理,转入处理控制字符的状态
  "^" {adjustStr(); begin(StartCondition__::STR_ESC_CNTL);}
  [0-9]{3} {adjustStr(); addChar2StrBuf((char)(std::stoi(matched()))); begin(StartCondition__::STR);}
  "\"" {adjustStr(); addChar2StrBuf('\"'); begin(StartCondition__::STR);}
  \\ {adjustStr(); addChar2StrBuf('\\'); begin(StartCondition__::STR);}
  //一个或者多个格式化字符，再加上一个\这些将被忽略
  //...存在一个问题:这里的被忽略的序列中如果有换行符，需要更新errormsg->Newline()吗？
  (([[:blank:]]{+}[\n])+)\\ {adjustStr(); begin(StartCondition__::STR);}
 }
 <STR_ESC_CNTL>{
  [A-Z] {adjustStr(); addChar2StrBuf((char)(matched()[0] - 'A' + 1)); begin(StartCondition__::STR);}
  "[" {adjustStr(); addChar2StrBuf((char)27); begin(StartCondition__::STR);}
  \\ {adjustStr(); addChar2StrBuf((char)28); begin(StartCondition__::STR);}
  "]" {adjustStr(); addChar2StrBuf((char)29); begin(StartCondition__::STR);}
  "^" {adjustStr(); addChar2StrBuf((char)30); begin(StartCondition__::STR);}
  "_" {adjustStr(); addChar2StrBuf((char)31); begin(StartCondition__::STR);}
 }

 /* 以下为处理COMMENT的逻辑 */
 //当读到/*时进入COMMENT状态，准备读取注释，并且需要增加当前的注释的层级
 "/*" {adjust(); begin(StartCondition__::COMMENT); ++comment_level_;}
 <COMMENT> {
  //当再次读到一个/*时, 增加注释的层级
  "/*" {adjust(); ++comment_level_;}
  //当读到一个*/时, 减少一层注释层级, 若注释层级减到1(因为初始化的时候设置的是1), 则代表这里注释结束，应该回到INITIAL状态
  "*/" {
    adjust();
    --comment_level_;
    if(comment_level_ == 1){
      begin(StartCondition__::INITIAL);
    }
  }
  //对于其他的所有输入全部跳过
  . {adjust();}
  //...注释中的换行也要计入errormsg的换行统计当中
  \n {adjust(); errormsg_->Newline();}
 }

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
//处理其它状态的不合理输入
<*>. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
