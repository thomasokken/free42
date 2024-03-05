# DM42PGM - Free42 frontend implementation for DM42

- There is DMCP interface doc in progress see [DMCP IFC doc](http://technical.swissmicros.com/dmcp/doc/DMCP-ifc-html/) (or
you can download HTML zip file from [doc directory](http://technical.swissmicros.com/dm42/doc/)).

# Architecture Overview
## Structure
The DM Calculator Platform (DMCP) is the OS for the DM42 calculator and comes as a binary file DMCP_flash_X.Y.bin.
The OS runs executable modules like Free42 which have a .pgm extension.
Both binaries are found here [link to binaries](http:/technical.swissmicros.com/dm42/firmware)

## DM42 Menus
The DM42 has two menus, the DMCP Menu and the System Menu.
The System Menu is the regular menu as known from previous versions, see 
[DM42 User Manual](https://technical.swissmicros.com/dm42/doc/dm42_user_manual/)
for details.


### The DMCP Menu features these options:

![dmcp_menu.jpg](/images/dmcp_menu.jpg)
 1. "Program Info" display information of the program loaded in the CPU
 1. "Run Program" runs the program loaded in the CPU
 1. "Load Program" copies an executable module from the FAT partition to the flash memory of the CPU
 1. "Load QSPI from FAT" should not be needed
 1. "Settings" for Time and Date
 1. "Active USB Disk" puts the DM42 in USB mode for file copy
 1. "Enter System Menu" jumps to the main System Menu
 1. "About" displays general information

## Update procedure

1. Active USB disk in the setup menu
1. Copy both [DMCP_flash_X.Y.bin and the .pgm](http://technical.swissmicros.com/dm42/firmware) file from the PC/MAC to the FAT partition of the DM42
1. Eject DM42 from PC/MAC
1. On the DM42, SETUP > System > Flash firmware from FAT.
1. Once finished the calculator resets and displays the DMCP Menu.
1. Choose "Load Program" and select DM-X.Y.Z.pgm
1. Once loaded, select "Run Program" and Free42 starts.

# Building Instructions
Read the README file.
