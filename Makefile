all:
	gcc src/main.c src/lexer/lexer.c src/parser/parser.c src/code_gen/codegen.c src/symbol_table/symbol_table.c -o cumpileher

run:
	./cumpileher input.c