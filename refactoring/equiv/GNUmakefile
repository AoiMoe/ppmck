PWD= $(shell pwd)
EXESFX= .exe

RELIABLE_BASE= $(PWD)/reliable
RELIABLE_PPMCKC= $(RELIABLE_BASE)/bin/ppmckc$(EXESFX)
RELIABLE_NESASM= $(RELIABLE_BASE)/bin/nesasm$(EXESFX)
RELIABLE_PPMCKASMSRCDIR= $(RELIABLE_BASE)/nes_include

TEST_TARGET_BASE= $(PWD)/../..

ifeq ($(shell test -e .in_place && echo yes),yes)
TEST_TARGET_PPMCKC= $(TEST_TARGET_BASE)/src/ppmckc/ppmckc$(EXESFX)
TEST_TARGET_NESASM= $(TEST_TARGET_BASE)/src/nesasm/nesasm$(EXESFX)
else
TEST_TARGET_PPMCKC= $(TEST_TARGET_BASE)/bin/ppmckc$(EXESFX)
TEST_TARGET_NESASM= $(TEST_TARGET_BASE)/bin/nesasm$(EXESFX)
endif
TEST_TARGET_PPMCKASMSRCDIR= $(TEST_TARGET_BASE)/nes_include

RELIABLE_OBJ_PREFIX= obj.reliable.
TEST_TARGET_OBJ_PREFIX= obj.target.

TESTS= $(shell for i in test*.mml; do test -e $$i && basename $$i .mml; done)

ifeq ($(TESTS),)
$(error test not found.)
endif

.PHONY: test all cmp clean
.PHONY: _each_tests $(TESTS)

test:
	@$(MAKE) --no-print-directory all
	@$(MAKE) --no-print-directory cmp

all cmp clean:
	@echo ===enter $@ stage===
	@$(MAKE) --no-print-directory TARGET=$@ _each_tests
	@echo ===leave $@ stage===

_each_tests: $(TESTS)


BUILD_F = \
	@$(MAKE) --no-print-directory -f $(PWD)/ppmck.mk \
	SONGNAME=$(1) \
	PPMCKC=$($(2)_PPMCKC) \
	NESASM=$($(2)_NESASM) \
	PPMCKASMSRCDIR=$($(2)_PPMCKASMSRCDIR) \
	OBJ=$($(2)_OBJ_PREFIX)$(1) $(3)
NSF_F = $($(2)_OBJ_PREFIX)$(1)/$(1).nsf

ifeq ($(TARGET),cmp)
$(TESTS):
	@echo -n "comparing $@ ... "
	@cmp $(call NSF_F,$@,RELIABLE) $(call NSF_F,$@,TEST_TARGET) 2>&1 && \
		echo ok.
else
$(TESTS):
	$(call BUILD_F,$@,RELIABLE,$(TARGET))
	$(call BUILD_F,$@,TEST_TARGET,$(TARGET))
endif
