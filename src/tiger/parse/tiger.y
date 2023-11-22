%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }


//定义一些terminal
%token <sym> ID
%token <sval> STRING
%token <ival> INT
//对于以下的terminal不用定义union(不用存储他们的value)
%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  //PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  ASSIGN //AND OR 
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

 /* token priority */
 /* TODO: Put your lab3 code here */

//对于运算符的运算优先级定义如下:先||,再&&,再==,!=,<,<=,>,>=,再+,-,最后*,/,并且对于运算符都是左结合的
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE

%type <exp> exp expseq
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
//程序就是一个表达式exp
program:  exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

//对于左值有以下这几种情况:变量，记录域，数组元素
lvalue:  ID  {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  |  oneormore  {$$ = $1;}
  ;

 /* TODO: Put your lab3 code here */

//注意对于左值此处如果直接写书本上的表达式可能会产生歧义。
one:  ID  DOT  ID  {$$ = new absyn::FieldVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  |  ID  LBRACK  exp  RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  ;

oneormore:  one  {$$ = $1;}
  |  oneormore DOT ID  {$$ = new absyn::FieldVar(scanner_.GetTokPos(), $1, $3);}
  |  oneormore LBRACK  exp  RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), $1, $3);}
  ;

//对于声明序列，可以是空，也可以是非空声明序列
decs:  {$$ = new absyn::DecList();}
  |  decs_nonempty  {$$ = $1;}
  ;

//对于非空的声明序列可以分为多种声明序列或者一种声明序列
decs_nonempty:  decs_nonempty_s  {$$ = new absyn::DecList($1);}
  |  decs_nonempty_s  decs_nonempty  {$$ = ($2)->Prepend($1);}
  ;

//对于声明序列，包含类型，值，函数声明三种，对应于tydec,vardec,fundec
decs_nonempty_s:  tydec  {$$ = new absyn::TypeDec(scanner_.GetTokPos(), $1);}
  |  vardec  {$$ = $1;}
  |  fundec  {$$ = new absyn::FunctionDec(scanner_.GetTokPos(), $1);}
  ;

//对于类型声明序列，可以是一种类型声明，也可以是多种类型声明
tydec:  tydec_one  {$$ = new absyn::NameAndTyList($1);}
  |  tydec_one  tydec  {$$ = ($2)->Prepend($1);}
  ;

//对于一种类型声明，包含类型名和类型
tydec_one:  TYPE  ID  EQ  ty  {$$ = new absyn::NameAndTy($2, $4);}
  ;

//对于类型，可以格式某个已经定义的类型type-id，也可以是记录{tyfields}，也可以是数组array of type-id
ty:  ID  {$$ = new absyn::NameTy(scanner_.GetTokPos(), $1);}
  |  LBRACE  tyfields  RBRACE  {$$ = new absyn::RecordTy(scanner_.GetTokPos(), $2);}
  |  ARRAY  OF  ID  {$$ = new absyn::ArrayTy(scanner_.GetTokPos(), $3);}
  ;

//对于类型域序列，可以为空，也可以是非空类型域序列
tyfields:  {$$ = new absyn::FieldList();}
  |  tyfields_nonempty  {$$ = $1;}
  ;

//对于非空类型域序列，可以是一个类型域，也可以是多个类型域
tyfields_nonempty:  tyfield  {$$ = new absyn::FieldList($1);}
  |  tyfield  COMMA tyfields_nonempty  {$$ = ($3)->Prepend($1);}
  ;

//对于tyfield,包含id和type-id
tyfield:  ID  COLON  ID  {$$ = new absyn::Field(scanner_.GetTokPos(), $1, $3);}
  ;

//对于变量声明可以指定类型，也可以不指定类型
vardec:  VAR  ID  ASSIGN  exp  {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, nullptr, $4);}
  |  VAR  ID  COLON  ID  ASSIGN  exp  {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, $4, $6);}
  ;

//对于函数声明序列，可以是一个函数声明，也可以是多个函数声明
fundec:  fundec_one  {$$ = new absyn::FunDecList($1);}
  |  fundec_one  fundec  {$$ = ($2)->Prepend($1);}
  ;

//对于单个函数声明，可以有返回值，也可以没有返回值(若有需要制定返回值的类型)，括号中的参数列表是一个tyfields
fundec_one:  FUNCTION  ID  LPAREN  tyfields  RPAREN  EQ  exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, nullptr, $7);}
  |  FUNCTION  ID  LPAREN  tyfields  RPAREN  COLON  ID  EQ  exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, $7, $9);}
  ;

//表达式
exp: lvalue  {$$ = new absyn::VarExp(scanner_.GetTokPos(), $1);}
//nil
  |  NIL  {$$ = new absyn::NilExp(scanner_.GetTokPos());}
//序列
  |  LPAREN  expseq  RPAREN  {$$ = $2;}
//无值
  |  LPAREN  RPAREN  {$$ = new absyn::VoidExp(scanner_.GetTokPos());}
//整形字面量
  |  INT  {$$ = new absyn::IntExp(scanner_.GetTokPos(), $1);}
//字符串字面量
  |  STRING  {$$ = new absyn::StringExp(scanner_.GetTokPos(), $1);}
//负值
  |  MINUS  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::MINUS_OP, new absyn::IntExp(scanner_.GetTokPos(), 0), $2);}
//函数调用
  |  ID  LPAREN  actuals  RPAREN  {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, $3);}
//算数操作
  |  exp  PLUS  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::PLUS_OP, $1, $3);}
  |  exp  MINUS  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::MINUS_OP, $1, $3);}
  |  exp  TIMES  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::TIMES_OP, $1, $3);}
  |  exp  DIVIDE  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::DIVIDE_OP, $1, $3);}
//比较操作
  |  exp  EQ  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::EQ_OP, $1, $3);}
  |  exp  NEQ  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::NEQ_OP, $1, $3);}
  |  exp  GT  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::GT_OP, $1, $3);}
  |  exp  LT  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::LT_OP, $1, $3);}
  |  exp  GE  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::GE_OP, $1, $3);}
  |  exp  LE  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::LE_OP, $1, $3);}
//布尔操作
  |  exp  AND  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::AND_OP, $1, $3);}
  |  exp  OR  exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::Oper::OR_OP, $1, $3);}
//记录创建
  |  ID  LBRACE  rec  RBRACE  {$$ = new absyn::RecordExp(scanner_.GetTokPos(), $1, $3);}
//数组创建
  |  ID  LBRACK  exp  RBRACK  OF  exp  {$$ = new absyn::ArrayExp(scanner_.GetTokPos(), $1, $3, $6);}
//赋值语句
  |  lvalue  ASSIGN  exp  {$$ = new absyn::AssignExp(scanner_.GetTokPos(), $1, $3);}
//if-then-else语句
  |  IF  exp  THEN  exp  ELSE  exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, $6);}
//if-then语句
  |  IF  exp  THEN  exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, nullptr);}
//while语句
  |  WHILE  exp  DO  exp  {$$ = new absyn::WhileExp(scanner_.GetTokPos(), $2, $4);}
//for语句
  |  FOR  ID  ASSIGN  exp  TO  exp  DO  exp  {$$ = new absyn::ForExp(scanner_.GetTokPos(), $2, $4, $6, $8);}
//break语句
  |  BREAK  {$$ = new absyn::BreakExp(scanner_.GetTokPos());}
//let语句
  |  LET  decs  IN  expseq  END  {$$ = new absyn::LetExp(scanner_.GetTokPos(), $2, $4);}
//圆括号
  |  LPAREN  exp  RPAREN  {$$ = $2;}
  ;


//用于表达式中的记录创建
rec:  {$$ = new absyn::EFieldList();}
  |  rec_nonempty  {$$ = $1;}
  ;

//非空的记录
rec_nonempty:  rec_one  {$$ = new absyn::EFieldList($1);}
  |  rec_one  COMMA  rec_nonempty  {$$ = ($3)->Prepend($1);}
  ;

//对于记录中的一个域，包含id和表达式
rec_one:  ID  EQ  exp  {$$ = new absyn::EField($1, $3);}
  ;

//函数的参数表, 讨论空和非空
actuals:  {$$ = new absyn::ExpList();}
  |  nonemptyactuals  {$$ = $1;}
  ;

//非空参数表,每个参数是一个表达式，每个参数之间利用逗号分隔
nonemptyactuals:  exp  {$$ = new absyn::ExpList($1);}
  |  exp  COMMA  nonemptyactuals  {$$ = ($3)->Prepend($1);}
  ;


//序列,相当于只做一个类型转换
expseq:  sequencing  {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);}
  ;

//讨论空和非空表达式列表
sequencing:  {$$ = new absyn::ExpList();}
  |  sequencing_exps  {$$ = $1;}
  ;

//非空表达式列表再一个个拆解
sequencing_exps: exp  {$$ = new absyn::ExpList($1);}
  |  exp  SEMICOLON  sequencing_exps  {$$ = ($3)->Prepend($1);}
  ;
