target=http
objs=common.o fstr.o fmap.o
cc=gcc
ccflags= -m32 -std=c99 -g
target:${objs}
	${cc} ${ccflags} -o ${target} http.c ${objs}

common.o:common.h
	${cc} ${ccflags} -c common.c

fstr.o:common.o
	${cc} ${ccflags} -c fstr.c

fmap.o:common.o
	${cc} ${ccflags} -c fmap.c

.PHONY:clean
clean:
	-rm -f *.o ${target}
