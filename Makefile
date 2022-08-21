PROJ = rpn
PATH := /usr/local/opt/bison/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/sh
YACC = bison # v3 required

all: $(PROJ)

$(PROJ): $(PROJ).tab.o lex.o
	$(CC) -o $(PROJ) *.o

%.tab.o: %.tab.c

%.tab.c %.tab.h: %.y
	$(YACC) -d $<

lex.o: lex.c rpn.tab.h

clean:
	rm -f *.o
	rm -f *.tab.c *.tab.h
	rm -f $(PROJ)

.PHONY: clean
