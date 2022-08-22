PROJ = rpn
PATH := /usr/local/opt/bison/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/sh
YACC = bison # v3 required

all: $(PROJ)

$(PROJ): $(PROJ).tab.o lex.yy.o
	$(CC) -o $(PROJ) *.o

%.tab.o: %.tab.c

%.tab.c %.tab.h: %.y
	$(YACC) -d $<

lex.yy.o: lex.yy.c rpn.tab.h

lex.yy.c: rpn.l
	flex $<

clean:
	rm -f *.o
	rm -f *.tab.c *.tab.h
	rm -f *.yy.c
	rm -f $(PROJ)

.PHONY: clean
