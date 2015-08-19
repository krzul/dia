CC   = gcc	# the C compiler
COPT = -O3 -Wall -s -m32	# options for the C compiler
LIBS = -lm	# libraries

OBJS = pfitsio.o errmess.o mkushortone.o

.c.o :
	${CC} ${COPT} -c $<

mkushortone: ${OBJS}
	${CC} ${OBJS} -o $@ ${LIBS} ${COPT}

${OBJS} : makefile
