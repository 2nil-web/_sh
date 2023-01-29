
ARCH=x86_64-w64-mingw32-

CFLAGS += -Wall -Wextra -pedantic

#GCC=gcc
#CFLAGS += -std=gnu18

GCC=g++
CFLAGS += -std=gnu++23

ifdef ARCH
	CC=$(ARCH)${GCC}
	LD=$(ARCH)${GCC}
else
	CC=${GCC}
	LD=ld
endif

LDFLAGS += -static
#LDFLAGS += -mwindows
LDLIBS  += -lreadline -lhistory -ltermcap.dll
LDLIBS += -lwinmm #-lwsock32 -lole32 -luuid -lcomctl32 -loleaut32

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TARGET=_sh.exe


all : $(TARGET)

$(TARGET) : $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean :
	rm -f $(OBJS) *~ $(TARGET)

rclean :
	rm -f $(OBJS) *.d *~ $(TARGET)

ifneq ($(MAKECMDGOALS),rclean)
# Régles pour construire les .exe d'après les .o et .c
%.exe: %.c
	$(LINK.c) $(LOADLIBES) $(LDLIBS) $^ -o $@

%.exe: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.d: %.c
	@echo "Building "$@" from "$<
	@$(SHELL) -ec '${GCC} -isystem /usr/include -MM $(CPPFLAGS) $< > $@'

# Inclusion of the dependency files '.d'
ifdef SRCS
-include $(SRCS:.c=.d)
endif
endif

