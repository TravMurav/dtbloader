# dtbloader

dtbloader is an EFI driver that finds and installs DeviceTree into the UEFI configuration table.

Since most existing Windows-on-Arm devices focus on Windows, they use ACPI to boot. In many cases
though this ACPI is hard or impossible to use with Linux so instead DT should be used. dtbloader
attemtps to simplify running Linux-based or other OS that use DT by providing:

- **Device detection** that uses DMI to pick which DTB should be used.
- **Device-specific DT fixups** to allow tailoring generic dtb to a given device (i.e. set hw variants, MAC...)
- **DT Fixup protocol** to allow bootloaders like sd-boot to load their own dtb while still applying the fixups.

## Supported devices

<!--
  We keep this list simple and just alphabetical order for now but
  add soc to each device in a comment so it's easier to split per-soc later.
-->

- <!-- sc7180   --> Acer Aspire 1
- <!-- sc8180x  --> Lenovo Flex 5G
- <!-- sc8280xp --> Lenovo ThinkPad X13s Gen 1
- <!-- sdm850   --> Lenovo Yoga C630

### Adding new devices

To add a new device, run `scripts/describe_hw.sh` on the target device and place the output into a new file
in `src/devices`. The script will automatically generate a template device description for you. To specify
which dtb should be used, run it like this:

```
$ sudo scripts/describe_hw.sh -d "qcom/8280xp-lenovo-thinkpad-x13s.dtb"
```

## Building

Make sure you have submodules:

```
git submodule update --init --recursive
```

Then build the project:

```
make
```

Note that dtbloader requires `clang` to be built.

## Usage

To manually install dtbloader, copy the file to ESP, boot into efi shell, then:

```
# Switch to the ESP partition (replace fs0 with yours)
Shell:\> fs0:
# Install the driver into the boot order
fs0:\> bcfg driver add 1 dtbloader.efi "dtbloader"
```

Alternatively, some bootloaders such as systemd-boot provide driver boot directory. If you use sd-boot,
you may place `dtbloader.efi` to ESP as `/EFI/systemd/drivers/dtbloaderaa64.efi` (where `aa64` is the arch suffix).

dtbloader will look for the dtb files in the partition it was installed on. It will look into:
`/dtbloader/dtbs/`; `dtbs/`; `/` in order of priority.

> [!WARNING]
> Some WoA devices keep full bootloader chain on the same eMMC/UFS as the OS. Make sure to never tamper with
> bootloader related partitions.

> [!TIP]
> If SecureBoot is enabled, dtbloader will save the DTB hash into an uefi varialbe and show a warning
> in case the hash changes. Hash will be updated automatically after.

## Acknowledgements

This project is a reimplementation of the original [DtbLoader.efi](https://github.com/aarch64-laptops/edk2/tree/dtbloader-app)
from the aarch64-laptops project. However while the original implementation used specially-named dtb files to discover
the compatible dtb, this one keeps a database of compatible devices to match to the conventional dtb name.
