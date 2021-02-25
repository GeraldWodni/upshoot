PROJECT=upshoot
GBDK=../gbdk
#EMULATOR=vbam -f 17
#EMULATOR=vbam -f 2
EMULATOR=mgba-qt

CC=$(GBDK)/bin/lcc
CCFLAGS=-Wa-l -Wl-m -Wl-j -Wm-yc

all: $(PROJECT).gb

$(PROJECT).gb: $(PROJECT).c media/*.c
	$(CC) $(CCFLAGS) -o $@ $<

run: $(PROJECT).gb
	$(EMULATOR) $(PROJECT).gb

.PHONY: tile-designer, clean
tile-designer:
	wine ../gbtd/GBTD.EXE

clean:
	rm $(PROJECT).gb
	rm $(PROJECT).map
	rm $(PROJECT).noi
