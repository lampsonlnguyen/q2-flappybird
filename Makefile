Q2_CC ?= clang
Q2_LD ?= ld.lld
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -pedantic
SYSROOT ?= $(HOME)/.cache/q2/sysroot
Q2_FLAGS = --target=mipsel-linux-gnu --sysroot=$(SYSROOT) -march=mips32r2 -mabi=32 -mfp64 -mnan=2008 -mabs=2008 -fno-strict-aliasing
Q2_LDFLAGS = -m elf32ltsmip -e __start --dynamic-linker /lib/ld-linux-mipsn8.so.1 --image-base=0x400000 -z noexecstack --hash-style=sysv --build-id=none -s
SRC = src/app.c src/render.c core/game_framework.c src/platform_q2.c src/audio.c src/crt-q2.c
SOUNDS = $(addprefix assets/sound/,sfx_wing.wav sfx_point.wav sfx_die.wav sfx_swooshing.wav)
OBJ = $(addprefix build/,$(SRC:.c=.o))
CARD = dist/.flappybird
all: dist/q2-flappybird.zip
check-sysroot:
	@test -f "$(SYSROOT)/usr/include/alsa/asoundlib.h" -a -f "$(SYSROOT)/lib/libasound.so.2" || (echo "Missing Q2 ALSA sysroot: $(SYSROOT)" >&2; exit 1)
build/%.o: %.c src/app.h src/audio.h core/game_framework.h | check-sysroot
	mkdir -p $(@D)
	$(Q2_CC) $(Q2_FLAGS) -O2 -Wall -Wextra -Werror -std=gnu11 -Isrc -Icore -c $< -o $@
$(CARD)/q2-flappybird: $(OBJ)
	mkdir -p $(@D)
	$(Q2_LD) $(Q2_LDFLAGS) -o $@ $^ \
		$(SYSROOT)/lib/libc.so.6 $(SYSROOT)/lib/libpthread.so.0 \
		$(SYSROOT)/lib/libasound.so.2 $(SYSROOT)/lib/libgcc_s.so.1 \
		$(SYSROOT)/lib/ld-linux-mipsn8.so.1
$(CARD)/q2app: q2app
	mkdir -p $(@D)
	tr -d '\r' < $< > $@
$(CARD)/LICENSE.txt: LICENSE
	mkdir -p $(@D)
	cp $< $@
$(CARD)/sounds.stamp: $(SOUNDS)
	mkdir -p $(CARD)/sounds
	cp $(SOUNDS) $(CARD)/sounds/
	@touch $@
dist/q2-flappybird.zip: $(CARD)/q2-flappybird $(CARD)/q2app $(CARD)/LICENSE.txt $(CARD)/sounds.stamp
	cd dist && rm -f q2-flappybird.zip && zip -q q2-flappybird.zip .flappybird/q2-flappybird .flappybird/q2app .flappybird/LICENSE.txt .flappybird/sounds/*.wav
build/input-test: tests/input_test.c src/app.c core/game_framework.c src/app.h core/game_framework.h
	mkdir -p build
	$(CC) $(CFLAGS) -std=c11 -Isrc -Icore -o $@ tests/input_test.c src/app.c core/game_framework.c
check: build/input-test
	./build/input-test
	mkdir -p build
	$(CC) $(CFLAGS) -std=c11 -Isrc -Icore -c src/app.c -o build/app-test.o
	$(CC) $(CFLAGS) -std=c11 -Isrc -Icore -c src/render.c -o build/render-test.o
	$(CC) $(CFLAGS) -std=c11 -Isrc -Icore -c core/game_framework.c -o build/framework-test.o
clean:
	rm -rf build dist
.PHONY: all check clean check-sysroot
