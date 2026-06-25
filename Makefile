NINJA ?= ninja
NINJA_FILE ?= build.ninja
PLAYWRIGHT ?= npx playwright

.PHONY: all x11wasm lib browser typecheck typecheck-js compile_commands compile-commands test clean

all: browser compile_commands

x11wasm lib:
	$(NINJA) -f $(NINJA_FILE) x11wasm

browser:
	$(NINJA) -f $(NINJA_FILE) browser

typecheck typecheck-js:
	$(NINJA) -f $(NINJA_FILE) typecheck_js

compile_commands compile-commands:
	$(NINJA) -f $(NINJA_FILE) compile_commands

test: browser
	$(PLAYWRIGHT) test -c playwright.config.js

clean:
	$(NINJA) -f $(NINJA_FILE) -t clean
