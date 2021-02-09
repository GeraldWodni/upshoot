PROJECT=upshoot
GBDK=../gbdk
EMULATOR=vbam
EMULATORFLAGS= -f 2

CC=$(GBDK)/bin/lcc
CCFLAGS=-Wa-l -Wl-m -Wl-j

all: $(PROJECT).gb

%.gb: %.c
	$(CC) $(CCFLAGS) -o $@ $<

.PHONY: run

run: $(PROJECT).gb
	$(EMULATOR) $(EMULATORFLAGS) $(PROJECT).gb
