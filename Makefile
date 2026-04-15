CC := $(CROSS_COMPILE)gcc

PKG_BUILD_DIR ?= .
BIN_DIR = $(PKG_BUILD_DIR)/build/bin

ifdef STAGING_DIR
    AMX_INC  := $(STAGING_DIR)/usr/include
    AMX_LDIR := $(STAGING_DIR)/usr/lib
else
    AMX_INC  := /usr/include
    AMX_LDIR :=
endif

EXTRA_CFLAGS  := -I$(AMX_INC) -D_GNU_SOURCE
EXTRA_LDFLAGS := $(if $(AMX_LDIR),-L$(AMX_LDIR),) \
                 -lamxd -lamxo -lamxc -lamxp -lamxb

.PHONY: all clean

all: $(BIN_DIR)/test-service

$(BIN_DIR)/test-service: src/main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< $(LDFLAGS) $(EXTRA_LDFLAGS)

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(PKG_BUILD_DIR)/build/
