
CROSS_COMPILE 	:= aarch64-linux-gnu-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
OBJCOPY		:= $(CROSS_COMPILE)objcopy

ARCH		:= aarch64
O		:= $(CURDIR)/build-$(ARCH)

GNUEFI_DIR = $(CURDIR)/external/gnu-efi
GNUEFI_OUT = $(GNUEFI_DIR)/$(ARCH)

CFLAGS += \
	-I$(GNUEFI_DIR)/inc/ -I$(GNUEFI_DIR)/inc/$(ARCH) -I$(GNUEFI_DIR)/inc/protocol \
	-fpic -fshort-wchar -fno-stack-protector -ffreestanding \
	-DCONFIG_$(ARCH) -D__MAKEWITH_GNUEFI -DGNU_EFI_USE_MS_ABI \
	-mstrict-align

LDFLAGS += \
	--no-wchar-size-warning \
	-s -Bsymbolic -nostdlib -shared \
	-L $(GNUEFI_OUT)/lib/ -L $(GNUEFI_OUT)/gnuefi \

LIBS = \
	-lefi -lgnuefi \
	-T $(GNUEFI_DIR)/gnuefi/elf_$(ARCH)_efi.lds \
	$(GNUEFI_OUT)/gnuefi/crt0-efi-$(ARCH).o

LIBEFI_A 	:= $(GNUEFI_OUT)/lib/libefi.a
LIBGNUEFI_A 	:= $(GNUEFI_OUT)/gnuefi/libgnuefi.a

OBJS := \
	$(O)/src/main.o \

all: $(LIBEFI_A) $(LIBGNUEFI_A) $(O)/test.efi

$(O)/%.efi: $(O)/%.so
	@echo [OBJCOPY] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .rodata -j .rel \
		    -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* \
		    -j .areloc -j .reloc --target efi-app-$(ARCH) --subsystem=10 $< $@

$(O)/test.so: $(OBJS)
	@echo [LD] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

$(O)/%.o: %.c $(LIBGNUEFI_A)
	@echo [CC] $(notdir $@)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -ffreestanding -c $< -o $@

$(LIBEFI_A): $(LIBGNUEFI_A)
	@echo [ DEP ] $@
	ln -s /usr/include/elf.h $(GNUEFI_DIR)/inc/elf.h
	@$(MAKE) -C$(GNUEFI_DIR) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) lib
	rm $(GNUEFI_DIR)/inc/elf.h

$(LIBGNUEFI_A):
	@echo [ DEP ] $@
	ln -s /usr/include/elf.h $(GNUEFI_DIR)/inc/elf.h
	@$(MAKE) -C$(GNUEFI_DIR) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) gnuefi
	rm $(GNUEFI_DIR)/inc/elf.h

.PHONY: clean
clean:
	rm -rf $(O)
	rm -rf $(GNUEFI_OUT)

