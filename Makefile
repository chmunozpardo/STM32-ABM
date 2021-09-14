# Compilers
GCC = @arm-none-eabi-gcc
GPP = @arm-none-eabi-g++

# ELF Filename
EXEC_FILE = main.elf

# Source directories
SRC_DIR = Src
SRC_DIR += lib/FATFS/src
SRC_DIR += lib/LWIP/src
SRC_DIR += Drivers/STM32F2xx_HAL_Driver/Src
SRC_DIR += Middlewares/Third_Party/FatFs/src
SRC_DIR += Middlewares/Third_Party/FatFs/src/option
SRC_DIR += Middlewares/Third_Party/LwIP/src/api
SRC_DIR += Middlewares/Third_Party/LwIP/src/apps/httpd
SRC_DIR += Middlewares/Third_Party/LwIP/src/apps/mqtt
SRC_DIR += Middlewares/Third_Party/LwIP/src/core
SRC_DIR += Middlewares/Third_Party/LwIP/src/core/ipv4
SRC_DIR += Middlewares/Third_Party/LwIP/src/core/ipv6
SRC_DIR += Middlewares/Third_Party/LwIP/src/netif
SRC_DIR += Middlewares/Third_Party/LwIP/src/netif/ppp

# Include directories
INC_DIR = Include
INC_DIR += Drivers/STM32F2xx_HAL_Driver/Inc
INC_DIR += Drivers/STM32F2xx_HAL_Driver/Inc/Legacy
INC_DIR += Drivers/CMSIS/Device/ST/STM32F2xx/Include
INC_DIR += Drivers/CMSIS/Include
INC_DIR += Middlewares/Third_Party/LwIP/system
INC_DIR += Middlewares/Third_Party/LwIP/src/include
INC_DIR += Middlewares/Third_Party/LwIP/src/include/netif/ppp
INC_DIR += Middlewares/Third_Party/LwIP/src/include/lwip
INC_DIR += Middlewares/Third_Party/LwIP/src/include/lwip/apps
INC_DIR += Middlewares/Third_Party/LwIP/src/include/lwip/priv
INC_DIR += Middlewares/Third_Party/LwIP/src/include/lwip/prot
INC_DIR += Middlewares/Third_Party/LwIP/src/include/netif
INC_DIR += Middlewares/Third_Party/LwIP/src/include/posix
INC_DIR += Middlewares/Third_Party/LwIP/src/include/posix/sys
INC_DIR += Middlewares/Third_Party/LwIP/system/arch
INC_DIR += Middlewares/Third_Party/LwIP/src/apps/httpd
INC_DIR += Middlewares/Third_Party/FatFs/src
INC_DIR += FATFS/Target
INC_DIR += FATFS/App
INC_DIR += LWIP/App
INC_DIR += LWIP/Target
INC_DIR += lib/FATFS/src
INC_DIR += lib/LWIP/src

# Build directory
OBJ_DIR = build

# Source files
SRC_FILES_TEMP = $(foreach var, $(SRC_DIR), $(wildcard $(var)/*.c))
SRC_FILES = $(filter-out Middlewares/Third_Party/LwIP/src/apps/httpd/fsdata_custom.c, $(SRC_FILES_TEMP))
SRC_CPP_FILES = $(foreach var, $(SRC_DIR), $(wildcard $(var)/*.cpp))

# Object files
OBJ_FILES = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_FILES))
OBJ_CPP_FILES = $(patsubst %.cpp, $(OBJ_DIR)/%.opp, $(SRC_CPP_FILES))
OBJ_ASM_FILES = $(OBJ_DIR)/startup_stm32f207zgtx.o

# Include folders
INCLUDES = $(patsubst %, -I%, $(INC_DIR))

# Compiler flags
ASMFLAGS = -mcpu=cortex-m3 -g3 -DDEBUG -c -x assembler-with-cpp --specs=nano.specs -mfloat-abi=soft -mthumb
GCCFLAGS = -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F207xx -c -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfloat-abi=soft -mthumb
GPPFLAGS = -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F207xx -c -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfloat-abi=soft -mthumb -fno-exceptions -fno-rtti -fno-use-cxa-atexit
LDFLAGS = -mcpu=cortex-m3 -T"STM32F207ZGTX_FLASH.ld" -Wl,-Map="main.map" -Wl,--gc-sections -static -u _printf_float --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group

all: $(EXEC_FILE)
	@echo "Finished!"

$(OBJ_DIR)/Middlewares/Third_Party/LwIP/src/apps/httpd/fs.o: Middlewares/Third_Party/LwIP/src/apps/httpd/fs.c Middlewares/Third_Party/LwIP/src/apps/httpd/fsdata_custom.c
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(GCC) $< -MMD -MP -MF$(patsubst %.o, %.d,$@) $(GCCFLAGS) -MT$@ $(INCLUDES) -c -o $@

$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(GCC) $< -MMD -MP -MF$(patsubst %.o, %.d,$@) $(GCCFLAGS) -MT$@ $(INCLUDES) -c -o $@

$(OBJ_DIR)/%.opp: %.cpp
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(GPP) $< -MMD -MP -MF$(patsubst %.o, %.d,$@) $(GPPFLAGS) -MT$@ $(INCLUDES) -c -o $@

$(OBJ_ASM_FILES): Src/startup_stm32f207zgtx.s
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(GCC) $< -MMD -MP -MF$(patsubst %.o, %.d,$@) $(ASMFLAGS) -MT$@ -o $@

$(EXEC_FILE): $(OBJ_FILES) $(OBJ_CPP_FILES) $(OBJ_ASM_FILES)
	@echo "Linking $@"
	$(GPP) $(LDFLAGS) $^ -o $@

clean:
	rm -rf $(OBJ_DIR)