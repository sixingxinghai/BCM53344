PROG=	imimore
CFLAGS+=-I. -DREGEX -DTERMIOS
SRCS=	ch.c command.c decode.c filename.c help.c input.c line.c \
	linenum.c main.c option.c os.c output.c position.c prim.c \
	screen.c signal.c tags.c ttyin.c 
OBJS=	ch.o command.o decode.o filename.o help.o input.o line.o \
	linenum.o main.o option.o os.o output.o position.o prim.o \
	screen.o signal.o tags.o ttyin.o 
LDADD+=	-ltermcap
DPADD+=	${LIBTERM}
LINKS=	${BINDIR}/more

all: ${PROG}
	${CC} ${CFLAGS} ${OBJS} -o $< ${LDADD}

imimore: ${OBJS}
