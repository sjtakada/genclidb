CPP = g++
CPPFLAGS = -g -Wall -I..
LDFLAGS = -g
LDLIBS = -lreadline -lboost_regex ../libjson_linux-gcc-4.4.7_libmt.a
#LDLIBS = -lreadline -lboost_regex -L. -ljson_linux-gcc-4.4.7_libmt

SRCS = main.cpp cli.cpp cli_tree.cpp cli_readline.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

all: cli

%.o: %.cpp %.hpp
	$(CPP) $(CPPFLAGS) -c $<

cli: $(OBJS)
	g++ $(LDFLAGS) -o cli $(OBJS) $(LDLIBS)
