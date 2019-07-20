# mckc makefile for POSIX-like systems

include ../../mk/sys.mk

SRCS	= datamake.c file.c mckc.c strings.c

OBJDIR = ./obj
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))

.PHONY: all clean strip install
.SUFFIX: $(EXESFX) $(OBJSFX)

TARGET_E= $(OBJDIR)/ppmckc_e$(EXESFX)
TARGET	= $(OBJDIR)/ppmckc$(EXESFX)

all: $(TARGET_E) $(TARGET)

$(TARGET_E): $(OBJS) $(OBJDIR)/version_e$(OBJSFX)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(OBJDIR)/version_e$(OBJSFX)
ifdef IS_STRIP
	$(STRIP) $@
endif

$(TARGET): $(OBJS) $(OBJDIR)/version$(OBJSFX)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(OBJDIR)/version$(OBJSFX)
ifdef IS_STRIP
	$(STRIP) $@
endif

$(OBJS): $(OBJDIR)

$(OBJDIR)/version_e$(OBJSFX): version.c
	$(CC) $(CFLAGS) $(CDEFS) -DENGLISH -o $@ -c version.c

$(OBJDIR)/%$(OBJSFX): %.c
	$(CC) $(CFLAGS) $(CDEFS) -o $@ -c $<

strip:
	$(STRIPPER) $(TARGET) $(TARGET_E)

mckc$(OBJSFX): mckc.h
datamake$(OBJSFX): mckc.h

install:
	[ -e $(EXEDIR) ] || mkdir -p $(EXEDIR)
	$(CP) $(TARGET) $(EXEDIR)/
	$(CP) $(TARGET_E) $(EXEDIR)/

clean:
	$(RM) $(OBJS) $(OBJDIR)/version$(OBJSFX) $(OBJDIR)/version_e$(OBJSFX) $(TARGET) $(TARGET_E)

$(OBJDIR) :
	mkdir $(OBJDIR)
