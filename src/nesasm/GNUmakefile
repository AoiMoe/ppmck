# nesasm makefile for POSIX-like systems

include ../../mk/sys.mk

SRCS =	main.c \
	input.c \
	assemble.c \
	expr.c \
	code.c \
	command.c \
	macro.c \
	func.c \
	proc.c \
	symbol.c \
	pcx.c \
	output.c \
	crc.c \
	pce.c \
	map.c \
	mml.c \
	nes.c

EXEDIR   = ../../bin/
OBJDIR   = ./obj

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))

TARGPCE  = $(OBJDIR)/pceas$(EXESFX)
TARGNES  = $(OBJDIR)/nesasm$(EXESFX)
TARGETS  = $(TARGPCE) $(TARGNES)

.PHONY: all clean strip install
.SUFFIX: $(EXESFX) $(OBJSFX)

#
#

all: $(TARGETS)

# CLEAN
#

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

# INSTALL
#

install:
	[ -e $(EXEDIR) ] || mkdir -p $(EXEDIR)
	$(CP) $(TARGPCE) $(EXEDIR)
	$(CP) $(TARGNES) $(EXEDIR)

# ASSEMBLER
#

$(OBJS)		: $(OBJDIR) defs.h externs.h protos.h
main$(OBJSFX)   : inst.h vars.h
expr$(OBJSFX)   : expr.h
pce$(OBJSFX)    : pce.h
nes$(OBJSFX)    : nes.h
$(OBJDIR)/%$(OBJSFX)	: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# EXE
#

$(TARGPCE) : $(OBJS)
	$(CC) -o $(TARGPCE) $(OBJS)
ifdef IS_STRIP
	$(STRIP) $@
endif

$(TARGNES) : $(OBJS)
	$(CC) -o $(TARGNES) $(OBJS)
ifdef IS_STRIP
	$(STRIP) $@
endif

$(OBJDIR) :
	mkdir $(OBJDIR)
