PREFIX=../ppmck-dist

DIRS= doc doc/nesasm bin mk nes_include nes_include/ppmck

FILES_top= \
	changelog.txt \
	pp-changes.txt \
	pp-readme.txt \
	ppmck-ja.txt \
	ppmck.txt \
	readme_ex.txt

FILES_doc= \
	mck.txt \
	mckc.txt \
	mckc_p040719.txt \
	pmck.txt \
	pmckc.txt

FILES_doc_nesasm= \
	CPU_INST.TXT \
	HISTORY.TXT \
	INDEX.TXT \
	NESHDR20.TXT \
	USAGE.TXT

FILES_bin= \
	nesasm$(EXESFX) \
	pceas$(EXESFX) \
	ppmckc$(EXESFX) \
	ppmckc_e$(EXESFX)

FILES_mk= \
	ppmck.mk

FILES_nes_include= \
	ppmck.asm

FILES_nes_include_ppmck= \
	dpcm.h \
	fds.h \
	fme7.h \
	freqdata.h \
	internal.h \
	mmc5.h \
	n106.h \
	sounddrv.h \
	vrc6.h \
	vrc7.h

FILES:= $(FILES_top) \
	$(foreach i,$(DIRS),$(foreach j,$(FILES_$(subst /,_,$i)),$i/$j))

include mk/sys.mk

.PHONY: build install clean make-distdirs
.SUFFIX: $(OBJSFX) $(EXESFX)

build:
	$(MAKE) -C src/nesasm all
	$(MAKE) -C src/ppmckc all
	$(MAKE) -C src/nesasm install
	$(MAKE) -C src/ppmckc install

clean:
	$(MAKE) -C src/nesasm clean
	$(MAKE) -C src/ppmckc clean

install: make-distdirs
	for i in $(FILES); do cp $$i $(PREFIX)/$$i; done

make-distdirs:
	[ -e $(PREFIX) ] || mkdir $(PREFIX)
	for i in $(DIRS); do [ -e $(PREFIX)/$$i ] || mkdir $(PREFIX)/$$i; done
