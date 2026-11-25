PLATFORM ?= ps5
SHELL := /bin/bash

# Samples that build with the payload SDK alone. Samples that need SDL, ffmpeg
# or other third-party packages are built by their own workflows instead.
TEST_SAMPLES := PKGInstall

.PHONY: all test clean

all:
	@set -uo pipefail; status=0; \
	find homebrew/$(PLATFORM) homebrew/shared -type f -name 'build*.sh' ! -name 'build-platform.sh' -print0 | sort -z | \
	while IFS= read -r -d '' script; do \
	  echo "==> $(PLATFORM): $$script"; \
	  bash "$$script" "$(PLATFORM)" || status=1; \
	done; exit $$status

test:
	@set -uo pipefail; status=0; \
	for sample in $(TEST_SAMPLES); do \
	  script="homebrew/shared/$$sample/build.sh"; \
	  if [ ! -f "$$script" ]; then \
	    echo "==> $(PLATFORM): $$sample (not available, skipped)"; \
	    continue; \
	  fi; \
	  echo "==> $(PLATFORM): $$script"; \
	  bash "$$script" "$(PLATFORM)" || status=1; \
	  if [ -z "$$(ls -A "build/$(PLATFORM)/homebrew/$$sample" 2>/dev/null)" ]; then \
	    echo "error: $$sample produced no output" >&2; \
	    status=1; \
	  fi; \
	done; exit $$status

clean:
	rm -rf "build/$(PLATFORM)/homebrew"
