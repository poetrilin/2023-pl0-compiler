## 一、设计任务
 
### 1.1程序实现要求

PL/0语言可以看成PASCAL语言的子集，它的编译程序是一个编译解释执行系统。PL/0的目标程序为假想栈式计算机的汇编语言，与具体计算机无关。


其编译过程采用一趟扫描方式，以语法分析程序为核心，词法分析和代码生成程序都作为一个独立的过程，当语法分析需要读单词时就调用词法分析程序，而当语法分析正确需要生成相应的目标代码时，则调用代码生成程序。

用表格管理程序建立变量、常量和过程标示符的说明与引用之间的信息联系。

用出错处理程序对词法和语法分析遇到的错误给出在源程序中出错的位置和错误性质。

当源程序编译正确时，PL/0编译程序自动调用解释执行程序，对目标代码进行解释执行，并按用户程序的要求输入数据和输出运行结果

## 文法

Program → Block .

 Block → [ConstDecl] [VarDecl][ProcDecl] Stmt

 ConstDecl → **const** ConstDef {, ConstDef} ;

 ConstDef → **ident** = **number**

 VarDecl → **var** **ident** {, **ident**} ;

 ProcDecl → **procedure** **ident** ; Block ; {**procedure** **ident** ; Block ;}

 Stmt → **ident** := Exp | **call** **ident** | **begin** Stmt {; Stmt} end |
**if** Cond **then** Stmt | **while** Cond **do** Stmt | ε
 Cond → **odd** Exp | Exp RelOp Exp

 RelOp → = | <> | < | > | <= | >=

 Exp → [+ | − ] Term {+ Term | − Term}

 Term → Factor {∗ Factor | / Factor}

 Factor → **ident** | **number** | ( Exp )

其中的标识符 **ident** 是字母开头的字母数字串，**number** 是无符号整数，begin、**call**、const、
do、end、if、odd、**procedure**、**then**、var、while 是保留字。

## ISA

1. LIT (Literal): 将常量加载到栈顶

- 格式: LIT 0, A
- 功能: 将常量 A 放入栈顶
2. OPR (Operation): 执行运算或逻辑操作

- 格式: OPR 0, A
- 功能: 根据操作码 A 执行相应的运算或逻辑操作
3. LOD (Load): 将变量加载到栈顶

- 格式: LOD L, A
- 功能: 将位于静态链 L 层上的偏移地址为 A 的变量加载到栈顶
4. STO (Store): 将栈顶元素存储到变量

- 格式: STO L, A
- 功能: 将栈顶元素存储到静态链 L 层上的偏移地址为 A 的变量
5. CAL (Call): 调用过程

- 格式: CAL L, A
- 功能: 调用静态链 L 层上的过程，A 为过程入口地址
6. INT (Increment): 分配内存空间

- 格式: INT 0, A
- 功能: 数据栈栈顶指针增加a,在栈顶为当前过程分配 A 个内存单元
7. JMP (Jump): 无条件跳转

- 格式: JMP 0, A
- 功能: 无条件跳转到指令地址 A
8. JPC (Jump Condition): 条件跳转

- 格式: JPC 0, A
- 功能: 如果栈顶元素为零，则跳转到指令地址 A

TODO: 可能待添加的操作:

RED L ，a 读数据并存入变量（相对地址为a，层次差为L）

WRT 0 ，0 将栈顶内容输出

**代码的具体形式：**

`code: f l a`

（1）操作码 f：上面已经列出了所有 8 种操作码。

（2）层次差 l：这里的层次差就是 5.3.2 节介绍嵌套深度时的 np − na。该域仅用于存取指令和调用指令。

（3）多用途 a：在运算指令中，a 的值用来区分不同的运算；在其他情况，a 或是一个
数（lit，int），或是一个程序地址（jmp，jpc，cal），或是一个数据地址（lod，sto）。



编译器对 PL/0 源程序进行一遍扫描，并逐行输出源程序。在源程序无错的情况下，编
译器每编译完一个分程序，就列出该分程序的代码，这由编译器的 listcode 过程完成。

每个分程序的第一条指令是 jmp 指令，其作用是绕过该分程序声明部分产生的代码（即绕过内嵌过程的代码）。listcode 过程没有列出这条代码。

解释器是编译器中的一个过程，若源程序无错，则编译结束时调用解释过程 interpret。

由于 PL/0 语言没有输出语句，解释器按执行次序，每遇到对变量赋值时就输出该值。

由于 PL/0 语言是过程嵌套语言，因此程序运行时，活动记录栈中每个活动记录需要包
含控制链和访问链，并按 5.3.2 节所讲的方法来建立访问链。

活动记录栈的栈顶以外的存储空间作为代码执行过程中所需要的计算栈，无需另外设立
计算栈。

### 编译表
编译表的条目结构用于存储编译器在处理源代码时收集的信息，这些信息包括标识符、常量、过程等在编译过程中的属性和特征。编译表的条目通常包含以下信息：

- 名字（Name）： 存储标识符、常量或过程的名称。

- 类型（Kind）： 指示条目所代表的是常量、变量还是过程。可能的取值包括 ID_CONSTANT、ID_VARIABLE 和 ID_PROCEDURE。

- 值（Value）： 如果是常量，存储其具体的数值。

- 层次（Level）： 用于指示标识符所在的嵌套层次。对于过程，这通常表示其嵌套的深度。

- 地址（Address）： 存储标识符在运行时栈或内存中的地址。

