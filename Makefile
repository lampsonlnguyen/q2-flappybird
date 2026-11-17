Q2_CC ?= clang
Q2_LD ?= ld.lld
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -pedantic
Q2_FLAGS = --target=mipsel-linux-gnu -march=mips32r2 -mabi=32 -mfp64 -mnan=2008 -mabs=2008 -mno-abicalls -fno-pic -G0 -ffreestanding -fno-builtin -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
SRC = src/app.c src/render.c core/game_framework.c src/platform_q2.c
OBJ = $(addprefix build/,$(SRC:.c=.o))
CARD = dist/.flappybird
all: dist/q2-flappybird.zip
build/%.o: %.c src/app.h core/game_framework.h
	mkdir -p $(@D)
	$(Q2_CC) $(Q2_FLAGS) -Oz -Wall -Wextra -Werror -std=c11 -Isrc -Icore -c $< -o $@
$(CARD)/q2-flappybird: $(OBJ)
	mkdir -p $(@D)
	$(Q2_LD) -m elf32ltsmip -static -e __start -s -z noexecstack -o $@ $^
$(CARD)/q2app: q2app
	mkdir -p $(@D)
	tr -d '\r' < $< > $@
$(CARD)/LICENSE.txt: LICENSE
	mkdir -p $(@D)
	cp $< $@
dist/q2-flappybird.zip: $(CARD)/q2-flappybird $(CARD)/q2app $(CARD)/LICENSE.txt
	cd dist && rm -f q2-flappybird.zip && zip -q q2-flappybird.zip .flappybird/q2-flappybird .flappybird/q2app .flappybird/LICENSE.txt
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
.PHONY: all check clean
