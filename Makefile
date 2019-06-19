cc = clang++
prom = toy
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.cpp")
obj = $(src:%.cpp=%.o)
llvm_config_include = $(shell llvm-config --cxxflags)
llvm_config_lib = $(shell llvm-config --ldflags --libs)

$(prom): Error.o Codegen.o Parser.o Lexer.o toy.o
	$(cc) -o $(prom) Error.o Codegen.o Parser.o Lexer.o toy.o $(llvm_config_lib) -lpthread -lncurses

%.o: %.cpp $(deps)
	$(cc) -c  $(llvm_config_include) $< -o $@

clean: 
	rm -rf $(obj) $(prom)

