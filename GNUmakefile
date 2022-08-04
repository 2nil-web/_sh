
BUILD=x86_64-w64-mingw32

ifdef BUILD
	CC=$(BUILD)-gcc
	CXX=$(BUILD)-g++
	LD=$(BUILD)-gcc
	RC=$(BUILD)-windres
else
	CC=gcc
	CXX=g++
	LD=ld
	RC=windres
endif

#CPPFLAGS += -I/usr/local/include -I.
CFLAGS += -Wall
#LDFLAGS += -L/usr/local/lib
#LDFLAGS += -static # -mwindows
LDLIBS  += -lreadline -lhistory -ltermcap
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
%.exe: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.exe: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Régles pour construire les fichier objet d'après les .rc
%.o : %.rc
ifeq ($(findstring 64,$(BUILD)),64)
	cp QuickRun64.manifest QuickRun.manifest
else
	cp QuickRun32.manifest QuickRun.manifest
endif
	$(RC) $(CPPFLAGS) $< --include-dir . $(OUTPUT_OPTION)

%.d: %.c
	@echo "Building "$@" from "$<
	@$(SHELL) -ec 'gcc -isystem /usr/include -MM $(CPPFLAGS) $< > $@'

# Inclusion of the dependency files '.d'
ifdef SRCS
-include $(SRCS:.c=.d)
endif
endif

