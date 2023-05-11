PROJ = rpn
PATH := /usr/local/opt/bison/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/sh
YACC = bison # v3 required

all: $(PROJ)

$(PROJ): $(PROJ).tab.o lex.yy.o
	$(CC) -o $(PROJ) *.o -lm

%.tab.o: %.tab.c

%.tab.c %.tab.h: %.y
	$(YACC) -d $<

lex.yy.o: lex.yy.c rpn.tab.h ast.o

ast.o: astring.o

lex.yy.c: rpn.l
	flex $<

test: testAst test2.ast
	./testAst test2.ast

testAst: testAst.o

clean:
	rm -f *.o
	rm -f *.tab.c *.tab.h
	rm -f *.yy.c
	rm -f $(PROJ)

.PHONY: clean test all
