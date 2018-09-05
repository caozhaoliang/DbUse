BINFILE := ./OracleDemo

LIBS := stdc++ m pthread clntshcore clntsh ipc1 mql1 nnz12 occi ociei ocijdbc12 sqlplusic

INCS := ./include

LIBPATHS := /usr/lib/ /usr/local/lib ./lib

CFLAGS = -g -Wall -pg -O2 -Wno-deprecated

CC = g++

COMMONSRCS=$(wildcard *.cpp)

COMMONOBJS=$(patsubst %.cpp, %.o , $(COMMONSRCS))

all:$(BINFILE)
$(BINFILE): $(COMMONOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(addprefix -L,$(LIBPATHS)) $(addprefix -l,$(LIBS))

%.o:%.cpp
	$(CC) $(CFLAGS) $(addprefix -I,$(INCS)) -c $< -o $@

clean:
	rm -rf $(COMMONOBJS) $(BINFILE)
