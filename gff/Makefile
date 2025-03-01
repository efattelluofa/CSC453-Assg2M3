CC = gcc
#CFLAGS = -O3 -g -Wall
CFLAGS = -g -Wall

LEX = flex
#LEXOPTS = -CFra -L 

YACC = yacc
YACCOPTS = -dv 
#YDEBUGOPTS = -DYYDEBUG
YDEBUGOPTS =

CFILES = main.c print.c symbols.c first.c follow.c

GENCFILES = lex.yy.c y.tab.c		    # machine generated files
GENHFILES = y.tab.h		 	    # machine generated files

HFILES = global.h symbols.h

OFILES = main.o lex.yy.o y.tab.o print.o symbols.o first.o follow.o

LIBS = -lfl -lm

DEST = gff

$(DEST) : $(OFILES) 
	$(CC) $(CFLAGS) $(OFILES) -o $(DEST) $(LIBS)

lex.yy.c : scanner.l
	$(LEX) $(LEXOPTS) scanner.l

y.tab.c : parse.y 
	$(YACC) $(YACCOPTS) parse.y

y.tab.h : parse.y
	$(YACC) $(YACCOPTS) parse.y

#proto.h : $(CFILES)
#	cproto $(CFILES) -o proto.h

main.o : main.c proto.h

lex.yy.o : lex.yy.c y.tab.h

print.o : print.c proto.h y.tab.h

symbols.o : symbols.h global.h symbols.c

first.o : symbols.h global.h proto.h first.c

follow.o : symbols.h global.h proto.h first.c

clean:
	/bin/rm -f *.BAK *.o $(DEST)

realclean:
	/bin/rm -f *.BAK *.o $(DEST) $(GENCFILES) $(GENHFILES)
