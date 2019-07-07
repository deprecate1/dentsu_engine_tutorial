# program name and object files
PROGRAM = go03
OBJS = go03.o

# define
CC = g++
CFLAGS = -O2 -Wall
LDFLAGS = -lm -lstdc++
#LDFLAGS = -v -lm -lstdc++ -lpthread -static
#LDFLAGS = -v -lm -lstdc++ -lpthread

# suffixes rule
.SUFFIXES: .c .o

# target         '$^' ... list of files.
$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDFLAGS)

# suffixe rule   '$<' ... top file name of list of files.
.cpp.o:
	$(CC) $(CFLAGS) -c $<

# delete target
.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
