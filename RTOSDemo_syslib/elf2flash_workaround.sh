#!/bin/sh
#
# fix for path failure in nios2-ide generate
#
 elf2flash --input=RTOSDemo.elf --flash= --boot="/usr/local/stow/altera8.0/i
p/altera/nios2_ip/altera_nios2/boot_loader_cfi.srec" --outfile=ext_flash.flash --sim_optimize=0 --base=0x4000000 --end=0x4FFFFFF --reset=0x0 
