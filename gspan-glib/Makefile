
OBJS = main.o graph.o gspan.o fileio.o history.o minimum.o extend.o

CC = mpicc
CFLAGS = -O2 -pg -g3 -fprofile-arcs -I. `pkg-config --cflags glib-2.0`
LDLIBS = `pkg-config --libs glib-2.0` -lgcov

gspan: ${OBJS}

clean:
	${RM} -rf *.o gspan
