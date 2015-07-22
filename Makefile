target=event
objs=common.o flog.o fstr.o fmap.o flist.o fqueue.o fbit.o net.o webc.o
cc=gcc
ccflags= -m32 -std=c99 -g

target:${objs}
	${cc} ${ccflags} -o ${target} event.c ${objs} -lpthread

common.o:./common/common.h
	${cc} ${ccflags} -c ./common/common.c

flog.o:./common/flog.h
	${cc} ${ccflags} -c ./common/flog.c

fstr.o:common.o
	${cc} ${ccflags} -c ./common/fstr.c

fmap.o:common.o
	${cc} ${ccflags} -c ./common/fmap.c

flist.o:common.o
	${cc} ${ccflags} -c ./common/flist.c

fqueue.o:common.o
	${cc} ${ccflags} -c ./common/fqueue.c


fbit.o:common.o
	${cc} ${ccflags} -c ./common/fbit.c

net.o: net.h
	${cc} ${ccflags} -c ./net.c

webc.o: webc.h
	${cc} ${ccflags} -c ./webc.c

.PHONY:clean
clean:
	-rm -f *.o ${target} core
