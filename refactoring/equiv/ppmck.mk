PPMCK_MK:=	1
PPMCKCFLAGS=	-i
NESASMFLAGS=	-raw
ifdef USE_MCKMACRO
MCKMACRO=	$(MCKTOOLSDIR)/mckmacro/mckmacro/mckmacro.exe
endif
NSFPLAY=	$(MCKTOOLSDIR)/nsfplay/nsfplay.exe
PPMCKDIR=	$(MCKTOOLSDIR)/ppmck/mck
PPMCKC=		$(PPMCKDIR)/bin/ppmckc.exe
NESASM=		$(PPMCKDIR)/bin/nesasm.exe

PPMCKASMSRCDIR=	$(PPMCKDIR)/nes_include
PPMCKASMSRCS=	$(PPMCKASMSRCDIR)/ppmck.asm \
		$(PPMCKASMSRCDIR)/ppmck/dpcm.h \
		$(PPMCKASMSRCDIR)/ppmck/fds.h \
		$(PPMCKASMSRCDIR)/ppmck/fme7.h \
		$(PPMCKASMSRCDIR)/ppmck/freqdata.h \
		$(PPMCKASMSRCDIR)/ppmck/internal.h \
		$(PPMCKASMSRCDIR)/ppmck/mmc5.h \
		$(PPMCKASMSRCDIR)/ppmck/n106.h \
		$(PPMCKASMSRCDIR)/ppmck/sounddrv.h \
		$(PPMCKASMSRCDIR)/ppmck/vrc6.h \
		$(PPMCKASMSRCDIR)/ppmck/vrc7.h
PPMCKASM=	ppmck.asm
PPMCKNES=	ppmck.nes
PPMCKCTMPS=	effect.h define.inc

SED=sed
SORT=sort

OBJ= obj

ifeq ($(OBJ),)
OBJDIR:= .
else
OBJDIR:= $(OBJ)
endif

ifeq ($(DMCSRC),)
DMCSRCDIR:= .
else
DMCSRCDIR:= $(DMCSRC)
endif

SRC:= $(SONGNAME).mml
PPMCKCOUT:= $(SONGNAME).h
OUT:= $(SONGNAME).nsf

PSRC:= $(SONGNAME).mmli
PSRCDST:= $(OBJDIR)/$(PSRC)

PPMCKCOUTDST:= $(OBJDIR)/$(PPMCKCOUT)
PPMCKCTMPSDST:= $(addprefix $(OBJDIR)/,$(PPMCKCTMPS))
PPMCKNESDST:= $(OBJDIR)/$(PPMCKNES)
OUTDST:= $(OBJDIR)/$(OUT)

s2bs= $(subst /,\,$(1))

ifdef USE_AUTO_DMC
ifndef DMCFILES
DMCFILES:= $(shell \
	cat '$(SRC)' | \
	$(SED) -E '/^ *@DPCM/!d;s:^.*\{ *\"(.+)\".*:\1:;s:\\+:\\:g' | \
	$(SORT) -u )
endif
endif
DMCFILESDST:= $(addprefix $(OBJDIR)/,$(DMCFILES))

.PHONY: all play clean cleantmp
.SUFFIX: .mml .mmli .nsf .dmc

all: $(OUTDST)

play: all
	start '$(call s2bs,$(NSFPLAY))' '$(call s2bs,$(OUTDST))'

ifeq ($(OBJ),)
MAKEOBJ:=
else
MAKEOBJ:= $(OBJDIR)
$(OBJDIR):
	-mkdir '$(OBJDIR)'
endif

ifdef USE_MCKMACRO
$(PSRCDST): $(SRC) | $(MAKEOBJ)
	$(MCKMACRO) -q -o '$@' '$<'
else
$(PSRCDST): $(SRC) | $(MAKEOBJ)
	cp $(SRC) $(PSRCDST)
endif

$(OBJDIR)/%.dmc: $(DMCSRCDIR)/%.dmc | $(MAKEOBJ)
	cp '$<' '$@'

$(PPMCKCOUTDST): $(PSRCDST) $(DMCFILESDST) | $(MAKEOBJ)
	cd '$(OBJDIR)' && '$(PPMCKC)' $(PPMCKCFLAGS) $(PSRC) $(PPMCKCOUT)

$(OUTDST): $(PPMCKCOUTDST) $(PPMCKASMSRCS)
	cd '$(OBJDIR)' && NES_INCLUDE='$(PPMCKASMSRCDIR)' '$(NESASM)' $(NESASMFLAGS) $(PPMCKASM)
	-rm -f '$(OUTDST)'
	mv '$(OBJDIR)/$(PPMCKNES)' '$(OUTDST)'

TMPSDST:=	$(PSRCDST) $(DMCFILESDST) \
		$(PPMCKCTMPSDST) $(PPMCKCOUTDST) $(PPMCKNESDST) $(OUTDST)

cleantmp:
	-rm -f $(TMPSDST)

clean: cleantmp
	-rm -f '$(OUTDST)'
ifneq ($OBJ),'')
	-[ ! -d '$(OBJDIR)' ] || rmdir '$(OBJDIR)'
endif
