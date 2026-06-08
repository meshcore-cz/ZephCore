SHELL := /bin/sh

ROOT := $(CURDIR)
WEST := $(ROOT)/.venv/bin/west
TOOLCHAIN_DIR := $(ROOT)/.toolchains/zephyr-sdk-1.0.1
DYLD_EXPAT := /opt/homebrew/opt/expat/lib
TOOL_PATH := $(ROOT)/.venv/bin:/private/tmp/zephcore-tools:/usr/bin:/bin:/usr/sbin:/sbin
PACKETLOGGER := /Applications/PacketLogger.app/Contents/Resources/packetlogger
BLE_TRACE ?= /private/tmp/meshcore_ble.pklg
BLE_TRACE_TXT ?= /private/tmp/meshcore_ble_current.txt
BLE_DIAG ?= 0

# Usage:
#   make build heltec_wifi_lora32_v3/esp32s3/procpu
#   BLE_DIAG=1 make build heltec_wifi_lora32_v3/esp32s3/procpu
#   make ble-diag
#   make ble-decode
#   make flash heltec_wifi_lora32_v3/esp32s3/procpu
#
# The board name is taken from the extra make goal after `build` or `flash`.
BOARD := $(strip $(word 2,$(MAKECMDGOALS)))

.DEFAULT_GOAL := help

.PHONY: help build diag ble-diag ble-decode flash clean

help:
	@printf '%s\n' \
		'Usage:' \
		'  make build <board>' \
		'  BLE_DIAG=1 make build <board>' \
		'  make diag <board>' \
		'  make ble-diag' \
		'  make ble-decode' \
		'  make flash <board>' \
		'' \
		'Example:' \
		'  make build heltec_wifi_lora32_v3/esp32s3/procpu'

build:
	@if [ -z "$(BOARD)" ]; then \
		echo "Usage: make build <board>"; \
		exit 1; \
	fi
	@if [ ! -x "$(WEST)" ]; then \
		echo "Missing $(WEST). Run the Zephyr tool setup first."; \
		exit 1; \
	fi
	@DYLD_LIBRARY_PATH="$(DYLD_EXPAT)" \
	ZEPHYR_SDK_INSTALL_DIR="$(TOOLCHAIN_DIR)" \
	PATH="$(TOOL_PATH)" \
	"$(WEST)" build -b "$(BOARD)" zephcore --pristine --sysbuild
	@$(MAKE) --no-print-directory diag "$(BOARD)"
	@if [ "$(BLE_DIAG)" = "1" ]; then \
		$(MAKE) --no-print-directory ble-diag; \
	fi

diag:
	@if [ -z "$(BOARD)" ]; then \
		echo "Usage: make diag <board>"; \
		exit 1; \
	fi
	@echo ""
	@echo "== ZephCore build diagnostics =="
	@echo "Board: $(BOARD)"
	@if [ -f build/mcuboot/zephyr/zephyr.bin ]; then \
		ls -lh build/mcuboot/zephyr/zephyr.bin; \
	else \
		echo "missing: build/mcuboot/zephyr/zephyr.bin"; \
	fi
	@if [ -f build/zephcore/zephyr/zephyr.signed.bin ]; then \
		ls -lh build/zephcore/zephyr/zephyr.signed.bin; \
	else \
		echo "missing: build/zephcore/zephyr/zephyr.signed.bin"; \
	fi
	@if [ -f build/zephcore/zephyr/.config ]; then \
		echo ""; \
		echo "BLE/security config:"; \
		grep -E '^(# )?CONFIG_(BT_APP_PASSKEY|BT_SMP_ENFORCE_MITM|ZEPHCORE_BLE_PASSKEY|BT_BONDABLE|BT_BONDING_REQUIRED|BT_SETTINGS|BT_PRIVACY|BT_SMP_DISABLE_LEGACY_JW_PASSKEY|ZEPHCORE_COMPANION_USB|ZEPHCORE_COMPANION_UART|UART_CONSOLE)' build/zephcore/zephyr/.config || true; \
	else \
		echo "missing: build/zephcore/zephyr/.config"; \
	fi
	@echo ""
	@echo "Flash map:"
	@echo "  0x0      build/mcuboot/zephyr/zephyr.bin"
	@echo "  0x20000  build/zephcore/zephyr/zephyr.signed.bin"

ble-diag:
	@if [ ! -x "$(PACKETLOGGER)" ]; then \
		echo "Missing PacketLogger: $(PACKETLOGGER)"; \
		exit 1; \
	fi
	@echo "Checking sudo access for BLE capture..."
	@sudo -v
	@echo ""
	@echo "BLE capture is now running:"
	@echo "  $(BLE_TRACE)"
	@echo "Reproduce the BLE failure, then press Ctrl+C here."
	@sudo "$(PACKETLOGGER)" convert -o "$(BLE_TRACE)"

ble-decode:
	@if [ ! -x "$(PACKETLOGGER)" ]; then \
		echo "Missing PacketLogger: $(PACKETLOGGER)"; \
		exit 1; \
	fi
	@if [ ! -f "$(BLE_TRACE)" ]; then \
		echo "Missing capture: $(BLE_TRACE)"; \
		exit 1; \
	fi
	@"$(PACKETLOGGER)" convert -i "$(BLE_TRACE)" -o "$(BLE_TRACE_TXT)" -f impsfl
	@echo "Decoded BLE capture:"
	@echo "  $(BLE_TRACE_TXT)"
	@echo ""
	@grep -nE 'MeshCore|Connection Complete|Disconnection|Encryption|SMP|ATT|Error Response|Write Request|Write Response|Handle Value|6E4000|Security|Pair|Passkey|Authentication|Insufficient' "$(BLE_TRACE_TXT)" || true

flash:
	@if [ -z "$(BOARD)" ]; then \
		echo "Usage: make flash <board>"; \
		exit 1; \
	fi
	@if [ ! -x "$(WEST)" ]; then \
		echo "Missing $(WEST). Run the Zephyr tool setup first."; \
		exit 1; \
	fi
	@DYLD_LIBRARY_PATH="$(DYLD_EXPAT)" \
	ZEPHYR_SDK_INSTALL_DIR="$(TOOLCHAIN_DIR)" \
	PATH="$(TOOL_PATH)" \
	"$(WEST)" flash --build-dir build

clean:
	@rm -rf build

%:
	@:
