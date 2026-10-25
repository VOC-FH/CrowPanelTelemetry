ARDUINO_CLI ?= arduino-cli
BUILD ?= /tmp/codex/build/CrowPanelTelemetry/local
FQBN ?= esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3
LIBRARIES ?=
SOURCES := $(wildcard *.h *.cpp *.ino tests/*.cpp tests/sd-stubs/*.h tests/sd-stubs/driver/*.h tests/sd-stubs/soc/*.h tests/arduino-stubs/*.h)
.PHONY: format format-check test compile patch-radio patch-jpeg
format:
	clang-format -i $(SOURCES)
format-check:
	clang-format --dry-run --Werror $(SOURCES)
test:
	BUILD='$(BUILD)' python3 tools/test.py
patch-radio:
	python3 tools/patch_dependency.py radiolib-7.8.1 '$(LIBRARIES)/RadioLib'
patch-jpeg:
	python3 tools/patch_dependency.py jpegdec-1.8.4 '$(LIBRARIES)/JPEGDEC'
compile:
	mkdir -p $(BUILD)/tmp $(BUILD)/cache
	TMPDIR=$(BUILD)/tmp ARDUINO_BUILD_CACHE_PATH=$(BUILD)/cache $(ARDUINO_CLI) compile --fqbn '$(FQBN)' $(if $(LIBRARIES),--libraries '$(LIBRARIES)') --build-path $(BUILD)/output .
