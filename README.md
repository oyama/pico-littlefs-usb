# Raspberry Pi Pico littlefs USB Flash Memory Interface

This project demonstrates mounting littlefs from a host PC via USB, allowing easy retrieval of accumulated sensor data and other information stored on microcontrollers from a standard PC.

Littlefs is widely used as a reliable file system for microcontrollers. However, since this file system is not supported by ordinary host PCs, special software and procedures are required for file read and write operations. The core idea of the project is to add an intermediate conversion layer on the USB Mass Storage Class device driver of the microcontroller to mimic littlefs as a FAT file system. This allows littlefs to be manipulated from the host PC as if it were a USB flash drive with a common FAT file system. Currently, only read access is available from the host PC. It may be possible to support writing as well, but it is still unclear if it can be done seamlessly.
The only footprint added to the microcontroller is RAM that holds the first 3 blocks (3x512 bytes) of FAT12.

## Demonstration Overview

The demo works as follows:

- Each time the BOOTSEL button is clicked, the number of clicks is added to the `SENSOR.TXT` file in the littlefs file system.
- The first time the BOOTSEL button is clicked on Pico, it starts as a USB stick.
- When the Pico is connected to the host PC via USB, it will be mounted as a read-only USB flash drive with a FAT12 file system.
- Holding down the BOOTSEL button for 10 seconds will format the littlefs file system.

## Build and Installation

Prepare a single Raspberry Pi Pico or Pico W, and a build environment using the [pico-sdk](https://github.com/raspberrypi/pico-sdk).

```bash
git submodule update --init
cd lib/pico-sdk; git submodule update --init; cd ../../
cd lib/littlefs; git submodule update --init; cd ../../

mkdir build; cd build
cmake ..
make
```

After successful compilation, you will have `littlefs-usb.uf2`. Simply drag and drop it onto your Raspberry Pi Pico to install and run the application.

## Limitations

Current implementation has several limitations:

- Read-only Access from USB: The USB interface supports read-only access to the littlefs.
- Limited File Size: File sizes are limited due to various constraints.
- No file update detection: The host PC cannot notice that the microcontroller has updated the file while it is mounted. Remounting will reflect the update.
- Visibility of Root Directory Files: Only files with short filenames (8+3) placed directly in the littlefs root directory are visible from the USB side.
- Unrefactored Source Code: The source code has not undergone refactoring.

Some limitations arise from unimplemented features, while others may not be feasible. For example, it is theoretically possible to add files unilaterally from the host PC, as in Pico's BOOTSEL mode, but a seamless implementation is not known to be feasible. Capacity constraints stem from the limited onboard flash (2MB) and the FAT12 file system (256KB). Implementing larger storage and FAT16 or FAT32 file systems would mitigate these limitations.

## How to mimic

FAT12 is a very simple file system and can be easily mimicked; depending on the location of the block device requested by the USB host, the microcontroller assembles and returns the appropriate FAT12 block.

- Block 0: `mimic_fat_boot_sector()`. Returns static FAT12 boot block.
- Block 1: `mimic_fat_table()`. Returns a mimicked FAT table, which searches the littlefs root directory and constructs information about the storage location on the FAT file system based on the file size of each file.
- Block 2: `mimic_fat_root_dir_entry()`. Returns the mimicked root directory entry; searches the littlefs root directory and registers the FAT file entry.
- Block 3 and later: `mimic_fat_file_entry()`. Returns a file block in littlefs. Searches the FAT root directory entry that holds the requested block and gets the file name and offset on littlefs. opens the file in littlefs and returns the data at the offset position.

## Reference

- [littlefs](https://github.com/littlefs-project/littlefs)
- [TyniUSB](https://docs.tinyusb.org/en/latest/)
