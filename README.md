# Kaleidoscope
Kaleidoscope 
# 报错 
```c
toy.o：在函数‘main’中：
toy.cpp:(.text+0xa0)：对‘getNextToken()’未定义的引用

在编译toy.cpp时会报出这种警告，在链接阶段会直接报错
2.h:2:12: warning: ‘int hello(int)’ used but never defined
 static int hello(int a);
```
这里的报错是因为getNextToken()函数的原型为`static int getNextToken();` 这里static修饰一个函数，表示这个函数的生命周期为整个程序，可见性为当前Parser.cpp。因此在链接阶段这里会报错，链接不到该函数。
