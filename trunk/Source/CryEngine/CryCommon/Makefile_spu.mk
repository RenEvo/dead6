#TODO roadmap:

# - identify placed for potential stalls in cache miss handler and insert patching for code paging there
#		-	check for work (first test if it really stalls, for instance atomic write backs)

#special makefile for *_spu.cpp and SPUDriver_spu.cpp in particular

INCLUDE_DIR_SPU		= $(PROJECT_HOME)/Code/Tools/PS3JobManager
JOBMAN_PATH_INC		= -I$(INCLUDE_DIR_SPU)/SPU -I$(INCLUDE_DIR_SPU)/PPU
CRY_COMMON_PATH		= $(PROJECT_HOME)/Code/CryEngine/CryCommon
OBJECT_DIR_SPU		= $(PROJECT_HOME)/../WorkInProgress/michaelg/PS3JobManager/PS3JobManager
JOBPARSER					= $(OUTPUT_DIR)JobParser.exe
BUBBLEGEN					= $(OUTPUT_DIR)BubbleGen.exe
SPU_DRIVER_PARSER	= $(OUTPUT_DIR)SPUDriverMemManagerParser
CELL_SDK_VERSION	?= 0_8_x
CELL_SDK		?= $(PROJECT_HOME)/Code/SDKs/PS3/$(CELL_SDK_VERSION)
HOST_SYSTEM ?= win32
REPLACE_HBR = perl $(CELL_SDK)/host-$(HOST_SYSTEM)/bin/replace_hbr.pl 

export OUTPUT_DIR

EXECUTE_SYMBOL	= _Z7ExecutePv
export EXECUTE_SYMBOL

SN_PS3_PATH = "c:/Program Files/SN Systems/PS3"
export SN_PS3_PATH

#switch bubble mode on/off
#EXEC_MODE_BUBBLES=-DUSE_BUBBLES
EXEC_MODE_BUBBLES=

#support sn systems debugging on/off
#SN_MODE=-DSUPP_SN
SN_MODE=

#switch profiling on/off
#SPU_PROF = -DDO_SPU_PROFILING
SPU_PROF = 

#enable explicite and all other branch hints on/off
SUPP_BRANCH_HINTS = -DSUPP_BRANCH_HINTS
#SUPP_BRANCH_HINTS =

#switch asm cache miss handler on/off
SPU_CACHE_MISS_USE_ASM = -DSPU_CACHE_MISS_USE_ASM
#SPU_CACHE_MISS_USE_ASM = 

#reset asm impl. of cache miss handler for profiling, not implemented there
ifeq "$(SPU_PROF)" "-DDO_SPU_PROFILING"
	SPU_CACHE_MISS_USE_ASM = 
endif

VEC_FLAGS = -DNO_VECIDX

PPU_SPU_COM_FLAGS = $(SN_MODE) $(VEC_FLAGS) $(EXEC_MODE_BUBBLES) $(SPU_PROF) $(CHECK_BUB_HAZARD) $(SPU_CACHE_MISS_USE_ASM)
export PPU_SPU_COM_FLAGS
export EXEC_MODE_BUBBLES

ifeq "$(EXEC_MODE_BUBBLES)" "-DUSE_BUBBLES"
  CXXFLAGS_BASE_OPT	= 
	COMPLETE_BUILD_DEF = -DTEMP_BUBBLE
	EXEC_ENTRY_POINT_PRE_BUILD = 
else
  CXXFLAGS_BASE_OPT	= -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,-zmuldefs
	COMPLETE_BUILD_DEF = -DTEMP_JOB -DTEMP_BUBBLE	
	EXEC_ENTRY_POINT_PRE_BUILD = -Wl,-entry=$(EXECUTE_SYMBOL)
endif
export COMPLETE_BUILD_DEF

DR_I_F = -include $(INCLUDE_DIR_SPU)/SPU/SPUJob.h

#entry points are not required, set 0 instead to avoid a warning

BUILD_SPU_DEBUG	= COMMAND_LINE='$(CXX_SPU) $(CXXFLAGS_DEBUG_SPU) -D_SPU_JOB -I$(INCLUDE_DIR_SPU)/SPU $(CXXFLAGS_BASE_OPT) $(EXEC_ENTRY_POINT_PRE_BUILD) -include $(INCLUDE_DIR_SPU)/SPU/SPUJob.h'\
						BUBBLE_GEN=$(BUBBLEGEN)\
						EXEC_MODE_BUBBLES=$(EXEC_MODE_BUBBLES)\
						BUBBLE_PREPARE=$(OUTPUT_DIR)/bubble_prepare_spu\
						BUBBLE_LINK='$(CXX_SPU) $(CXXFLAGS_DEBUG_SPU) -nostartfiles'\
						PROJECT_OUTPUT_PATH=$(PROJECT_OUTPUT_PATH)\
					  NM='$(NM)'\
					  $(SHELL) $(OUTPUT_DIR)build_spu '$(OUTPUT_DIR)'
BUILD_SPU_RELEASE	= COMMAND_LINE='$(CXX_SPU) $(CXXFLAGS_RELEASE_SPU) -D_SPU_JOB -I$(INCLUDE_DIR_SPU)/SPU $(CXXFLAGS_BASE_OPT) $(EXEC_ENTRY_POINT_PRE_BUILD) -include $(INCLUDE_DIR_SPU)/SPU/SPUJob.h'\
						BUBBLE_GEN=$(BUBBLEGEN)\
						EXEC_MODE_BUBBLES=$(EXEC_MODE_BUBBLES)\
						BUBBLE_PREPARE=$(OUTPUT_DIR)/bubble_prepare_spu\
						BUBBLE_LINK='$(CXX_SPU) $(CXXFLAGS_RELEASE_SPU) -nostartfiles'\
						PROJECT_OUTPUT_PATH=$(PROJECT_OUTPUT_PATH)\
					  NM='$(NM)'\
					  $(SHELL) $(OUTPUT_DIR)build_spu '$(OUTPUT_DIR)'

SPU_BIN_DIR				= $(CELL_SDK)/host-$(HOST_SYSTEM)/spu/bin
#SPU_BIN_DIR			= $(CELL_SDK)/host-$(HOST_SYSTEM)/spu/bin_old
CXX_SPU_NO_FPIC		= $(SPU_BIN_DIR)/spu-lv2-g++ $(PPU_SPU_COM_FLAGS)
CXX_SPU						= $(CXX_SPU_NO_FPIC) -fPIC -D__LV2_KERNEL__
CCC_SPU						= $(SPU_BIN_DIR)/spu-lv2-gcc -fPIC -D__LV2_KERNEL__ $(PPU_SPU_COM_FLAGS)
CXX_SPU_FILT			= $(SPU_BIN_DIR)/spu-lv2-c++filt
OBJCOPY						= $(CELL_SDK)/host-$(HOST_SYSTEM)/ppu/bin/ppu-lv2-objcopy
NM								= $(SPU_BIN_DIR)/spu-lv2-nm
STRIP_SPU					= $(SPU_BIN_DIR)/spu-lv2-strip --strip-unneeded
EMBEDSPU					= $(OBJCOPY) -B powerpc -I binary -O elf64-powerpc-celloslv2 --set-section-align .data=7 --set-section-pad .data=16

SILENT_TAG=@

COMMON_LIB_PATH_SPU = -L$(OUTPUT_DIR) -L$(CELL_SDK)/target-ceb/spu/lib/ -L$(CELL_SDK)/target-ceb/spu/lib/gcc/spu-lv2/3.4.1/ -L$(SN_PS3_PATH)/spu/lib/sn/
EXCLUDE_DIR_COMMON	= 

INCLUDE_FLAGS_COMMON_SPU = -I. -I../ -I../../ -I$(CELL_SDK)/target/spu/include \
	-I$(CELL_SDK)/host-$(HOST_SYSTEM)/spu/lib/gcc/spu-lv2/3.4.1/include \
  -I$(CELL_SDK)/target/spu/include/vectormath/cpp -I$(CELL_SDK)/target/spu/include/sys
ifeq "$(SN_MODE)" "-DSUPP_SN"
	INCLUDE_FLAGS_COMMON_SPU += -I$(SN_PS3_PATH)/spu/include/sn
endif
INCLUDE_FLAGS_COMMON_SPU += $(JOBMAN_PATH_INC) -I$(CRY_COMMON_PATH)
													 
CXX_WARNING_FLAGS_SPU = -Wstrict-aliasing=2 -Wformat -Wmissing-braces -Wreturn-type -Wno-multichar -Wsequence-point -Wno-invalid-offsetof
CXX_WARNING_FLAGS_SPU = -w
													 
CXXFLAGS_COMMON_SPU		= -fno-rtti -fno-exceptions -D_LIB $(INCLUDE_FLAGS_COMMON_SPU) $(INCLUDE_FLAGS) $(CXX_WARNING_FLAGS_SPU) -DPS3 -D_C_IN_GLOBAL -D__LV2_KERNEL__ -Wno-ctor-dtor-privacy -D_REENTRANT

COMMON_LIBS_SPU				= -ldma -lstdc++

COMMON_DEFINES_SPU		= -D_SPU -D__SPU__ -DPS3 
DEBUG_DEFINES_SPU			= $(COMMON_DEFINES_SPU) -D_DEBUG 
RELEASE_DEFINES_SPU		= $(COMMON_DEFINES_SPU) -DNDEBUG

#change only in sync with PS3JobManager lib
SPU_ASSERT_OPTS_RELEASE			= -D_NO_SPU_CACHE_ASSERT -D_NO_SPU_CACHE_HAZ_CHECK -D_NO_SPU_ASSERT
SPU_ASSERT_OPTS_DEBUG				= -DCHECK_BUB_HAZARD -D_NO_SPU_CACHE_HAZ_CHECK

CXXFLAGS_RELEASE_SPU_COMMON = $(CXXFLAGS_COMMON_SPU) -O3 -std=c++98 -ffast-math -finline-functions-called-once \
	-fgcse-after-reload $(RELEASE_DEFINES_SPU) -fomit-frame-pointer -fpermissive $(SPU_ASSERT_OPTS_RELEASE)
CXXFLAGS_RELEASE_SPU_DRIVER = $(CXXFLAGS_RELEASE_SPU_COMMON) -fstrict-aliasing $(SUPP_BRANCH_HINTS)
CXXFLAGS_RELEASE_SPU				= $(CXXFLAGS_RELEASE_SPU_COMMON) $(SUPP_BRANCH_HINTS) -falign-functions=128

CXXFLAGS_DEBUG_SPU					= $(CXXFLAGS_COMMON_SPU) $(DEBUG_DEFINES_SPU) -fpermissive $(SPU_ASSERT_OPTS_DEBUG)

DebugPS3/SPU/SPUElf32.x:
	@mkdir -p ./DebugPS3/SPU
	@echo Buidling linker script for SPUDriver_spu
	@$(BUILD) \
	  $(CCC_SPU) $(INCLUDE_DIR_SPU)/SPU $(INCLUDE_FLAGS_COMMON_SPU) \
	  -DSET_STACK_PTR -E -P $(INCLUDE_DIR_SPU)/SPU/SPUElf32.h -o $@

ProfilePS3/SPU/SPUElf32.x:
	@mkdir -p ./ProfilePS3/SPU
	@echo Buidling linker script for SPUDriver_spu
	$(BUILD) \
	  $(CCC_SPU) $(INCLUDE_DIR_SPU)/SPU $(INCLUDE_FLAGS_COMMON_SPU) \
	  -DSET_STACK_PTR -E -P $(INCLUDE_DIR_SPU)/SPU/SPUElf32.h -o $@

DebugPS3/SPU/MissHandler_spu.x: DebugPS3/SPU/SPUMemManager_spu.o
	@echo MissHandler_spu.S
	@$(CCC_SPU) $(SPU_ASSERT_OPTS_DEBUG) -DPS3 -D__SPU__ SPU/CodePage/MissHandler_spu.S -c -o DebugPS3/SPU/MissHandler_spu.x
	@cp DebugPS3/SPU/MissHandler_spu.x DebugPS3/SPU/MissHandler_spu.o
	
ProfilePS3/SPU/MissHandler_spu.x: ProfilePS3/SPU/SPUMemManager_spu.o
	@echo MissHandler_spu.S
	@$(CCC_SPU) $(SPU_ASSERT_OPTS_RELEASE) -DPS3 -D__SPU__ SPU/CodePage/MissHandler_spu.S -c -o ProfilePS3/SPU/MissHandler_spu.x
	@cp ProfilePS3/SPU/MissHandler_spu.x ProfilePS3/SPU/MissHandler_spu.o

DebugPS3/SPU/DoCacheLookup_spu.x: DebugPS3/SPU/SPUMemManager_spu.o
	@echo DoCacheLookup_spu.S
	@$(CCC_SPU) $(SPU_ASSERT_OPTS_DEBUG) -DPS3 -D__SPU__ SPU/Cache/DoCacheLookup_spu.S -c -o DebugPS3/SPU/DoCacheLookup_spu.x
	@cp DebugPS3/SPU/DoCacheLookup_spu.x DebugPS3/SPU/DoCacheLookup_spu.o
	
ProfilePS3/SPU/DoCacheLookup_spu.x: ProfilePS3/SPU/SPUMemManager_spu.o
	@echo DoCacheLookup_spu.S
	@$(CCC_SPU) $(SPU_ASSERT_OPTS_RELEASE) -DPS3 -D__SPU__ SPU/Cache/DoCacheLookup_spu.S -c -o ProfilePS3/SPU/DoCacheLookup_spu.x
	@cp ProfilePS3/SPU/DoCacheLookup_spu.x ProfilePS3/SPU/DoCacheLookup_spu.o

#copy always over the other one since the driver address needs to be the correct one
#invalidate both libraries to force relinking too
DebugPS3/SPU/SPUDriver_spu.o: SPUDriver_spu.cpp DebugPS3/SPU/SPUElf32.x \
	DebugPS3/SPU/SPUMemManager_spu.o DebugPS3/SPU/DoCacheLookup_spu.x DebugPS3/SPU/MissHandler_spu.x
	@$(BUILD) --echo $(notdir $<) \
	  $(CXX_SPU_NO_FPIC) $(CXXFLAGS_DEBUG_SPU) $(DR_I_F) -O0 -ffunction-sections $< -c \
	  -o DebugPS3/SPU/SPUDriver_spu.obj
	@$(BUILD) \
	  $(CXX_SPU_NO_FPIC) $(SPU_ASSERT_OPTS_DEBUG) $(CXXFLAGS_BASE_OPT) $(DR_I_F) -Wl,-d -Wl,-nostdlib -Wl,--gc-sections -nostartfiles -Wl,-T,DebugPS3/SPU/SPUElf32.x -Wl,-entry=main DebugPS3/SPU/SPUDriver_spu.obj -o DebugPS3/SPU/SPUDriver_inc_debug.elf \
	  DebugPS3/SPU/SPUMemManager_spu.o DebugPS3/SPU/DoCacheLookup_spu.o DebugPS3/SPU/MissHandler_spu.o $(COMMON_LIBS_SPU)
	$(SPU_DRIVER_PARSER) "DebugPS3/SPU/SPUDriver_inc_debug.elf" "$(INCLUDE_DIR_SPU)/SPU/SPUJob.h"
ifeq "$(SN_MODE)" "-DSUPP_SN"
	$(SN_PS3_PATH)/bin/spumodgen DebugPS3/SPU/SPUDriver_inc_debug.elf DebugPS3/SPU/SPUDriver_inc_debug.elf
endif
	$(SILENT_TAG)$(EMBEDSPU) --redefine-sym _binary_$(subst -,_,$(subst /,_,DebugPS3/SPU/SPUDriver_inc_debug))_elf_start=SPUDriver DebugPS3/SPU/SPUDriver_inc_debug.elf DebugPS3/SPU/SPUDriver_spu.o
	@cp DebugPS3/SPU/SPUDriver_spu.o ProfilePS3/SPU/SPUDriver_spu.o
	@$(NM) -n -td DebugPS3/SPU/SPUDriver_inc_debug.elf | grep "CodePagingCallMissHandler" | awk '{print $$1}' > $(OUTPUT_DIR)/CPMHAddr.txt
	@$(NM) -n -td DebugPS3/SPU/SPUDriver_inc_debug.elf | grep "CodePagingReturnMissHandler" | awk '{print $$1}' > $(OUTPUT_DIR)/CPRHAddr.txt
#	@rm -f $(OUTPUT_DIR)lib$(OUTPUT).a
#	@rm -f DebugPS3/SPU/SPUDriver_inc_debug.elf

ProfilePS3/SPU/SPUDriver_spu.o: SPUDriver_spu.cpp ProfilePS3/SPU/SPUElf32.x \
	ProfilePS3/SPU/SPUMemManager_spu.o ProfilePS3/SPU/DoCacheLookup_spu.x ProfilePS3/SPU/MissHandler_spu.x
	@$(BUILD) --echo $(notdir $<) \
	  $(CXX_SPU_NO_FPIC) $(CXXFLAGS_RELEASE_SPU_DRIVER) $(DR_I_F) -save-temps -dA -ffunction-sections $< -c -o ProfilePS3/SPU/SPUDriver_spu.obj
	@rm -f ./SPUDriver_spu.ii
	@cat ./SPUDriver_spu.s | $(CXX_SPU_FILT) > ProfilePS3/SPU/SPUDriver_spu.asm
	@rm -f ./SPUDriver_spu.s	
	@$(BUILD) \
		$(CXX_SPU_NO_FPIC) $(SPU_ASSERT_OPTS_RELEASE) $(CXXFLAGS_BASE_OPT) $(DR_I_F) -Wl,-d -Wl,-nostdlib -Wl,--gc-sections -nostartfiles -Wl,-T,ProfilePS3/SPU/SPUElf32.x -Wl,-entry=main \
		ProfilePS3/SPU/SPUDriver_spu.obj -o ProfilePS3/SPU/SPUDriver_spu_inc_release.elf \
	  ProfilePS3/SPU/SPUMemManager_spu.o ProfilePS3/SPU/DoCacheLookup_spu.o ProfilePS3/SPU/MissHandler_spu.o $(COMMON_LIBS_SPU)
ifneq "$(SUPP_BRANCH_HINTS)" "-DSUPP_BRANCH_HINTS"
	$(REPLACE_HBR) ProfilePS3/SPU/SPUDriver_spu_inc_release.elf
endif	
	$(SPU_DRIVER_PARSER) "ProfilePS3/SPU/SPUDriver_spu_inc_release.elf" "$(INCLUDE_DIR_SPU)/SPU/SPUJob.h"
	@$(NM) -n --demangle ProfilePS3/SPU/SPUDriver_spu.obj | grep -v -e '\( A \| U \|__static_initialization_and_destruction_0\|global constructors\)' > ProfilePS3/SPU/SPUDriver_spu.s
	@cat ProfilePS3/SPU/SPUDriver_spu.asm >> ProfilePS3/SPU/SPUDriver_spu.s
	@rm -f ProfilePS3/SPU/SPUDriver_spu.asm
#	$(SILENT_TAG) $(STRIP_SPU) --keep-symbol=g_sMemMan ProfilePS3/SPU/SPUDriver_spu_inc_release.elf
	$(SILENT_TAG)$(EMBEDSPU) --redefine-sym _binary_$(subst -,_,$(subst /,_,ProfilePS3/SPU/SPUDriver_spu_inc_release))_elf_start=SPUDriver ProfilePS3/SPU/SPUDriver_spu_inc_release.elf $@
	@cp ProfilePS3/SPU/SPUDriver_spu.o DebugPS3/SPU/SPUDriver_spu.o	
	@$(NM) -n -td ProfilePS3/SPU/SPUDriver_spu_inc_release.elf | grep "CodePagingCallMissHandler" | awk '{print $$1}' > $(OUTPUT_DIR)/CPMHAddr.txt
	@$(NM) -n -td ProfilePS3/SPU/SPUDriver_spu_inc_release.elf | grep "CodePagingReturnMissHandler" | awk '{print $$1}' > $(OUTPUT_DIR)/CPRHAddr.txt	
#	@rm -f $(OUTPUT_DIR)lib$(OUTPUT)_debug.a
#	@rm -f ProfilePS3/SPU/SPUDriver_spu_inc_release.elf

DebugPS3/SPU/SPUMemManager_spu.o: SPUMemManager_spu.cpp
	@mkdir -p ./DebugPS3/SPU
	@$(BUILD) --echo $(notdir $<) \
	  $(CXX_SPU_NO_FPIC) $(CXXFLAGS_DEBUG_SPU) $(DR_I_F) -O0 $< -c -o DebugPS3/SPU/SPUMemManager_spu.o
	@mkdir -p ./ProfilePS3/SPU		  
	@cp DebugPS3/SPU/SPUMemManager_spu.o ProfilePS3/SPU/SPUMemManager_spu.o

ProfilePS3/SPU/SPUMemManager_spu.o: SPUMemManager_spu.cpp
	@mkdir -p ./ProfilePS3/SPU
	@$(BUILD) --echo $(notdir $<) \
	  $(CXX_SPU_NO_FPIC) $(CXXFLAGS_RELEASE_SPU_DRIVER) $(DR_I_F) -save-temps $< -c -o ProfilePS3/SPU/SPUMemManager_spu.o
	@rm -f ./SPUMemManager_spu.ii
#	@cat ./SPUMemManager_spu.s | $(CXX_SPU_FILT) > ProfilePS3/SPU/SPUMemManager_spu.asm
	@cat ./SPUMemManager_spu.s > ProfilePS3/SPU/SPUMemManager_spu.asm
	@rm -f ./SPUMemManager_spu.s
	@$(NM) -n --demangle ProfilePS3/SPU/SPUMemManager_spu.o | grep -v -e '\( A \| U \|__static_initialization_and_destruction_0\|global constructors\)' > ProfilePS3/SPU/SPUMemManager_spu.s
	@cat ProfilePS3/SPU/SPUMemManager_spu.asm >> ProfilePS3/SPU/SPUMemManager_spu.s
	@rm -f ProfilePS3/SPU/SPUMemManager_spu.asm
	@mkdir -p ./DebugPS3/SPU
	@cp ProfilePS3/SPU/SPUMemManager_spu.o DebugPS3/SPU/SPUMemManager_spu.o

DEPENDENCY_FILE = Makefile.depend
FILELIST_BODY	= $(subst .dirs,.files_, $(wildcard *dirs))

#entry point for usual ps3 jobs must be Execute (generated automatically)
#external symbol is filename without _spu.cpp, elf data are renamed to $(OUTPUT)_(base filename)
#make dependent on SPUDriver since this compilation sets entry point of memory manager
#compile order:
#		- compile job file just to get compile errors (use -shared to not output warning about missing entry point)
#		- invoke Jobparser to create PPU include file and SPU execution file
#		- compile SPU execution file (with target original _spu.o - name), define EXECUTE_SYMBOL as entry point and copy obj to object file
CXX_SPU_JOB_OPT_FLAGS_DEBUG = -O0

DebugPS3/SPU/%_spu.o: %_spu.cpp $(INCLUDE_DIR_SPU)/SPU/SPUJob.h
	@mkdir -p ./DebugPS3/SPU
	@echo $(notdir $<)
	$(SILENT_TAG) $(JOBPARSER) $< $(OUTPUT) $(DEPENDENCY_FILE) $(FILELIST_BODY) DebugPS3/SPU/
	@mv -f $(subst _spu,_ppu_include.h, $(basename $<)) DebugPS3/SPU
	$(BUILD_SPU_DEBUG) DebugPS3 $(subst _spu.cpp,, $(notdir $<))\
		$(CXX_SPU) $(CXXFLAGS_DEBUG_SPU) $(CXX_SPU_JOB_OPT_FLAGS_DEBUG) -save-temps -D_SPU_JOB -I$(INCLUDE_DIR_SPU)/SPU $(CXXFLAGS_BASE_OPT)\
			-include $(INCLUDE_DIR_SPU)/SPU/SPUJob.h -nostartfiles $(COMPLETE_BUILD_DEF) $(EXEC_ENTRY_POINT_PRE_BUILD)\
			-o DebugPS3/SPU/$(basename $(notdir $<))_inc_debug.elf $(basename $<)_helper.cpp
ifneq "$(SUPP_BRANCH_HINTS)" "-DSUPP_BRANCH_HINTS"
	$(REPLACE_HBR) DebugPS3/SPU/$(basename $(notdir $<))_inc_debug.elf
endif	
	@rm -f ./$(basename $(notdir $<))_helper.ii		
	@mv -f ./$(basename $(notdir $<))_helper.s DebugPS3/SPU/$(basename $(notdir $<))_helper.S
	@mv -f $(subst _spu,_spu_helper.cpp, $(basename $<)) DebugPS3/SPU		  
	@mv -f $(subst _spu,_spu_helper.o, $(basename $<)) DebugPS3/SPU		  	
ifeq "$(EXEC_MODE_BUBBLES)" ""		
	$(SILENT_TAG)if ! $(NM) DebugPS3/SPU/$(basename $(notdir $<))_inc_debug.elf | grep $(EXECUTE_SYMBOL) > /dev/null; then \
		echo Entry symbol $(EXECUTE_SYMBOL) not found in $(basename $(notdir $<))_inc_debug.elf; exit 1; else :;fi
	$(SILENT_TAG)$(EMBEDSPU) --redefine-sym _binary_$(subst -,_,$(subst /,_,DebugPS3/SPU/$(addsuffix _inc_debug, $(basename $(notdir $<)))))_elf_start=$(OUTPUT)_$(basename $(notdir $<)) DebugPS3/SPU/$(addsuffix _inc_debug.elf, $(basename $(notdir $<))) $@
endif	
	@rm -f $(OUTPUT_DIR)$(PROJECT_OUTPUT_PATH)/Debug/Bubbles/$(subst _spu,,$(basename $(notdir $<)).elf)	
	@mv -f DebugPS3/SPU/$(basename $(notdir $<))_inc_debug.elf $(OUTPUT_DIR)$(PROJECT_OUTPUT_PATH)/Debug/Bubbles/$(subst _spu,,$(basename $(notdir $<)).elf)

ProfilePS3/SPU/%_spu.o: %_spu.cpp $(INCLUDE_DIR_SPU)/SPU/SPUJob.h
	@mkdir -p ./ProfilePS3/SPU
	@echo $(notdir $<)	
	$(SILENT_TAG) $(JOBPARSER) $< $(OUTPUT)	$(DEPENDENCY_FILE) $(FILELIST_BODY) ProfilePS3/SPU/
	@mv -f $(subst _spu,_ppu_include.h, $(basename $<)) ProfilePS3/SPU	
	$(BUILD_SPU_RELEASE) ProfilePS3 $(subst _spu.cpp,, $(notdir $<))\
		$(CXX_SPU) $(CXXFLAGS_RELEASE_SPU) -save-temps -D_SPU_JOB -I$(INCLUDE_DIR_SPU)/SPU $(CXXFLAGS_BASE_OPT)\
		-include $(INCLUDE_DIR_SPU)/SPU/SPUJob.h -nostartfiles $(COMPLETE_BUILD_DEF) $(EXEC_ENTRY_POINT_PRE_BUILD)\
		-o ProfilePS3/SPU/$(basename $(notdir $<))_inc_release.elf $(basename $<)_helper.cpp
ifneq "$(SUPP_BRANCH_HINTS)" "-DSUPP_BRANCH_HINTS"
	$(REPLACE_HBR) ProfilePS3/SPU/$(basename $(notdir $<))_inc_release.elf
endif	
	@rm -f ./$(basename $(notdir $<))_helper.ii		
	@mv -f ./$(basename $(notdir $<))_helper.s ProfilePS3/SPU/$(basename $(notdir $<))_helper.S
	@mv -f $(subst _spu,_spu_helper.cpp, $(basename $<)) ProfilePS3/SPU		  
	@mv -f $(subst _spu,_spu_helper.o, $(basename $<)) ProfilePS3/SPU		  	
ifeq "$(EXEC_MODE_BUBBLES)" ""	
	$(SILENT_TAG)if ! $(NM) ProfilePS3/SPU/$(basename $(notdir $<))_inc_release.elf | grep $(EXECUTE_SYMBOL) > /dev/null; then \
		echo Entry symbol $(EXECUTE_SYMBOL) not found in $(basename $(notdir $<))_inc_release.elf; exit 1; else :;fi
	$(SILENT_TAG)$(EMBEDSPU) --redefine-sym _binary_$(subst -,_,$(subst /,_,ProfilePS3/SPU/$(addsuffix _inc_release, $(basename $(notdir $<)))))_elf_start=$(OUTPUT)_$(basename $(notdir $<)) ProfilePS3/SPU/$(addsuffix _inc_release.elf, $(basename $(notdir $<))) $@
endif	
	@rm -f $(OUTPUT_DIR)$(PROJECT_OUTPUT_PATH)/Release/Bubbles/$(subst _spu,,$(basename $(notdir $<)).elf)
	@mv -f ProfilePS3/SPU/$(basename $(notdir $<))_inc_release.elf $(OUTPUT_DIR)$(PROJECT_OUTPUT_PATH)/Release/Bubbles/$(subst _spu,,$(basename $(notdir $<)).elf)
