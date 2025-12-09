
BINDIR=../
ROM=dev7

CC=${BINDIR}glcc.cmd
GTMID2C=../mid2glcc_gtmid/midi_converter.exe
CFLAGS=-map=64k,./music.ovl --no-runtime-bss sound.s clock.s

PGMS=music.gt1

MIDIS=May_Be.gbas.c

all: ${PGMS}

music.gt1: music.c ${MIDIS}
	${CC} -o $@ -rom=${ROM} ${CFLAGS} $< ${MIDIS}

clean: FORCE
	rm -rf ${PGMS}

midi/%.c: midi/%.gtmid
	${GTMID2C} -s 256 $< $@

FORCE:

.PHONY: FORCE

.PRECIOUS: ${MIDIS}
