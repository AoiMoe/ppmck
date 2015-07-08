# mckc makefile for POSIX-like systems

include ../../mk/sys.mk

OBJS	= datamake$(OBJSFX) file$(OBJSFX) mckc$(OBJSFX) strings$(OBJSFX)

.PHONY: all clean strip install
.SUFFIX: $(EXESFX) $(OBJSFX)

TARGET_E= ppmckc_e$(EXESFX)
TARGET	= ppmckc$(EXESFX)

all: $(TARGET_E) $(TARGET)

$(TARGET_E): $(OBJS) version_e$(OBJSFX)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) version_e$(OBJSFX)
ifdef IS_STRIP
	$(STRIP) $@
endif

$(TARGET): $(OBJS) version$(OBJSFX)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) version$(OBJSFX)
ifdef IS_STRIP
	$(STRIP) $@
endif

version_e$(OBJSFX): version.c
	$(CC) $(CFLAGS) $(CDEFS) -DENGLISH -o $@ -c version.c

.c$(OBJSFX):
	$(CC) $(CFLAGS) $(CDEFS) -c $<

strip:
	$(STRIPPER) $(TARGET) $(TARGET_E)

mckc$(OBJSFX): mckc.h
datamake$(OBJSFX): mckc.h

install:
	[ -e $(EXEDIR) ] || mkdir -p $(EXEDIR)
	$(CP) $(TARGET) $(EXEDIR)/
	$(CP) $(TARGET_E) $(EXEDIR)/

clean:
	$(RM) $(OBJS) version$(OBJSFX) version_e$(OBJSFX) $(TARGET) $(TARGET_E)
