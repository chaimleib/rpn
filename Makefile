PROJ = rpn
YACC = bison

all: $(PROJ)

$(PROJ): $(PROJ).tab.o
	$(CC) -o $(PROJ) $<

%.tab.o: %.tab.c

%.tab.c: %.y
	$(YACC) $<

clean:
	rm -f *.o
	rm -f *.tab.c
	rm -f $(PROJ)

.PHONY: clean
