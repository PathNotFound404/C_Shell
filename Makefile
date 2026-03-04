###########################################################
#
# Makefile: hfsh26
#
###########################################################
.SUFFIXES: .h .c .cpp .l .o

CC = gcc
CXX = g++
CFLAGS = -Werror -g -I.
LEX = flex
LIBS = 
RM = /bin/rm
RMFLAGS = -rf

PROGS = hfsh26
OBJS = hfsh26.o lex.yy.o


$(PROGS): $(OBJS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CXX) $(CFLAGS) -c $<

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c $<

lex.yy.c: scan.l
	$(LEX) $<

test:
	./test-hfsh26.sh

test-all:
	./test-hfsh26.sh -c

clean:
	$(RM) $(RMFLAGS) *.o *~ hfsh26 lex.yy.c tests-out


