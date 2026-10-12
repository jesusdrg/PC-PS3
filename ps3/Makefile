#---------------------------------------------------------------------------------
# ps3link — servidor para el PS3 (PSL1GHT). Basado en la plantilla de los samples.
#
#   make        → ps3link.self
#   make pkg    → ps3link.pkg (instalar una vez desde el XMB)
#   make deploy → sube EBOOT.BIN por FTP (webMAN MOD) a la app ya instalada
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include $(PSL1GHT)/ppu_rules

TARGET		:=	ps3link
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
# ../common contiene el protocolo compartido con la laptop.
INCLUDES	:=	../common

TITLE		:=	ps3link
APPID		:=	PS3L00001
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000

PS3_HOST	?=	10.0.0.2

CFLAGS		=	-O2 -Wall -mcpu=cell $(MACHDEP) $(INCLUDE)
CXXFLAGS	=	$(CFLAGS) -std=gnu++17
LDFLAGS		=	$(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS	:=	-lnet -lsysutil -lsysmodule -lrt -llv2 -lm

LIBDIRS	:=

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)
export BUILDDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.S=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES), -I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					$(LIBPSL1GHT_INC) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					$(LIBPSL1GHT_LIB)

.PHONY: $(BUILD) clean pkg deploy

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).fake.self $(OUTPUT)*.pkg

pkg: $(BUILD) $(OUTPUT).pkg

deploy: pkg
	@PS3_HOST=$(PS3_HOST) APPID=$(APPID) $(CURDIR)/deploy.sh $(BUILDDIR)/pkg/USRDIR/EBOOT.BIN

else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf:	$(OFILES)

-include $(DEPENDS)

endif
