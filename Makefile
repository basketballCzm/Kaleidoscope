cc = clang++
prom = toy
obj =  Error.o Lexer.o  Parser.o  Codegen.o toy.o
llvm_config_include = $(shell llvm-config --cxxflags)
llvm_config_lib = $(shell llvm-config --ldflags --libs)

$(prom): $(obj)
	$(cc) -o $(prom)  $(obj) $(llvm_config_lib) -lpthread -lncurses

Error.o:Error.cpp AST.h
	$(cc) $(llvm_config_include)  -c Error.cpp 

Lexer.o:Lexer.cpp Error.h
	$(cc) $(llvm_config_include)  -c Lexer.cpp 

Parser.o:Parser.cpp Lexer.h Error.h AST.h
	$(cc) $(llvm_config_include) -c Parser.cpp 

Codegen.o:Codegen.cpp Error.h  AST.h
	$(cc) $(llvm_config_include) -c Codegen.cpp 

toy.o:toy.cpp Error.h  Lexer.h Parser.h Codegen.h AST.h
	$(cc) $(llvm_config_include) -c toy.cpp


clean: 
	rm -rf *.o

