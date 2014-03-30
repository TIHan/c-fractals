CC		= gcc
CFLAGS		= -Wall -std=c99 -pedantic-errors -Werror -DGL_GLEXT_PROTOTYPES
SOURCES		= $(wildcard *.c)
OBJECTS		= $(SOURCES:%.c=%.o)
LIBS		= -framework Cocoa -framework OpenGL -framework IOKit -framework SDL2
INCLUDES	= -I.
EXECUTABLE	= c-fractals

ifeq ($(shell uname), Linux)
	LIBS = -lm -lGL -lSDL2
endif

release: CFLAGS += -O2
release: $(SOURCES) $(EXECUTABLE)

debug: CFLAGS += -DDEBUG -g3
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.cc.o:
	$(Cc) $(INCLUDES) -c -o $@ $< $(CFLAGS)
