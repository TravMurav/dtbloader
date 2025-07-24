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
- <!-- x1e80100 --> ASUS Vivobook S 15
- <!-- x1p42100 --> ASUS Zenbook A14 UX3407QA
- *<!-- x1e80100 --> Dell Latitude 7455*
- <!-- x1e80100 --> Dell XPS 13 9345
- <!-- x1e80100 --> HP Omnibook X 14
- <!-- sc8280xp --> Huawei Matebook E Go
- <!-- sc8180x  --> Lenovo Flex 5G
- <!-- msm8998 --> Lenovo Miix 630
- <!-- x1e78100 --> Lenovo ThinkPad T14s Gen 6
- <!-- sc8280xp --> Lenovo ThinkPad X13s Gen 1
- <!-- sc8180x  --> Lenovo Yoga 5G
- <!-- sdm850   --> Lenovo Yoga C630
- <!-- x1e80100 --> Lenovo Yoga Slim 7x
- <!-- sc8280xp --> Microsoft Surface Pro 9 5G
- *<!-- x1e80100 --> Microsoft Surface Pro 11*
- <!-- sc8280xp --> Microsoft Windows Dev Kit 2023
- <!-- x1e001de --> QTI Snapdragon Devkit for Windows (x1e)

(Note: Devices marked with *italic* may not be available in upstream Linux or linux-next yet.)

### Adding new devices

To add a new device, run `scripts/describe_hw.sh` on the target device and place the output into a new file
in `src/devices`. The script will automatically generate a template device description for you. To specify
which dtb should be used, run it like this:

```
$ sudo scripts/describe_hw.sh -d "qcom/sc8280xp-lenovo-thinkpad-x13s.dtb"
```

## Building

Make sure you have submodules:

```
git submodule update --init --recursive
```

Then build the project:

```
make -j$(nproc)
```

Use `make DEBUG=1` to enable additional log messages.

Note that dtbloader uses `clang` and `lld` to be built. You may also need additional tools from `llvm` package.

## Usage

Some bootloaders such as systemd-boot provide driver boot directory. If you use sd-boot, you may place
`dtbloader.efi` to ESP as `/EFI/systemd/drivers/dtbloaderaa64.efi` (where `aa64` is the arch suffix).

To instead manually install dtbloader, copy the file to ESP and use `efibootmgr -rcl "dtbloader.efi" -L "dtbloader" -d /dev/nvme0n1 -p 12`
(replacing 12 with your EFI System Partition number or making `/dev/nvme0n1` match your install disk)
to add it to the beginning of `DriverOrder`.

Alternatively boot into efi shell, then:

```
# Switch to the ESP partition (replace fs0 with yours)
Shell:\> fs0:
# Install the driver into the boot order
fs0:\> bcfg driver add 1 dtbloader.efi "dtbloader"
# On some devices you may need to add optional data
# to the driver entry for firmware to load it.
fs0:\> echo none > tmp.txt
fs0:\> bcfg driver -opt 1 tmp.txt
fs0:\> rm tmp.txt
```

dtbloader will look for the dtb files in the partition it was installed on. It will look into:
`/dtbloader/dtbs/`; `dtbs/`; `/` in order of priority.

> [!WARNING]
> Some WoA devices keep full bootloader chain on the same eMMC/UFS as the OS. Make sure to never tamper with
> bootloader related partitions.

> [!TIP]
> If SecureBoot is enabled, dtbloader will save the DTB hash into an uefi varialbe and show a warning
> in case the hash changes. Hash will be updated automatically after.

### EFI Variables

dtbloader saves some EFI variables that can be used by bootloaders or other tools. All variables are created
under the namespace defined by dtbloader's GUID: `{96176C01-5F3F-47C9-9F85-5632F602F611}`.

#### `DtbName` - Name of the DTB that shall be loaded.

This variable stores the canonical filename of the dtb, such as `qcom\sc8280xp-lenovo-thinkpad-x13s.dtb`.
The variable is encoded using two-byte UCS2 characters and MAY contain a terminating NULL character.
Dtbloader does not set Non-Volatile flag to this variable and will not override it if it was already set.

The suggested use for this variable is implementing "devicetree directory" features in a bootloader. Bootloader
may use this variable in combination with a dtb directory path to load correct dtb for a given boot option.

> [!TIP]
> If you wish to use a bootloader or tool that uses this variable but wish not to use dtbloader (or override
> it's decision), you can set it manually. For example, using EFI Shell:
>
> ```
> Shell:\> setvar -guid 96176C01-5F3F-47C9-9F85-5632F602F611 -bs -rt -nv =L"qcom\sc8280xp-lenovo-thinkpad-x13s.dtb"
> ```
>
> Note that the `-nv` flag will save the variable into non-volatile storage.

## Acknowledgements

This project is a reimplementation of the original [DtbLoader.efi](https://github.com/aarch64-laptops/edk2/tree/dtbloader-app)
from the aarch64-laptops project. However while the original implementation used specially-named dtb files to discover
the compatible dtb, this one keeps a database of compatible devices to match to the conventional dtb name.
