######################################
# target
######################################
TARGET = DM42rv

######################################
# building variables
######################################
# debug build?
ifdef DEBUG
DEBUG = 1
endif

#######################################
# pathes
#######################################
# Build path
BUILD_DIR = build
# Free42 header files
FREE42DIR = free42

# Path to aux build scripts (including trailing /)
# Leave empty for scripts in PATH
BIN_DIR = bin/

######################################
# source
######################################
C_SOURCES = dm/ini.c
ASM_SOURCES = \
  dmcp/startup_pgm.s

#######################################
# Custom section
#######################################
C_SOURCES += dmcp/sys/pgm_syscalls.c
C_INCLUDES += -Idm -Idmcp

# Free42 frontend
CXX_SOURCES += dm/dm42_fns.cc
CXX_SOURCES += dm/dm42_menu.cc
CXX_SOURCES += dm/dm42_ext.cc
C_DEFS += -DARM -DDM42
C_DEFS += -DDECIMAL_CALL_BY_REFERENCE=1 -DDECIMAL_GLOBAL_ROUNDING=1 \
  -DDECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS=1 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS=1 \
  -DDECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS=1 \
  -DBCD_MATH
C_INCLUDES += -I$(FREE42DIR)

# Free42 libraries
ifeq ($(DEBUG), 1)
LIBS += lib/libfree42_bcd.a lib/gcc111libbid_hard.a
else
LIBS += lib/libfree42_bcd_rel.a lib/gcc111libbid_hard.a
endif
# ---

#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
AR = arm-none-eabi-ar
SIZE = arm-none-eabi-size
HEX = $(OBJCOPY) -O ihex
BIN = $(OBJCOPY) -O binary -S

#######################################
# CFLAGS
#######################################
# macros for gcc
AS_DEFS =
C_DEFS += -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))"
AS_INCLUDES =


CPUFLAGS += -mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16


# compile gcc flags
ASFLAGS = $(CPUFLAGS) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS  = $(CPUFLAGS) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += -Wno-misleading-indentation
DBGFLAGS = -g 

ifeq ($(DEBUG), 1)
CFLAGS += -O0
else
CFLAGS += -O2 -DFW_RELEASE
endif

CFLAGS  += $(DBGFLAGS)
LDFLAGS += $(DBGFLAGS)

# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = stm32_program.ld
# libraries
#LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS += $(CPUFLAGS) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -specs=nano.specs
LDFLAGS += -Wl,--gc-sections -Wl,--wrap=_malloc_r


# default action: build all
# $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex
all: $(BUILD_DIR)/$(TARGET).elf 

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# C++ sources
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(CXX_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cc Makefile | $(BUILD_DIR) 
	$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cc=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LIBS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(OBJCOPY) --remove-section .qspi -O ihex    $@  $(BUILD_DIR)/$(TARGET)_flash.hex
	$(OBJCOPY) --remove-section .qspi -O binary  $@  $(BUILD_DIR)/$(TARGET)_flash.bin
	$(OBJCOPY) --only-section   .qspi -O ihex    $@  $(BUILD_DIR)/$(TARGET)_qspi.hex
	$(OBJCOPY) --only-section   .qspi -O binary  $@  $(BUILD_DIR)/$(TARGET)_qspi.bin
	$(BIN_DIR)check_qspi_crc $(TARGET) dm/qspi_crc.h || ( $(MAKE) clean && false )
	$(BIN_DIR)add_pgm_chsum build/$(TARGET)_flash.bin build/$(TARGET).pgm
	$(SIZE) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@
	
$(BUILD_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)/*.o $(BUILD_DIR)/*.lst

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

.PHONY: clean all

# *** EOF ***
