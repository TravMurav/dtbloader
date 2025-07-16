# SPDX-License-Identifier: BSD-3-Clause

ARCH		:= aarch64
O		:= $(CURDIR)/build-$(ARCH)

CC		:= clang
LD 		:= lld-link
AR		:= llvm-ar

LIBFDT_DIR = $(CURDIR)/external/dtc/libfdt
LIBFDT  := $(O)/external/libfdt.a

GNUEFI_DIR = $(CURDIR)/external/gnu-efi
LIBEFI  := $(O)/external/libefi.a

LIBSHA1_DIR = $(CURDIR)/external/sha1
LIBSHA1 := $(O)/external/libsha1.a


CFLAGS		:= -target $(ARCH)-windows \
		   -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone \
		   -I$(GNUEFI_DIR)/inc \
		   -I$(LIBFDT_DIR) \
		   -I$(LIBSHA1_DIR) \
		   -I$(CURDIR)/src/include \
		   -g -gcodeview -O2

CFLAGS_SRC	:= -Wall

ifneq ($(DEBUG),)
	CFLAGS  += -DEFI_DEBUG
endif

ifneq ($(ABORT_IF_UNSUPPORTED),)
	CFLAGS  += -DABORT_IF_UNSUPPORTED
endif

CFLAGS		+= -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

LDFLAGS		:= -entry:efi_main -nodefaultlib -debug

DEVICE_SRCS := \
	$(notdir $(shell find $(CURDIR)/src/devices -name '*.c'))

OBJS := \
	$(O)/src/main.o \
	$(O)/src/libc.o \
	$(O)/src/device.o \
	$(O)/src/util.o \
	$(O)/src/chid.o \
	$(O)/src/qcom.o \
	$(DEVICE_SRCS:%.c=$(O)/src/devices/%.o)


all: $(O)/dtbloader.efi

$(O)/dtbloader.efi: $(OBJS) $(LIBEFI) $(LIBFDT) $(LIBSHA1)
	@echo [LD] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) -subsystem:efi_boot_service_driver $^ -out:$@

$(O)/%.o: %.c
	@echo [CC] $(if $(findstring external,$@),\($(word 3,$(subst /, ,$(@:$(CURDIR)%=%)))\) )$(notdir $@)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(if $(findstring external,$@), ,$(CFLAGS_SRC)) -c $< -o $@


.PHONY: clean
clean:
	rm -rf $(O)

#
# GNU-EFI Related rules
#

# NOTE: We drop setjmp and ctors since they are asm code in gnu flavor and clang is upset with that.
LIBEFI_FILES = boxdraw smbios console crc data debug dpath \
        entry error event exit guid hand hw init lock \
        misc pause print sread str cmdline\
	runtime/rtlock runtime/efirtlib runtime/rtstr runtime/vm runtime/rtdata  \
	$(ARCH)/initplat $(ARCH)/math

LIBEFI_OBJS := $(LIBEFI_FILES:%=$(O)/external/gnu-efi/lib/%.o)

$(LIBEFI): $(LIBEFI_OBJS)
	@echo [AR] $(notdir $@)
	@$(AR) rc $@ $(LIBEFI_OBJS)

#
# libfdt related rules
#

LIBFDT_dir = $(LIBFDT_DIR)

include $(LIBFDT_DIR)/Makefile.libfdt
LIBFDT_OBJS := $(LIBFDT_SRCS:%.c=$(O)/external/dtc/libfdt/%.o)

$(LIBFDT): $(LIBFDT_OBJS)
	@echo [AR] $(notdir $@)
	@$(AR) rc $@ $(LIBFDT_OBJS)

#
# sha1 related rules
#

LIBSHA1_OBJS := $(O)/external/sha1/sha1.o

$(LIBSHA1): $(LIBSHA1_OBJS)
	@echo [AR] $(notdir $@)
	@$(AR) rc $@ $(LIBSHA1_OBJS)
