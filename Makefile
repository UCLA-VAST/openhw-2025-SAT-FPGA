# Convenient command shortcuts

EMU_TYPE ?= sw_emu
BUILD_DIR := build/$(EMU_TYPE)

# Default CMake configure flags
CMAKE_FLAGS := -DEMU_TYPE=$(EMU_TYPE)

all: run

.PHONY: configure
configure:
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

.PHONY: compilecl
compilecl: configure
	@$(MAKE) -C $(BUILD_DIR) compilecl

.PHONY: compilekernel
compilekernel: configure
	@$(MAKE) -C $(BUILD_DIR) compilekernel

.PHONY: linkkernel
linkkernel: configure
	@$(MAKE) -C $(BUILD_DIR) linkkernel

.PHONY: run
run: configure
	@$(MAKE) -C $(BUILD_DIR) run

.PHONY: hw_reports
hw_reports:
	@$(MAKE) EMU_TYPE=hw configure
	@$(MAKE) -C build/hw hw_reports

.PHONY: clean clean-all
clean:
	@echo "Cleaning $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

clean-all:
	@echo "Cleaning all build directories"
	@rm -rf build
