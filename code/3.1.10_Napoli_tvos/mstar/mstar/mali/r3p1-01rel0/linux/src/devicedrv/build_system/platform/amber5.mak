#
# Copyright (C) 2010-2012 MStar Semiconductor, Inc. All rights reserved.
# 
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
# 
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# Amber5 platform options
ARCH=mips
TARGET_PLATFORM=mstar
MSTAR_PLATFORM=AMBER5
USING_RIU=1
RIU_ADDRESS_TYPE=16
GPU_CLOCK=276
PHYS_TO_BUS_ADDRESS_ADJUST=0x40000000

# Linux kernel config checking
CONFIG_CHIP_NAME=amber5
CHECK_CONFIG_CHIP_NAME=1