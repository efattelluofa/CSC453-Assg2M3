compile: driver.o scanner.o parser.o ast.o ast-print.o
	gcc -Wall -g -o compile scanner.o driver.o parser.o ast.o ast-print.o

parser.o: parser.c scanner.h 
	gcc -Wall -g -c -o parser.o parser.c 

scanner.o: scanner.c scanner.h
	gcc -Wall -g -c -o scanner.o scanner.c

ast.o: ast.c ast.h 
	gcc -Wall -g -c -o ast.o ast.c

ast-print.o: ast-print.c ast.h
	gcc -Wall -g -c -o ast-print.o ast-print.c

driver.o: driver.c scanner.h parser.o
	gcc -Wall -g -c -o driver.o driver.c

clean:
	rm -f compile scanner.o parser.o driver.o
