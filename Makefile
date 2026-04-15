CC := $(CROSS_COMPILE)gcc

PKG_BUILD_DIR ?= .
BIN_DIR = $(PKG_BUILD_DIR)/build/bin

STAGING_DIR ?= /usr/local
AMX_INC := $(STAGING_DIR)/usr/include
AMX_LDIR := $(STAGING_DIR)/usr/lib

CFLAGS += -I$(AMX_INC)
LDFLAGS += -L$(AMX_LDIR) -lamxd -lamxo -lamxc -lamxp -lamxb

.PHONY: all clean

all: $(BIN_DIR)/test-service

$(BIN_DIR)/test-service: src/main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(PKG_BUILD_DIR)/build/
