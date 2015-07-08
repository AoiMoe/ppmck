# nesasm makefile for POSIX-like systems

include ../../mk/sys.mk

OBJS =	main$(OBJSFX) \
	input$(OBJSFX) \
	assemble$(OBJSFX) \
	expr$(OBJSFX) \
	code$(OBJSFX) \
	command$(OBJSFX) \
	macro$(OBJSFX) \
	func$(OBJSFX) \
	proc$(OBJSFX) \
	symbol$(OBJSFX) \
	pcx$(OBJSFX) \
	output$(OBJSFX) \
	crc$(OBJSFX) \
	pce$(OBJSFX) \
	map$(OBJSFX) \
	mml$(OBJSFX) \
	nes$(OBJSFX)

EXEDIR   = ../../bin/

TARGPCE  = pceas$(EXESFX)
TARGNES  = nesasm$(EXESFX)
TARGETS  = $(TARGPCE) $(TARGNES)

.PHONY: all clean strip install
.SUFFIX: $(EXESFX) $(OBJSFX)

#
#

all: $(TARGETS)

# CLEAN
#

clean:
	$(RM) *$(OBJSFX)
	$(RM) $(TARGETS)

# INSTALL
#

install:
	[ -e $(EXEDIR) ] || mkdir -p $(EXEDIR)
	$(CP) $(TARGPCE) $(EXEDIR)
	$(CP) $(TARGNES) $(EXEDIR)

# ASSEMBLER
#

$(OBJS)		: defs.h externs.h protos.h
main$(OBJSFX)   : inst.h vars.h
expr$(OBJSFX)   : expr.h
pce$(OBJSFX)    : pce.h
nes$(OBJSFX)    : nes.h
%$(OBJSFX)	: %.c
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
