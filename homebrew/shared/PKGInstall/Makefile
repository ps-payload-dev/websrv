#   Copyright (C) 2025 John Törnblom

PLATFORM ?= ps5
ifeq ($(PLATFORM),ps4)
    include $(PS4_PAYLOAD_SDK)/toolchain/orbis.mk
else ifeq ($(PLATFORM),ps5)
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error Set PLATFORM=ps4 or PLATFORM=ps5)
endif

CFLAGS := -g -O0 -Wall -Werror
LDADD  := -lSceIpmi -lSceAppInstUtil
ELF    := $(OUTPUT_DIR)/pkg_install.elf

.PHONY: all clean
all:
	mkdir -p "$(OUTPUT_DIR)"
	$(CC) $(CFLAGS) $(LDADD) -o "$(ELF)" pkg_install.c

clean:
	rm -f $(ELF)
