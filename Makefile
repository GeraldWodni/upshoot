PROJECT=upshoot
GBDK=../gbdk
#EMULATOR=vbam -f 17
EMULATOR=mgba-qt

CC=$(GBDK)/bin/lcc
CCFLAGS=-Wa-l -Wl-m -Wl-j

all: $(PROJECT).gb

$(PROJECT).gb: $(PROJECT).c media/*.c
	$(CC) $(CCFLAGS) -o $@ $<

run: $(PROJECT).gb
	$(EMULATOR) $(PROJECT).gb

.PHONY: tile-designer
tile-designer:
	wine ../gbtd/GBTD.EXE
