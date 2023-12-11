#ifndef SET_H
#define SET_H

typedef struct snode
{
	int elem;
	struct snode* next;
} snode, *symset;

symset phi;// 空集
symset declbegsys;// 声明开始符号集合 
symset statbegsys; // 语句开始符号集合
symset facbegsys;// 因子开始符号集合
symset  relset;// 关系运算符集合


symset createset(int data, .../* SYM_NULL */);
void destroyset(symset s);
symset uniteset(symset s1, symset s2);
int inset(int elem, symset s);

#endif
// EOF set.h
