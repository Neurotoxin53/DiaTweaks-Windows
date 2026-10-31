# Windows launcher. The macOS Makefile at the repo root is separate.
BIN := bin
MINGW := x86_64-w64-mingw32-gcc
CFLAGS := -O2 -Wall -Wextra -Werror -std=c11

.PHONY: all test clean

all: $(BIN)/diatweaks.dll $(BIN)/diawrap.exe

$(BIN):
	mkdir -p $(BIN)

$(BIN)/diatweaks.dll: diatweaks.c tweak.c tweak.h winrt_ids.h | $(BIN)
	$(MINGW) $(CFLAGS) -shared -o $@ diatweaks.c tweak.c -lole32 -loleaut32 -lshell32 -luser32 -luuid

$(BIN)/diawrap.exe: diawrap.c | $(BIN)
	$(MINGW) $(CFLAGS) -o $@ diawrap.c -luser32 -lshell32

test: tweak_test
	./tweak_test

tweak_test: tweak_test.c tweak.c tweak.h
	$(CC) $(CFLAGS) -o $@ tweak_test.c tweak.c

clean:
	rm -rf $(BIN) tweak_test tweak_test.exe
