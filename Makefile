# Simple project Makefile
# OpenWrt package is in test-service/Makefile

.PHONY: all clean install

all:
	@echo "Test service - shell script, nothing to compile"

clean:
	@echo "Nothing to clean"

install:
	install -D -m 755 test-service/files/test-service.sh /usr/bin/test-service.sh
