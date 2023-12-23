// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "PL0.h"
#include "set.c"
void array_visit(short arr_index, int dim, symset fsys);//visit array
//////////////////////////////////////////////////////////////////////

// print error message.
void error(int n){
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

// get a character from input file.
void getch(void){
	// cc读入的字符数，ll当前行长度，ch当前字符
	if (cc == ll){  // get character to end of line
		if (feof(infile)){
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);
		while ( (!feof(infile))&& ((ch = getc(infile)) != '\n'))//read a line
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];
} // getch

//---------------------------------
//-------------词法分析
//---------------------------------
//从源文件中读出若干有效字符，组成一个 token 串，识别它的类型为保留字/标识
//符/数字或是其它符号。如果是保留字，把 sym 置成相应的保留字类型，如果是标
//识符，把 sym 置成 ident 表示是标识符，于此同时，id 变量中存放的即为保留
//字字符串或标识符名字。如果是数字，把 sym 置为 number,同时 num 变量中存
//放该数字的值。如果是其它的操作符，则直接把 sym 置成相应类型。经过本过程后
//ch 变量中存放的是下一个即将被识别的字符
void getsym(void){
	int i, k;
	char a[MAXIDLEN + 1];//save an identifier

	while (ch == ' '||ch == '\t')getch();  // 删除前导空格

	if (isalpha(ch)){  // 当前token的第一个字符为字母，可能是标识符或保留字
		k = 0;	// 用于记录token长度
		do{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}while (isalpha(ch) || isdigit(ch));
		a[k] = 0; //读完整个token，添加结束符\0

		strcpy(id, a);
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));//compare with reserved words
		//如果能匹配到保留字，i+1即为保留字的类型,否则i=-1表示不是保留字,i.e是ID
		if (++i)
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   
	}

	else if (isdigit(ch)){ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do{
			num = num * 10 + ch - '0';
			k++; 
			getch();
		}while (isdigit(ch));

		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':'){ // symbol is either : or :=
		getch();
		if (ch == '='){
			sym = SYM_BECOMES; // :=
			getch();
		}
		else if(ch == ':'){
			sym = SYM_DOMAIN;// ::
				getch();
		}
		else 
			sym = SYM_NULL;// :       
	}
	else if (ch == '>'){ // symbol is either > or >=
		getch();
		if (ch == '='){
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
			sym = SYM_GTR;     // >
	}
	else if (ch == '<'){// symbol is either < or <= or <>
		getch();
		if (ch == '='){
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>'){
			sym = SYM_NEQ;     // <> 即!=
			getch();
		}
		else
			sym = SYM_LES;     // <
	}
	else{ // other tokens
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);//compare with other tokens
		if (++i){
			sym = ssym[i];//symbol is a special symbol
			getch();
		}
		else{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym


//中间代码生成,将生成的中间代码写入中间代码数组，供后面的解释器运行
void gen(int x, int y, int z){
	if (cx > CXMAX){//program too long
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

//-------------测试当前单词是否合法
//---------------------------------
// 参数：s1:当语法分析进入或退出某一语法单元时当前单词符合应属于的集合
//      s2:在某一出错状态下，可恢复语法分析正常工作的补充单词集合
//      n :出错信息编号，当当前符号不属于合法的 s1 集合时发出的出错信息
void test(symset s1, symset s2, int n){//test if error occurs
	symset s;

	if (! inset(sym, s1)){//if sym is not in s1
		error(n);
		s = uniteset(s1, s2);//s = s1 + s2
		while(! inset(sym, s))//skip all symbols that do not belongs to s
			getsym();//get a symbol
		destroyset(s);//destroy s
	}
} // test


int depth;

// enter object(constant, variable or procedre) into table.
//-------------符号表
void enter(int kind){
	mask* mk;//pointer to mask

	tx++;//table index + 1
	strcpy(table[tx].name, id);//save an identifier
	table[tx].kind = kind;    //save the kind of object
	table[tx].depth = depth;  //save the depth of object
	table[tx].domain = cur_domain; //save the domain of object

	switch (kind){
	case ID_CONSTANT: //constant
		if (num > MAXADDRESS){
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;//save the value of constant
		break;
	case ID_VARIABLE://variable
		mk = (mask*) &table[tx];//mk points to the table[tx]
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE://procedure
		mk = (mask*) &table[tx];//mk points to the table[tx]
		mk->level = level;
		break;
	case ID_ARRAY://array
		mk = (mask*) &table[tx];//mk points to the table[tx]
		mk->level = level;
		mk->address = arr_tx;//save the address of array
		last_arr = &(array_table[arr_tx]);//last_arr points to the array_table[arr_tx]
		array_table[arr_tx].dim = 0;//initialize the dimension of array
		array_table[arr_tx].size = 1;//initialize the size of array
		array_table[arr_tx].dim_size[1] = 0;//initialize the size of the first dimension of array
		arr_tx++;//array table index + 1
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table.
// @return : 0:not found, n: index of symbol table entry
int position(char* id){
	int i;
	strcpy(table[0].name, id); // sentinel
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);//compare with identifiers in table
	return i;
} // position

int domain_position(char* id, int tar_domain){
	int i;
	strcpy(table[0].name, id); // sentinel
	i = tx + 1;
	while ((strcmp(table[i].name, id) != 0)||(table[i].domain != tar_domain)) i--;
	return i;
}

int domain_search(int domain_index){
	// @paras: domain_index: table[domain_index]下搜索, default =0 代表main域
	// @return : 返回该"domain"表达式所在table 的index
		int i;
		getsym();
		int index;
		if (sym == SYM_IDENTIFIER){
			if ((i = domain_position(id,domain_index)) == 0)//if i = 0
				error(11); // Undeclared identifier.
			else if(table[i].kind == ID_VARIABLE)		
				index = domain_position(id,domain_index);
			else if(table[i].kind == ID_PROCEDURE){
				getsym();
				if(sym == SYM_DOMAIN){
					index = domain_search(i);
				}
				else error(21); // Procedure identifier can not be in an expression.
			}
		}
		else error(4);// There must be an identifier to follow 'const', 'var', or 'procedure'.
		return index;
}

//////////////////////////////////////////////////////////////////////
void constdeclaration(){
	if (sym == SYM_IDENTIFIER){
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES){//if sym is = or :=
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER){//if sym is a number
				enter(ID_CONSTANT);//enter a constant
				getsym();
			}
			else
				error(2); // There must be a number to follow '='.
		}
		else
			error(3); // There must be an '=' to follow the identifier.
	} else	error(4);// There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration
//////////////////////////////////////////////////////////////////////
void dimdeclaration(void){
	int i;
	if(sym == SYM_LSQUAREBRACKET){//if sym is [
		getsym();
		switch(sym){
			case SYM_NUMBER://if sym is a number
				if(last_arr->dim >= MAX_DIM)
					error(31);//The dimension of array must be less than 10.
				if(num <= 0)
					error(33);//The size of array must be greater than 0.
				last_arr->dim++;//dimension + 1
				last_arr->dim_size[last_arr->dim] = num;//save the size of the dimension
				last_arr->size *= num;//calculate the size of the array
				getsym();
				if(sym == SYM_RSQUAREBRACKET){//if sym is ]
					getsym();
					dimdeclaration();
				}
				else
					error(27);//Missing ']'.
				break;
			case SYM_IDENTIFIER://if sym is an identifier
				if(last_arr->dim >= MAX_DIM)
					error(31);//The dimension of array must be less than 10.
				i = position(id);//locate the identifier in table
				if(i == 0)
					error(11);//Undeclared identifier.
				if(table[i].kind != ID_CONSTANT)
					error(30);//Only constant identifier is allowed for array declaration.
				if(table[i].value <= 0)
					error(33);//The size of array must be greater than 0.
				last_arr->dim++;//dimension + 1
				last_arr->dim_size[last_arr->dim] = table[i].value;//save the size of the dimension
				last_arr->size *= table[i].value;//calculate the size of the array
				getsym();
				if(sym == SYM_RSQUAREBRACKET){//if sym is ]
					getsym();
					dimdeclaration();
				}
				else
					error(27);//Missing ']'.
				break;
			case SYM_RSQUAREBRACKET://if sym is ],a[][1]
				if(last_arr->dim!=0)
					error(39);//The empty dimension must be the first one.
				last_arr->dim++;//dimension + 1
				last_arr->dim_size[last_arr->dim] = 0;//save the size of the dimension
				last_arr->size *= 1;//calculate the size of the array
				getsym();
				dimdeclaration();
				break;
			default:
				error(26);//Must define the dimension size of an array.
				break;

		}
	}
	else {
		if(last_arr->dim_size[1]){//if the size of the first dimension is not 0
			dx += last_arr->size;//data allocation index + the size of the array
			last_arr->address = dx - 1;//save the address of the array
			printf("allocate for array  %d\n", dx-1);
		}
		else{//
			int cc_temp = cc;
			char ch_temp = ch;
			int sym_temp = sym;
			int cnt = 0;
			int dimsize = 0;
			int temp = 1;
			getsym();
			for(int i=1;i<=last_arr->dim;i++)
				dimsize += last_arr->dim_size[i];
			last_arr->size = dimsize;
			last_arr->dim_size[1] = -1;
			current_level = -1;
			array_index = 0;
			array_dim = last_arr->dim;
			max_level = array_dim;
			array_full = 0;
			before_Lbracket = 0;
			bl_index = 0;
			for(int i=1;i<array_dim;i++){
				current_array[i] = 0;
				max_array[i] = last_arr->dim_size[i]-1;
				bracketlevel[i] = 0;
			}
			if(last_arr->dim_size[1]==-1)
				max_array[1]=5;//default size of the first dimension
			for(int i=array_dim;i>0;i--){
				judge_level[i]=temp;
				temp *= last_arr->dim_size[i];//calculate the size of the dimension
			}
			current_array[array_dim] = -1;
			mask* mk;
			mk = (mask*) &table[tx];
			countsize();
			cc = cc_temp;
			ch = ch_temp;
			sym = sym_temp;
		}
	}
}
//////////////////////////////////////////////////////////////////////
int levelindex(void){//get the level of the identifier by its index
	for(int i=1;i<=array_dim;i++){
		if(array_index % judge_level[i] == 0)//if array_index is a multiple of judge_level[i]
			return i;
	}
}
//////////////////////////////////////////////////////////////////////
void initializer_list1(void){//initialize the array
	if (sym == SYM_COMMA) {//if sym is ,
		getsym();
		initializer();
		getsym();
		initializer_list1();
	}
}
//////////////////////////////////////////////////////////////////////  
void initializer_list(void){//initialize the array
	if (sym == SYM_NUMBER || sym == SYM_LBRACKET) {//if sym is a number or [
		initializer();
		getsym();
		initializer_list1();
	}
}
//////////////////////////////////////////////////////////////////////
void initializer(void){
	if(sym == SYM_NUMBER){
		int i;
		before_Lbracket = 0;
		for (i = array_dim;i > 0;i--) {//for each dimension
			if (current_array[i] < max_array[i]) {
				current_array[i]++;
				break;
			}
			if (current_array[i] == max_array[i]) {
				current_array[i] = 0;//reset the current_array[i]
			}
		}
		if (!i) {
			error(37);//Too many initializers.
			return;
		}
		gen(LIT, 0, array_index++);
		gen(LIT, 0, num);
		gen(STI, 0, 0);
	}
	else if(sym == SYM_LBRACKET){
		if(current_level == -1) current_level=0;
		else {
			current_level = (current_level+1)>levelindex()?current_level+1:levelindex();
			before_Lbracket = 1;//before_Lbracket = 1 means that the identifier is before [
			bracketlevel[++bl_index] = current_level;//save the level of the [
		}
		if(current_level>max_level){
			error(37);//Too many initializers.
			return;
		}
		getsym();
		initializer_list();
		if(sym == SYM_COMMA)getsym();
		if(sym == SYM_RBRACKET){
			if (current_level == max_level) {
				if (before_Lbracket) {
					gen(LIT, 0, array_index++);
					gen(LIT, 0, 0);
					gen(STI, 0, 0);
				}
				before_Lbracket = 0;
				current_level = bracketlevel[--bl_index];
			}
			else if (current_level == 0&& last_arr->dim_size[1] == -1) {
				int k, offset_count, array_index_after = 0;
				if (array_index == 0) {
					offset_count = judge_level[1];
					for (int offset = 0;offset < offset_count;offset++) {
						gen(LIT, 0, array_index + offset);
						gen(LIT, 0, 0);
						gen(STI, 0, 0);
					}
				}
				else {
					for (k = 2;k < array_dim + 1;k++) {
						current_array[k] = max_array[k];
					}
					for (int m = 1;m < array_dim + 1;m++) {
						array_index_after = array_index_after * (max_array[m] + 1) + current_array[m];
					}
					array_index_after++;
					offset_count = array_index_after - array_index;
					for (int offset = 0;offset < offset_count;offset++) {
						gen(LIT, 0, array_index + offset);
						gen(LIT, 0, 0);
						gen(STI, 0, 0);
					}
					array_index = array_index_after;
					last_arr->dim_size[1] = array_index / judge_level[1];
					last_arr->size = array_index;
					dx += last_arr->size;
					last_arr->address = dx - 1;
					//printf("allocate for array: %d\n", dx-1);
				}
			}
			else {
				int k, offset_count, array_index_after = 0;
				for (k = array_dim;k > current_level;k--) {
					current_array[k] = max_array[k];
				}
				for (int m = 1;m < array_dim + 1;m++) {
					array_index_after = array_index_after * (max_array[m] + 1) + current_array[m];
				}
				array_index_after++;
				offset_count = array_index_after - array_index;
				for (int offset = 0;offset < offset_count;offset++) {
					gen(LIT, 0, array_index + offset);
					gen(LIT, 0, 0);
					gen(STI, 0, 0);
				}
				before_Lbracket = 0;
				array_index = array_index_after;
				current_level = bracketlevel[--bl_index];
			}
			return;
		}
		else error(38);//Missing ']'.
	}
	else error(36);//Missing initializer.
}
//////////////////////////////////////////////////////////////////////
void countsize_list1(void){
	if (sym == SYM_COMMA) {
		getsym();
		countsize();
		getsym();
		countsize_list1();
	}
}

void countsize_list(void){
	if (sym == SYM_NUMBER || sym == SYM_LBRACKET) {
		countsize();
		getsym();
		countsize_list1();
	}
}
//////////////////////////////////////////////////////////////////////
void countsize(void){//calculate the size of the array
	if(sym == SYM_NUMBER){
		int i;
		before_Lbracket = 0;
		for (i = array_dim;i > 0;i--) {
			if (current_array[i] < max_array[i]) {
				current_array[i]++;
				break;
			}
			if (current_array[i] == max_array[i]) {
				current_array[i] = 0;
			}
		}
		array_index++;
	}
	else if(sym == SYM_LBRACKET){
		if(current_level == -1) current_level=0;
		else {
			current_level = current_level+1 > levelindex()?current_level+1:levelindex();
			before_Lbracket = 1;
			bracketlevel[++bl_index] = current_level;
		}
		getsym();
		countsize_list();
		if(sym == SYM_COMMA)getsym();
		if(sym == SYM_RBRACKET){
			if (current_level == max_level) {
				if (before_Lbracket) {
					array_index++;
				}
				before_Lbracket = 0;
				current_level = bracketlevel[--bl_index];
			}
			else if (current_level == 0&& last_arr->dim_size[1] == -1) {
				int k, offset_count, array_index_after = 0;
				if (array_index == 0) {
					offset_count = judge_level[1];
				}
				else {
					for (k = 2;k < array_dim + 1;k++) {
						current_array[k] = max_array[k];
					}
					for (int m = 1;m < array_dim + 1;m++) {
						array_index_after = array_index_after * (max_array[m] + 1) + current_array[m];
					}
					array_index_after++;
					offset_count = array_index_after - array_index;
					for (int offset = 0;offset < offset_count;offset++) {
						array_index + offset;
					}
					array_index = array_index_after;
					last_arr->dim_size[1] = array_index / judge_level[1];
					last_arr->size = array_index;
					dx += last_arr->size;
					last_arr->address = dx - 1;
					//printf("allocate for array: %d\n", dx-1);
				}
			}
			else {
				int k, offset_count, array_index_after = 0;
				for (k = array_dim;k > current_level;k--) {
					current_array[k] = max_array[k];
				}
				for (int m = 1;m < array_dim + 1;m++) {
					array_index_after = array_index_after * (max_array[m] + 1) + current_array[m];
				}
				array_index_after++;
				offset_count = array_index_after - array_index;
				before_Lbracket = 0;
				array_index = array_index_after;
				current_level = bracketlevel[--bl_index];
			}
			return;
		}
		else error(38);//Missing ']'.
	}
	else error(36);//Missing initializer.
}
//////////////////////////////////////////////////////////////////////
void vardeclaration(void){//variable declaration
	if (sym == SYM_IDENTIFIER){
		getsym();
		if(sym==SYM_LSQUAREBRACKET){
			enter(ID_ARRAY);//enter an array
			dimdeclaration();
			if(sym==SYM_EQU){
				getsym();
				if(sym==SYM_LBRACKET){
					current_level = -1;
					array_index = 0;
					array_dim = last_arr->dim;
					max_level = array_dim;
					array_full = 0;
					before_Lbracket = 0;
					bl_index = 0;
					for(int i=1;i<array_dim+1;i++){
						current_array[i] = 0;
						max_array[i] = last_arr->dim_size[i]-1;
						bracketlevel[i] = 0;
					}
					if(last_arr->dim_size[1]==-1)
						max_array[1]=5;//default size of the first dimension
					int temp = 1;
					for(int i=array_dim;i>0;i--){
						judge_level[i]=temp;
						temp *= last_arr->dim_size[i];//calculate the size of the dimension
					}
					current_array[array_dim] = -1;
					mask* mk;
					mk = (mask*) &table[tx];
					gen(RES,0,0);
					gen(LEA,level-mk->level,last_arr->address);
					initializer();
					gen(RET,0,0);
					getsym();
				}
			}
		}
		else enter(ID_VARIABLE);//enter a variable
	}
	else if (sym == SYM_TIMES) { // 指针
		depth = 0;
		do {
			++depth;
			getsym();
		} while (sym == SYM_TIMES);
		vardeclaration(); // 有可能是数组
		depth = 0; // 结束一个指针变量/数组
	}
	else 
		error(4); // There must be an identifier to follow 'const', 'var', a string of '*' or 'procedure'.
} // vardeclaration
//////////////////////////////////////////////////////////////////////
void array_visit(short arr_index, int dim, symset fsys) {
	getsym();
	if (sym == SYM_LSQUAREBRACKET) {
		gen(LIT, 0, array_table[arr_index].dim_size[dim + 1]);
		gen(OPR, 0, OPR_MUL);//multiply top two
		getsym();
		expression(fsys);
		gen(OPR, 0, OPR_ADD);
		array_visit(arr_index, dim + 1, fsys);//visit next dimension 
	}
	else if (dim != array_table[arr_index].dim)  error(34);//missing dimensions
}

int pointer_visit(int cannotID_VARIABLE, symset fsys) { // 出现指针运算，递归处理可能出现的多层解引用
	getsym();
	if (sym == SYM_TIMES) {
		getsym();
		if (sym == SYM_TIMES || sym == SYM_IDENTIFIER) {
			int dim = pointer_visit(cannotID_VARIABLE, fsys);
			gen(LDA, 0, 0);
			getsym();
			return dim + 1;
		} else if (sym == SYM_LPAREN) {
			getsym();
			int dim = pointer_visit(cannotID_VARIABLE, fsys);
			if (sym == SYM_PLUS) {
				getsym();
				if (sym == SYM_NUMBER) gen(LIT, 0, num);
				else error(41);
			} else error(42);
			getsym();
			if (sym == SYM_RPAREN) getsym();
			else error(22);
		}
	} else if (sym == SYM_IDENTIFIER) {
		int i;
		if ((i = position(id)) == 0)//if i = 0
			error(11); // Undeclared identifier.
		else {
			if (table[i].kind == ID_VARIABLE) {

			} else if (table[i].kind == ID_ARRAY) {
				
			}
		}
	}
}
//-------------输出当前代码块的中间代码
//---------------------------------
void listcode(int from, int to){
	int i;
	printf("\n");
	for (i = from; i < to; i++)//for each code
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);//print the code
	
	printf("\n");
} // listcode

//-------------factor处理 // Todo : 加入指针解引用
//---------------------------------
//fsys: 如果出错可用来恢复语法分析的符号集合
void factor(symset fsys){
	void expression(symset fsys);//declare expression
	int i,arr_index;
	symset set;//declare set
	
	test(facbegsys, fsys, 24);   
                                  
	if (inset(sym, facbegsys)){
		if (sym == SYM_TIMES) {// 右值是解引用指针
			pointer_visit(0, fsys); // todo
		}
		if (sym == SYM_IDENTIFIER){
			if ((i = position(id)) == 0)//if i = 0
				error(11); // Undeclared identifier.
			else{
				switch (table[i].kind){//switch the kind of identifier
					mask* mk;
				case ID_CONSTANT:
					gen(LIT, 0, table[i].value);
					getsym();
					break;
				case ID_VARIABLE:
					mk = (mask*) &table[i];
					gen(LOD, level - mk->level, mk->address);
					getsym();
					break;
				case ID_PROCEDURE:
					getsym();
					if(sym == SYM_DOMAIN){
						int idx = domain_search(i);//
						mk = (mask*) &table[idx];
						gen(LOD, level - mk->level, mk->address);
						getsym();
					break;
					}
					else error(21); // Procedure identifier can not be in an expression.
					break;
				case ID_ARRAY:
					mk = (mask*) &table[i];
					arr_index = mk->address;
					mk = (mask*) &table[i];
					gen(LEA, level - mk->level,array_table[arr_index].address);
					gen(LIT, 0, 0);//initialize the index of array
					set = createset(SYM_RSQUAREBRACKET);
					array_visit(arr_index,0,set);//visit the array
					gen(OPR,0,OPR_MIN);
					gen(LDA,0,0);//load the address of the array
					break;
				} // switch
			}
		}
		else if(sym == SYM_DOMAIN){
						mask* mk;
						int idx = domain_search(i);//
						mk = (mask*) &table[idx];
						gen(LOD, level - mk->level, mk->address);
						getsym();
			}
		else if (sym == SYM_NUMBER){
			if (num > MAXADDRESS){
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);//generate a LIT instruction
			getsym();
		}
		else if (sym == SYM_LPAREN){//if sym is (
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);//set = {SYM_RPAREN} + fsys
			expression(set);
			destroyset(set);
			if (sym == SYM_RPAREN)//if sym is )
				getsym();
			else
				error(22); // Missing ')'.
		}
		else if(sym == SYM_MINUS){ // UMINUS,  Expr -> '-' Expr
			 getsym();
			 factor(fsys);
			 gen(OPR, 0, OPR_NEG);//generate a OPR instruction
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);  // 一个因子处理完毕，遇到的 token 应在 fsys 集合中
	} // if
} // factor

//////////////////////////////////////////////////////////////////////
void term(symset fsys){
	int mulop;
	symset set;
	
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));//set = fsys + {SYM_TIMES, SYM_SLASH}
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH){//if sym is * or /
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)//if mulop is *
			gen(OPR, 0, OPR_MUL);
		else
			gen(OPR, 0, OPR_DIV);
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
void expression(symset fsys){
	int i, addop;
	symset set, set1;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));//set = fsys + {SYM_PLUS, SYM_MINUS}
	
	if (sym == SYM_QUOTE) { // 对变量取地址
		getsym();
		if (sym == SYM_NUMBER){ // 对数字取地址
			error(40); // Illegal address operation.
		} else if (sym == SYM_IDENTIFIER) {
			mask* mk;
			if (! (i = position(id)))
				error(11); // Undeclared identifier.
			else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY) {
				//if the kind of identifier is not ID_VARIABLE
				error(12); // Illegal assignment.
				i = 0;
			}
			if(table[i].kind==ID_VARIABLE){
				mask* mk;
				mk = (mask*) &table[i];
				// 取地址，LOD指令改为LEA指令，保留地址信息即可
				gen(LEA, level - mk->level, mk->address);
				getsym();
			}
			else if(table[i].kind == ID_ARRAY){
				int arr_index;
				mask* mk;
				mk = (mask*) &table[i];
				array_index = mk->address;
				mk = (mask*) &table[i];
				gen(LEA, level - mk->level, array_table[array_index].address);
				gen(LIT, 0, 0);//initialize the index of array
				set1=createset(SYM_RSQUAREBRACKET);
				array_visit(array_index,0,set1);//visit the array
				gen(OPR,0,OPR_MIN);
				//取地址，所以最后不用加载地址内的值，保留地址即可
				//gen(LDA,0,0);//load the address of the array
			}
		}
	}
	else {
		term(set);
		while (sym == SYM_PLUS || sym == SYM_MINUS){
			addop = sym;
			getsym();
			term(set);
			if (addop == SYM_PLUS)//if addop is +
				gen(OPR, 0, OPR_ADD);
			else
				gen(OPR, 0, OPR_MIN);
		} // while
	}
	destroyset(set);
} // expression

//////////////////////////////////////////////////////////////////////
void condition(symset fsys){
	int relop;
	symset set;

	if (sym == SYM_ODD){//if sym is odd
		getsym();
		expression(fsys);
		gen(OPR, 0, 6);
	}
	else{
		set = uniteset(relset, fsys);
		expression(set);
		destroyset(set);
		if (! inset(sym, relset))//if sym is not in relset
			error(20);
		else{
			relop = sym;
			getsym();
			expression(fsys);
			switch (relop){
			case SYM_EQU://if relop is =
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ://if relop is <>
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES://if relop is <
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ://if relop is >=
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR://if relop is >
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ://if relop is <=
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition

//////////////////////////////////////////////////////////////////////
void statement(symset fsys){
	int i, cx1, cx2;
	symset set1, set;
	int count = 0;

	if (sym == SYM_TIMES) { // 解引用，可能有数组的指针形式
		mask* mk;
		
	}
	else if (sym == SYM_IDENTIFIER){ // variable assignment
		mask* mk;
		if (! (i = position(id)))
			error(11); // Undeclared identifier.
		else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY){
			//if the kind of identifier is not ID_VARIABLE or ID_ARRAY
			error(12); // Illegal assignment.
			i = 0;
		}
		if(table[i].kind==ID_VARIABLE){
			getsym();
			if (sym == SYM_BECOMES)//if sym is :=
				getsym();
			else
				error(13); // ':=' expected.
			expression(fsys);
			mk = (mask*) &table[i];
			if (i)//if i != 0
				gen(STO, level - mk->level, mk->address);
		}
		else if(table[i].kind == ID_ARRAY){
			mk = (mask*) &table[i];
			array_index = mk->address;
			gen(LEA, level - mk->level, array_table[array_index].address);
			gen(LIT, 0, 0);//initialize the index of array
			set1=createset(SYM_RSQUAREBRACKET);
			array_visit(array_index,0,set1);//visit the array
			if(sym!=SYM_BECOMES)error(13);//':=' expected.
			gen(OPR,0,OPR_MIN);
			getsym();
			expression(fsys);
			if(i)gen(STA,0,0);//store the value of expression
		}
	}
	else if (sym == SYM_CALL){ // procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)
			error(14); // There must be an identifier to follow the 'call'.
		else{
			if (! (i = position(id)))
				error(11); // Undeclared identifier.
			else if (table[i].kind == ID_PROCEDURE){
				mask* mk;
				mk = (mask*) &table[i];
				gen(CAL, level - mk->level, mk->address);
			}
			else
				error(15); // A constant or variable can not be called. 
			getsym();
		}
	} 
	else if (sym == SYM_IF){ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)//if sym is then
			getsym();
		else
			error(16); // 'then' expected.
		cx1 = cx;
		gen(JPC, 0, 0);
		statement(fsys);
		code[cx1].a = cx;
	}
	else if (sym == SYM_BEGIN){ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys)){//if sym is ; or in statbegsys
			if (sym == SYM_SEMICOLON)//if sym is ;
				getsym();
			else
				error(10); // ';' expected.
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)//if sym is end
			getsym();
		else
			error(17); // ';' or 'end' expected.
	}
	else if (sym == SYM_WHILE){ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);//set1 = {SYM_DO}
		set = uniteset(set1, fsys);//set = {SYM_DO} + fsys
		condition(set);
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
			getsym();
		else
			error(18); // 'do' expected.
		statement(fsys);
		gen(JMP, 0, cx1);
		code[cx2].a = cx;
	}
	else if(sym==SYM_PRINT){
		getsym();
		if(sym!=SYM_LPAREN)error(28);
		while (1){
			getsym();
			if(sym==SYM_RPAREN)break;
			else if(sym==SYM_IDENTIFIER){
				count++;
				if (! (i = position(id)))
					error(11); // Undeclared identifier.
				else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY
							&&table[i].kind != ID_CONSTANT&&table[i].kind != ID_PROCEDURE)
					error(29);//Illegal print.
				else { // 
					if(table[i].kind==ID_VARIABLE){
						mask* mk;
						mk = (mask*) &table[i];
						gen(LOD, level - mk->level, mk->address);
						getsym();
					}
					else if(table[i].kind == ID_CONSTANT){
						gen(LIT, 0, table[i].value);
						getsym();
					}
					else if(table[i].kind == ID_PROCEDURE){
						getsym();
						if(sym != SYM_DOMAIN) error(21);

						int idx = domain_search(i);
						mask* mk;
						mk = (mask*) &table[idx];
						gen(LOD, level - mk->level, mk->address);
						getsym();
					}
					else if(table[i].kind == ID_ARRAY){
						int arr_index;
						mask* mk;
						mk = (mask*) &table[i];
						array_index = mk->address;
						mk = (mask*) &table[i];
						gen(LEA, level - mk->level, array_table[array_index].address);
						gen(LIT, 0, 0);//initialize the index of array
						set1=createset(SYM_RSQUAREBRACKET);
						array_visit(array_index,0,set1);//visit the array
						gen(OPR,0,OPR_MIN);
						gen(LDA,0,0);//load the address of the array
					}
					if(sym==SYM_COMMA)continue;
					else if(sym==SYM_RPAREN)break;
					else error(28);
				}
			}
			else if(sym == SYM_DOMAIN){
				count++;
				int idx = domain_search(0);
				mask* mk;
				mk = (mask*) &table[idx];
				gen(LOD, level - mk->level, mk->address);	
				getsym();
				if(sym==SYM_COMMA)continue;
					else if(sym==SYM_RPAREN)break;
					else error(28);
			}
			else {
				if(sym==SYM_NUMBER)
					error(29);//Illegal print.
				else error(28);
			}
		}
		gen(PRT,0,count);
		getsym();
	}
	test(fsys, phi, 19);
} // statement
			
//////////////////////////////////////////////////////////////////////
void block(symset fsys){
	int cx0; // initial code index
	mask* mk;
	
	int savedTx;//saved table index
	symset set1, set;

	dx = 3;
	// 地址寄存器给出每层局部量当前已分配到的相对位置
    // 置初始值为 3 的原因是：每一层最开始的位置有三个空间用于存放
    // 静态链 SL、动态链 DL 和 返回地址 RA
	int block_dx = dx; 
	int block_domain = cur_domain;

	mk = (mask*) &table[tx]; 
	mk->address = cx;
	gen(JMP, 0, 0); // block开始时首先写下一句跳转指令，地址到后面再补

	if (level > MAXLEVEL) error(32); // There are too many levels.

	do{
		if (sym == SYM_CONST){ // constant declarations
			getsym();
			do{
				constdeclaration();
				while (sym == SYM_COMMA){//if sym is ,
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)//if sym is ;
					getsym();
				else
					error(5); // Missing ',' or ';'.
				
			}
			while (sym == SYM_IDENTIFIER);
		} // if

		if (sym == SYM_VAR){ // variable declarations
			getsym();
			do{
				vardeclaration();
				while (sym == SYM_COMMA){//if sym is ,
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
					getsym();
				else
					error(5); // Missing ',' or ';'.
			}while (sym == SYM_IDENTIFIER);//if sym is identifier
		} // if

		block_dx = dx; //save dx before handling procedure call!
		
		while (sym == SYM_PROCEDURE){ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER){
				enter(ID_PROCEDURE);
				cur_domain = position(id);
				getsym();
			}
			else
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			
			if (sym == SYM_SEMICOLON)
				getsym();
			else
				error(5); // Missing ',' or ';'.

			level++;
			savedTx = tx; //保存table index便于回溯
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
		
			block(set);
			destroyset(set1);
			destroyset(set);
			tx = savedTx;
			level--;
			cur_domain = block_domain;

			if (sym == SYM_SEMICOLON){
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
				error(5); // Missing ',' or ';'.
		} // while
		dx = block_dx; //restore dx after handling procedure call!

		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);//set = statbegsys + {SYM_IDENTIFIER}
		//test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}  while (inset(sym, declbegsys));

	code[mk->address].a = cx;  //
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);

} // block

//-------------通过静态链求出数据区基地址
int base(int stack[], int currentLevel, int levelDiff){//`
	int b = currentLevel;
	
	while (levelDiff--)   //while levelDiff > 0
		b = stack[b];    //b points to the base of the current level
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret(){
	int pc;        // program counter
	int stack[STACKSIZE];
	int top;       // top of stack
	int b;         // program, base, and top-stack register
	instruction i; // instruction register

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1; 
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	do{
		i = code[pc++];
		switch (i.f){
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a){ // operator
			case OPR_RET://return
				top = b - 1;
				pc = stack[top + 3];
				b = stack[top + 2];
				break;
			case OPR_NEG://negation
				stack[top] = -stack[top];
				break;
			case OPR_ADD://addition
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN://subtraction
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL://multiplication
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV://division
				top--;
				if (stack[top + 1] == 0){//if divided by zero
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD://odd
				stack[top] %= 2;
				break;
			case OPR_EQU://equal
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ://not equal
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES://less
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			} // switch
			break;
		case LEA:
			stack[++top] = base(stack, b, i.l) + i.a;//load the address of the array
			break;
		case LDA:
			stack[top] = stack[stack[top]];//load the value of the array
			break;
		case LOD://load
			stack[++top] = stack[base(stack, b, i.l) + i.a];//load a value to the top of stack
			break;
		case STO://store
			stack[base(stack, b, i.l) + i.a] = stack[top];//store a value to the top of stack
			//printf("%d\n", stack[top]);//print the value
			top--;
			break;
		case STA:
			stack[stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case STI:
			stack[stack[top - 2] - stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case CAL:
			stack[top + 1] = base(stack, b, i.l);
			// generate new block mark
			stack[top + 2] = b;//stack[top + 2] points to the base of the new block
			stack[top + 3] = pc;//stack[top + 3] points to the return address
			b = top + 1;
			pc = i.a;//pc points to the beginning address of the called procedure
			break;
		case INT:
			top += i.a;//allocate i.a space for the new block
			break;
		case RES:
			stack[STACKSIZE - 10] = top;
			top = STACKSIZE - 10;
			break;
		case RET:
			top = stack[top - 1];
			break;
		case JMP://jump
			pc = i.a;//pc points to the address of the jump
			break;
		case JPC://jump if false
			if (stack[top] == 0)//if stack[top] is 0
				pc = i.a;//pc points to the address of the jump
			top--;//pop stack[top]
			break;
		case PRT:
			if (i.a == 0) { printf("\n"); }//print a new line
			else {
				for (int k = i.a - 1;k >= 0;k--) {
					printf("%d ", stack[top - k]);//print the value
				}
				top = top - i.a;//pop i.a values
			}
			printf("\n");
			break;
		} // switch
	}while (pc);
	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
int main (){
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL){
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_NULL);
	facbegsys = createset(SYM_DOMAIN ,SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	cur_domain = 0;
	ch = ' ';
	kk = MAXIDLEN;
	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);
	block(set);
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)//if sym is not .
		error(9); // '.' expected.
	if (err == 0){//if there is no error
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fprintf(hbin, "%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	//listcode(0, cx);

	return 0;
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
