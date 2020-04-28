CC = g++ -O2 -w -Wno-deprecated -std=c++11

LD_FLAGS = -l pthread -lgtest

tag = -i
test_out_tag = -ll

ifdef linux
tag = -n
test_out_tag = -lfl
endif

# gtest: Record.o Comparison.o ComparisonEngine.o Schema.o File.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o RelOp.o Function.o BigQ.o GTest.o Pipe.o MainScheme.o y.tab.o lex.yy.o Statistics.o
# 	$(CC) -o gtest Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o GTest.o Pipe.o RelOp.o MainScheme.o Statistics.o Function.o y.tab.o lex.yy.o $(test_out_tag) $(LD_FLAGS)

a4-1.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o RelOp.o Function.o BigQ.o Pipe.o Statistics.o y.tab.o lex.yy.o a41test.o
	$(CC) -o a4-1.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o RelOp.o Function.o BigQ.o Pipe.o Statistics.o y.tab.o lex.yy.o a41test.o $(test_out_tag) -lpthread

a2test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o Pipe.o y.tab.o lex.yy.o a2test.o
	$(CC) -o a2test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o Pipe.o y.tab.o lex.yy.o a2test.o $(test_out_tag) -lpthread

test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o RelOp.o Function.o BigQ.o Pipe.o mainScheme.o Statistics.o y.tab.o lex.yy.o test.o
	$(CC) -o a42.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o DBFile.o GenericDBFile.o HeapDBFile.o SortedDBFile.o RelOp.o Function.o BigQ.o mainScheme.o Pipe.o Statistics.o y.tab.o lex.yy.o test.o $(test_out_tag) -lpthread

GTest.o:
	$(CC) -g -c GTest.cc

test.o: test.cc
	$(CC) -g -c test.cc

mainScheme.o: MainScheme.cc
	$(CC) -g -c MainScheme.cc

a41test.o: a41test.cc
	$(CC) -g -c a41test.cc

a2test.o: a2test.cc
	$(CC) -g -c a2test.cc

Statistics.o: Statistics.cc
	$(CC) -g -c Statistics.cc

Comparison.o: Comparison.cc
	$(CC) -g -c Comparison.cc
	
ComparisonEngine.o: ComparisonEngine.cc
	$(CC) -g -c ComparisonEngine.cc
	
DBFile.o: DBFile.cc
	$(CC) -g -c DBFile.cc

GenericDBFile.o: GenericDBFile.cc
	$(CC) -g -c GenericDBFile.cc

HeapDBFile.o: HeapDBFile.cc
	$(CC) -g -c HeapDBFile.cc

SortedDBFile.o: SortedDBFile.cc
	$(CC) -g -c SortedDBFile.cc

RelOp.o: RelOp.cc
	$(CC) -g -c RelOp.cc

Function.o: Function.cc
	$(CC) -g -c Function.cc

BigQ.o: BigQ.cc
	$(CC) -g -c BigQ.cc

Pipe.o: Pipe.cc
	$(CC) -g -c Pipe.cc

File.o: File.cc
	$(CC) -g -c File.cc

Record.o: Record.cc
	$(CC) -g -c Record.cc

Schema.o: Schema.cc
	$(CC) -g -c Schema.cc

y.tab.o: Parser.y
	yacc -d Parser.y
	g++ -c y.tab.c

lex.yy.o: Lexer.l
	flex Lexer.l
	gcc  -c lex.yy.c

clean: 
	rm -f *.o
	rm -f *.out
	rm -f y.tab.*
	rm -f yyfunc.tab.*
	rm -f lex.yy.*
	rm -f lex.yyfunc*

uninstall: clean
	rm -f *.bin
	rm -f *.bin.meta