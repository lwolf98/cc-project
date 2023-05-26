CC=gcc
CFLAGS=-O0 -ggdb3 -Wall
OBJ=ast.o list.o strop.o evaluation.o stack.o debug.o error.o lang.tab.o lex.yy.o

lang: $(OBJ)
	$(CC) -o $@ $^ -lm

lang.tab.c lang.tab.h: lang.y
	bison --defines -Wcounterexamples lang.y

lang.tab.o: lang.tab.c lang.tab.h

lex.yy.c: lang.l
	flex lang.l

lex.yy.o: lex.yy.c lang.tab.h

clean:
	rm -f lang.tab.* lex.yy.* lang $(OBJ)