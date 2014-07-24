CPP = g++
CPPFLAGS = -g -Wall -I..
LDFLAGS = -g
LDLIBS = -lreadline -lcurlpp ../libjson_linux-gcc-4.4.7_libmt.a
#LDLIBS = -lreadline -lboost_regex -L. -ljson_linux-gcc-4.4.7_libmt

SRCS = main.cpp cli.cpp cli_tree.cpp cli_readline.cpp cli_action.cpp \
       cli_builtins.cpp cli_utils.cpp cli_string.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

all: cli

%.o: %.cpp %.hpp
	$(CPP) $(CPPFLAGS) -c $<

cli: $(OBJS)
	$(CPP) $(LDFLAGS) -o cli $(OBJS) $(LDLIBS)
