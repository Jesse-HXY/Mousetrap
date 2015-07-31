#mousetrap makefile v1.0
#07/29/2015 - gh

CC = g++
CFLAGS = -g -O2
OUTPUT = mousetrap
LIBDIR = /usr/lib/X11
INCDIR = /usr/include/X11
LIBS = X11
SRC = \
		main.cpp

all:
	$(CC) $(SRC) -L $(LIBDIR) -I $(INCDIR) -l $(LIBS) $(CFLAGS) -o $(OUTPUT) 