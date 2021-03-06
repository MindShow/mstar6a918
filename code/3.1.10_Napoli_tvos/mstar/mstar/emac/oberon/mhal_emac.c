////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (��MStar Confidential Information��) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   Mhal_emac.c
/// @brief  EMAC Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------
#include <linux/mii.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/pci.h>
#include "mhal_emac.h"
//-------------------------------------------------------------------------------------------------
//  Data structure
//-------------------------------------------------------------------------------------------------
struct _MHalBasicConfigEMAC
{
    u8                  connected;          // 0:No, 1:Yes    <== (20070515) Wait for Julian's reply
	u8					speed;				// 10:10Mbps, 100:100Mbps
	// ETH_CTL Register:
	u8                  wes;				// 0:Disable, 1:Enable (WR_ENABLE_STATISTICS_REGS)
	// ETH_CFG Register:
	u8					duplex;				// 1:Half-duplex, 2:Full-duplex
	u8					cam;                // 0:No CAM, 1:Yes
	u8 					rcv_bcast;      	// 0:No, 1:Yes
	u8					rlf;                // 0:No, 1:Yes receive long frame(1522)
	// MAC Address:
	u8					sa1[6];				// Specific Addr 1 (MAC Address)
	u8					sa2[6];				// Specific Addr 2
	u8					sa3[6];				// Specific Addr 3
	u8					sa4[6];				// Specific Addr 4
};
typedef struct _MHalBasicConfigEMAC MHalBasicConfigEMAC;

struct _MHalUtilityVarsEMAC
{
    u32                 cntChkCableConnect;
    u32                 cntChkINTCounter;
	u32 				readIdxRBQP;		// Reset = 0x00000000
	u32 				rxOneFrameAddr;		// Reset = 0x00000000 (Store the Addr of "ReadONE_RX_Frame")

	u8                  flagISR_INT_DONE;
};
typedef struct _MHalUtilityVarsEMAC MHalUtilityVarsEMAC;

MHalBasicConfigEMAC MHalThisBCE;
MHalUtilityVarsEMAC MHalThisUVE;

typedef volatile unsigned int EMAC_REG;
typedef struct _TITANIA_EMAC {
// Constant: ----------------------------------------------------------------
// Register MAP:
  EMAC_REG   REG_EMAC_CTL_L; 	      //0x00000000 Network Control Register Low  16 bit
  EMAC_REG   REG_EMAC_CTL_H; 	      //0x00000004 Network Control Register High 16 bit
  EMAC_REG   REG_EMAC_CFG_L; 	      //0x00000008 Network Configuration Register Low  16 bit
  EMAC_REG   REG_EMAC_CFG_H; 	      //0x0000000C Network Configuration Register High 16 bit
  EMAC_REG   REG_EMAC_SR_L; 	      //0x00000010 Network Status Register Low  16 bit
  EMAC_REG   REG_EMAC_SR_H; 	      //0x00000014 Network Status Register High 16 bit
  EMAC_REG   REG_EMAC_TAR_L; 	      //0x00000018 Transmit Address Register Low  16 bit
  EMAC_REG   REG_EMAC_TAR_H; 	      //0x0000001C Transmit Address Register High 16 bit
  EMAC_REG   REG_EMAC_TCR_L; 	      //0x00000020 Transmit Control Register Low  16 bit
  EMAC_REG   REG_EMAC_TCR_H; 	      //0x00000024 Transmit Control Register High 16 bit
  EMAC_REG   REG_EMAC_TSR_L; 	      //0x00000028 Transmit Status Register Low  16 bit
  EMAC_REG   REG_EMAC_TSR_H; 	      //0x0000002C Transmit Status Register High 16 bit
  EMAC_REG   REG_EMAC_RBQP_L;         //0x00000030 Receive Buffer Queue Pointer Low  16 bit
  EMAC_REG   REG_EMAC_RBQP_H;         //0x00000034 Receive Buffer Queue Pointer High 16 bit
  EMAC_REG   REG_EMAC_RBCFG_L;        //0x00000038 Receive buffer configuration Low  16 bit
  EMAC_REG   REG_EMAC_RBCFG_H;        //0x0000003C Receive buffer configuration High 16 bit
  EMAC_REG   REG_EMAC_RSR_L; 	      //0x00000040 Receive Status Register Low  16 bit
  EMAC_REG   REG_EMAC_RSR_H; 	      //0x00000044 Receive Status Register High 16 bit
  EMAC_REG   REG_EMAC_ISR_L;          //0x00000048 Interrupt Status Register Low  16 bit
  EMAC_REG   REG_EMAC_ISR_H;          //0x0000004C Interrupt Status Register High 16 bit
  EMAC_REG   REG_EMAC_IER_L;          //0x00000050 Interrupt Enable Register Low  16 bit
  EMAC_REG   REG_EMAC_IER_H;          //0x00000054 Interrupt Enable Register High 16 bit
  EMAC_REG   REG_EMAC_IDR_L; 	      //0x00000058 Interrupt Disable Register Low  16 bit
  EMAC_REG   REG_EMAC_IDR_H; 	      //0x0000005C Interrupt Disable Register High 16 bit
  EMAC_REG   REG_EMAC_IMR_L; 	      //0x00000060 Interrupt Mask Register Low  16 bit
  EMAC_REG   REG_EMAC_IMR_H; 	      //0x00000064 Interrupt Mask Register High 16 bit
  EMAC_REG   REG_EMAC_MAN_L; 	      //0x00000068 PHY Maintenance Register Low  16 bit
  EMAC_REG   REG_EMAC_MAN_H; 	      //0x0000006C PHY Maintenance Register High 16 bit
  EMAC_REG   REG_EMAC_RBRP_L;         //0x00000070 Receive Buffer First full pointer Low  16 bit
  EMAC_REG   REG_EMAC_RBRP_H;         //0x00000074 Receive Buffer First full pointer High 16 bit
  EMAC_REG   REG_EMAC_RBWP_L;         //0x00000078 Receive Buffer Current pointer Low  16 bit
  EMAC_REG   REG_EMAC_RBWP_H;         //0x0000007C Receive Buffer Current pointer High 16 bit
  EMAC_REG   REG_EMAC_FRA_L; 	      //0x00000080 Frames Transmitted OK Register Low  16 bit
  EMAC_REG   REG_EMAC_FRA_H; 	      //0x00000084 Frames Transmitted OK Register High 16 bit
  EMAC_REG   REG_EMAC_SCOL_L;         //0x00000088 Single Collision Frame Register Low  16 bit
  EMAC_REG   REG_EMAC_SCOL_H;         //0x0000008C Single Collision Frame Register High 16 bit
  EMAC_REG   REG_EMAC_MCOL_L;         //0x00000090 Multiple Collision Frame Register Low  16 bit
  EMAC_REG   REG_EMAC_MCOL_H;         //0x00000094 Multiple Collision Frame Register High 16 bit
  EMAC_REG   REG_EMAC_OK_L; 	      //0x00000098 Frames Received OK Register Low  16 bit
  EMAC_REG   REG_EMAC_OK_H; 	      //0x0000009C Frames Received OK Register High 16 bit
  EMAC_REG   REG_EMAC_SEQE_L;         //0x000000A0 Frame Check Sequence Error Register Low  16 bit
  EMAC_REG   REG_EMAC_SEQE_H;         //0x000000A4 Frame Check Sequence Error Register High 16 bit
  EMAC_REG   REG_EMAC_ALE_L; 	      //0x000000A8 Alignment Error Register Low  16 bit
  EMAC_REG   REG_EMAC_ALE_H; 	      //0x000000AC Alignment Error Register High 16 bit
  EMAC_REG   REG_EMAC_DTE_L; 	      //0x000000B0 Deferred Transmission Frame Register Low  16 bit
  EMAC_REG   REG_EMAC_DTE_H; 	      //0x000000B4 Deferred Transmission Frame Register High 16 bit
  EMAC_REG   REG_EMAC_LCOL_L;         //0x000000B8 Late Collision Register Low  16 bit
  EMAC_REG   REG_EMAC_LCOL_H;         //0x000000BC Late Collision Register High 16 bit
  EMAC_REG   REG_EMAC_ECOL_L;         //0x000000C0 Excessive Collision Register Low  16 bit
  EMAC_REG   REG_EMAC_ECOL_H;         //0x000000C4 Excessive Collision Register High 16 bit
  EMAC_REG   REG_EMAC_TUE_L; 	      //0x000000C8 Transmit Underrun Error Register Low  16 bit
  EMAC_REG   REG_EMAC_TUE_H; 	      //0x000000CC Transmit Underrun Error Register High 16 bit
  EMAC_REG   REG_EMAC_CSE_L;          //0x000000D0 Carrier sense errors Register Low  16 bit
  EMAC_REG   REG_EMAC_CSE_H;          //0x000000D4 Carrier sense errors Register High 16 bit
  EMAC_REG   REG_EMAC_RE_L;           //0x000000D8 Receive resource error Low  16 bit
  EMAC_REG   REG_EMAC_RE_H;           //0x000000DC Receive resource error High 16 bit
  EMAC_REG   REG_EMAC_ROVR_L;         //0x000000E0 Received overrun Low  16 bit
  EMAC_REG   REG_EMAC_ROVR_H;         //0x000000E4 Received overrun High 16 bit
  EMAC_REG   REG_EMAC_SE_L;           //0x000000E8 Received symbols error Low  16 bit
  EMAC_REG   REG_EMAC_SE_H;           //0x000000EC Received symbols error High 16 bit
//  EMAC_REG	 REG_EMAC_CDE; 	      //           Code Error Register
  EMAC_REG   REG_EMAC_ELR_L; 	      //0x000000F0 Excessive Length Error Register Low  16 bit
  EMAC_REG   REG_EMAC_ELR_H; 	      //0x000000F4 Excessive Length Error Register High 16 bit

  EMAC_REG   REG_EMAC_RJB_L; 	      //0x000000F8 Receive Jabber Register Low  16 bit
  EMAC_REG   REG_EMAC_RJB_H; 	      //0x000000FC Receive Jabber Register High 16 bit
  EMAC_REG   REG_EMAC_USF_L; 	      //0x00000100 Undersize Frame Register Low  16 bit
  EMAC_REG   REG_EMAC_USF_H; 	      //0x00000104 Undersize Frame Register High 16 bit
  EMAC_REG   REG_EMAC_SQEE_L; 	      //0x00000108 SQE Test Error Register Low  16 bit
  EMAC_REG   REG_EMAC_SQEE_H; 	      //0x0000010C SQE Test Error Register High 16 bit
//  EMAC_REG	 REG_EMAC_DRFC;       //           Discarded RX Frame Register
  EMAC_REG   REG_Reserved1_L; 	      //0x00000110 Low  16 bit
  EMAC_REG   REG_Reserved1_H; 	      //0x00000114 High 16 bit
  EMAC_REG   REG_Reserved2_L; 	      //0x00000118 Low  16 bit
  EMAC_REG   REG_Reserved2_H; 	      //0x0000011C High 16 bit
  EMAC_REG   REG_EMAC_HSH_L; 	      //0x00000120 Hash Address High[63:32] Low  16 bit
  EMAC_REG   REG_EMAC_HSH_H; 	      //0x00000124 Hash Address High[63:32] High 16 bit
  EMAC_REG   REG_EMAC_HSL_L; 	      //0x00000128 Hash Address Low[31:0] Low  16 bit
  EMAC_REG   REG_EMAC_HSL_H; 	      //0x0000012C Hash Address Low[31:0] High 16 bit

  EMAC_REG   REG_EMAC_SA1L_L;         //0x00000130 Specific Address 1 Low, First 4 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA1L_H;         //0x00000134 Specific Address 1 Low, First 4 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA1H_L;         //0x00000138 Specific Address 1 High, Last 2 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA1H_H;         //0x0000013C Specific Address 1 High, Last 2 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA2L_L;         //0x00000140 Specific Address 2 Low, First 4 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA2L_H;         //0x00000144 Specific Address 2 Low, First 4 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA2H_L;         //0x00000148 Specific Address 2 High, Last 2 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA2H_H;         //0x0000014C Specific Address 2 High, Last 2 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA3L_L;         //0x00000150 Specific Address 3 Low, First 4 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA3L_H;         //0x00000154 Specific Address 3 Low, First 4 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA3H_L;         //0x00000158 Specific Address 3 High, Last 2 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA3H_H;         //0x0000015C Specific Address 3 High, Last 2 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA4L_L;         //0x00000160 Specific Address 4 Low, First 4 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA4L_H;         //0x00000164 Specific Address 4 Low, First 4 bytes High 16 bit
  EMAC_REG   REG_EMAC_SA4H_L;         //0x00000168 Specific Address 4 High, Last 2 bytes Low  16 bit
  EMAC_REG   REG_EMAC_SA4H_H;         //0x0000016C Specific Address 4 High, Last 2 bytes High 16 bit
  EMAC_REG   REG_TAG_TYPE_L; 	      //0x00000170 tag type of the frame Low  16 bit
  EMAC_REG   REG_TAG_TYPE_H; 	      //0x00000174 tag type of the frame High 16 bit
  EMAC_REG   REG_Reserved3[34];       //0x00000178 Low  16 bit
  EMAC_REG   REG_JULIAN_0100_L;       //0x00000200
  EMAC_REG   REG_JULIAN_0100_H;       //0x00000204
  EMAC_REG   REG_JULIAN_0104_L;       //0x00000208
  EMAC_REG   REG_JULIAN_0104_H;       //0x0000020C
  EMAC_REG   REG_JULIAN_0108_L;       //0x00000210
  EMAC_REG   REG_JULIAN_0108_H;       //0x00000214
  /*
  EMAC_REG   REG_CAMA0_l_L; 		  //0x00000210 16 LSB of CAM address  0 Low  16 bit
  EMAC_REG   REG_CAMA0_l_H; 		  //0x00000214 16 LSB of CAM address  0 High 16 bit
  EMAC_REG   REG_CAMA0_h_L; 		  //0x00000218 32 MSB of CAM address  0 Low  16 bit
  EMAC_REG   REG_CAMA0_h_H; 		  //0x0000021C 32 MSB of CAM address  0 High 16 bit
  EMAC_REG   REG_Reserved4[368];      //0x00000220
  EMAC_REG   REG_CAMA62_l_L; 	      //0x000007E0 16 LSB of CAM address 62 Low  16 bit
  EMAC_REG   REG_CAMA62_l_H; 	      //0x000007E4 16 LSB of CAM address 62 High 16 bit
  EMAC_REG   REG_CAMA62_h_L; 	      //0x000007E8 32 MSB of CAM address 62 Low  16 bit
  EMAC_REG   REG_CAMA62_h_H; 	      //0x000007EC 32 MSB of CAM address 62 High 16 bit
  */
} TITANIA_EMAC_Str, *TITANIA_EMAC;

#define MHal_MAX_INT_COUNTER    100
//-------------------------------------------------------------------------------------------------
//  EMAC hardware for Titania
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_WritRam32(u32 uRamAddr, u32 xoffset,u32 xval)
{
	*((u32*)((char*)uRamAddr + xoffset)) = xval;
}

void MHal_EMAC_Write_SA1_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5)
{
	u32 w0 = (u32)m3 << 24 | m2 << 16 | m1 << 8 | m0;
	u32 w1 = (u32)m5 <<  8 | m4;
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;

	regs->REG_EMAC_SA1L_L= w0&0x0000FFFF;
    regs->REG_EMAC_SA1L_H= w0>>16;
	regs->REG_EMAC_SA1H_L= w1&0x0000FFFF;
    regs->REG_EMAC_SA1H_H= w1>>16;
}

void MHal_EMAC_Write_SA2_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5)
{
	u32 w0 = (u32)m3 << 24 | m2 << 16 | m1 << 8 | m0;
	u32 w1 = (u32)m5 <<  8 | m4;
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;

	regs->REG_EMAC_SA2L_L= w0&0x0000FFFF;
    regs->REG_EMAC_SA2L_H= w0>>16;
	regs->REG_EMAC_SA2H_L= w1&0x0000FFFF;
    regs->REG_EMAC_SA2H_H= w1>>16;
}

void MHal_EMAC_Write_SA3_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5)
{
	u32 w0 = (u32)m3 << 24 | m2 << 16 | m1 << 8 | m0;
	u32 w1 = (u32)m5 <<  8 | m4;
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;

	regs->REG_EMAC_SA3L_L= w0&0x0000FFFF;
    regs->REG_EMAC_SA3L_H= w0>>16;
	regs->REG_EMAC_SA3H_L= w1&0x0000FFFF;
    regs->REG_EMAC_SA3H_H= w1>>16;
}

void MHal_EMAC_Write_SA4_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5)
{
	u32 w0 = (u32)m3 << 24 | m2 << 16 | m1 << 8 | m0;
	u32 w1 = (u32)m5 <<  8 | m4;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;

	regs->REG_EMAC_SA4L_L= w0&0x0000FFFF;
    regs->REG_EMAC_SA4L_H= w0>>16;
	regs->REG_EMAC_SA4H_L= w1&0x0000FFFF;
    regs->REG_EMAC_SA4H_H= w1>>16;
}

//-------------------------------------------------------------------------------------------------
//  R/W EMAC register for Titania
//-------------------------------------------------------------------------------------------------

void MHal_EMAC_update_HSH(u8 mc1, u8 mc0)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_HSL_L= mc1&0x0000FFFF;
    regs->REG_EMAC_HSL_H= mc1>>16;
	regs->REG_EMAC_HSH_L= mc0&0x0000FFFF;
    regs->REG_EMAC_HSH_H= mc0>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read control register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_CTL(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_CTL_L)&0x0000FFFF)+((regs->REG_EMAC_CTL_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Network control register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_CTL(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_CTL_L= xval&0x0000FFFF;
    regs->REG_EMAC_CTL_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Network configuration register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_CFG(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_CFG_L)&0x0000FFFF)+((regs->REG_EMAC_CFG_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Network configuration register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_CFG(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_CFG_L= xval&0x0000FFFF;
    regs->REG_EMAC_CFG_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read RBQP
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_RBQP(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_RBQP_L)&0x0000FFFF)+((regs->REG_EMAC_RBQP_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write RBQP
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_RBQP(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_RBQP_L= xval&0x0000FFFF;
    regs->REG_EMAC_RBQP_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Write Transmit Address register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_TAR(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_TAR_L= xval&0x0000FFFF;
    regs->REG_EMAC_TAR_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Write Transmit Control register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_TCR(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_TCR_L= xval&0x0000FFFF;
    regs->REG_EMAC_TCR_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Transmit Status Register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_TSR(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_TSR_L)&0x0000FFFF)+((regs->REG_EMAC_TSR_L)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Read Interrupt status register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_ISR(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_ISR_L)&0x0000FFFF)+((regs->REG_EMAC_ISR_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Read Interrupt enable register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_IER(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_IER_L)&0x0000FFFF)+((regs->REG_EMAC_IER_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Interrupt enable register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_IER(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_IER_L= xval&0x0000FFFF;
    regs->REG_EMAC_IER_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Interrupt disable register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_IDR(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_IDR_L)&0x0000FFFF)+((regs->REG_EMAC_IDR_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Interrupt disable register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_IDR(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_IDR_L= xval&0x0000FFFF;
    regs->REG_EMAC_IDR_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Interrupt mask register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_IMR(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_IMR_L)&0x0000FFFF)+((regs->REG_EMAC_IMR_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Read PHY maintenance register
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_MAN(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_MAN_L)&0x0000FFFF)+((regs->REG_EMAC_MAN_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write PHY maintenance register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_MAN(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_MAN_L= xval&0x0000FFFF;
    regs->REG_EMAC_MAN_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Write Receive Buffer Configuration
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_BUFF(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_RBCFG_L= xval&0x0000FFFF;
    regs->REG_EMAC_RBCFG_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Receive Buffer Configuration
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_BUFF(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_RBCFG_L)&0x0000FFFF)+((regs->REG_EMAC_RBCFG_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Read Receive First Full Pointer
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_RDPTR(void)
{
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_RBRP_L)&0x0000FFFF)+((regs->REG_EMAC_RBRP_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Receive First Full Pointer
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_RDPTR(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_RBRP_L= xval&0x0000FFFF;
    regs->REG_EMAC_RBRP_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Write Receive First Full Pointer
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_WRPTR(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_RBWP_L= xval&0x0000FFFF;
    regs->REG_EMAC_RBWP_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Frames transmitted OK
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_FRA(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_FRA);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_FRA_L)&0x0000FFFF)+((regs->REG_EMAC_FRA_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Single collision frames
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_SCOL(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_SCOL);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_SCOL_L)&0x0000FFFF)+((regs->REG_EMAC_SCOL_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Multiple collision frames
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_MCOL(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_MCOL);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_MCOL_L)&0x0000FFFF)+((regs->REG_EMAC_MCOL_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Frames received OK
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_OK(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_OK);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_OK_L)&0x0000FFFF)+((regs->REG_EMAC_OK_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Frame check sequence errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_SEQE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_SEQE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_SEQE_L)&0x0000FFFF)+((regs->REG_EMAC_SEQE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Alignment errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_ALE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_ALE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_ALE_L)&0x0000FFFF)+((regs->REG_EMAC_ALE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Late collisions
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_LCOL(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_LCOL);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_LCOL_L)&0x0000FFFF)+((regs->REG_EMAC_LCOL_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Excessive collisions
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_ECOL(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_ECOL);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_ECOL_L)&0x0000FFFF)+((regs->REG_EMAC_ECOL_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Transmit under-run errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_TUE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_TUE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_TUE_L)&0x0000FFFF)+((regs->REG_EMAC_TUE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Carrier sense errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_CSE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_CSE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_CSE_L)&0x0000FFFF)+((regs->REG_EMAC_CSE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Receive resource error
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_RE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_RE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_RE_L)&0x0000FFFF)+((regs->REG_EMAC_RE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Received overrun
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_ROVR(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_ROVR);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_ROVR_L)&0x0000FFFF)+((regs->REG_EMAC_ROVR_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Received symbols error
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_SE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_SE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_SE_L)&0x0000FFFF)+((regs->REG_EMAC_SE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Excessive length errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_ELR(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_ELR);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_ELR_L)&0x0000FFFF)+((regs->REG_EMAC_ELR_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Receive jabbers
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_RJB(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_RJB);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_RJB_L)&0x0000FFFF)+((regs->REG_EMAC_RJB_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Undersize frames
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_USF(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_USF);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_USF_L)&0x0000FFFF)+((regs->REG_EMAC_USF_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  SQE test errors
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_SQEE(void)
{
    //return MHal_EMAC_ReadReg32(REG_ETH_SQEE);
    u32 xval;
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    xval = ((regs->REG_EMAC_SQEE_L)&0x0000FFFF)+((regs->REG_EMAC_SQEE_H)<<0x10);
	return xval;
}

//-------------------------------------------------------------------------------------------------
//  Write Julian 100
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_JULIAN_0100(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	//regs->REG_JULIAN_0100= xval&0x0000FFFF;
	regs->REG_JULIAN_0100_L= xval&0x0000FFFF;
    regs->REG_JULIAN_0100_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Julian 104
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_JULIAN_0104(void)
{
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	return ((regs->REG_JULIAN_0104_L)&0x0000FFFF)+((regs->REG_JULIAN_0104_H)<<0x10);
}

//-------------------------------------------------------------------------------------------------
//  Write Julian 104
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_JULIAN_0104(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_JULIAN_0104_L= xval&0x0000FFFF;
    regs->REG_JULIAN_0104_H= xval>>16;
}

//-------------------------------------------------------------------------------------------------
//  Read Julian 108
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_Read_JULIAN_0108(void)
{
    TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	return ((regs->REG_JULIAN_0108_L)&0x0000FFFF)+((regs->REG_JULIAN_0108_H)<<0x10);
}

//-------------------------------------------------------------------------------------------------
//  Write Julian 108
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_Write_JULIAN_0108(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_JULIAN_0108_L= xval&0x0000FFFF;
    regs->REG_JULIAN_0108_H= xval>>16;
}

void MHal_EMAC_Set_Tx_JULIAN_T(u32 xval)
{
}

void MHal_EMAC_Set_TEST(u32 xval)
{
}

u32 MHal_EMAC_Get_Tx_FIFO_Threshold(void)
{
}

void MHal_EMAC_Set_Rx_FIFO_Enlarge(u32 xval)
{
}

u32 MHal_EMAC_Get_Rx_FIFO_Enlarge(void)
{
}

void MHal_EMAC_Set_Miu_Priority(u32 xval)
{
}

u32 MHal_EMAC_Get_Miu_Priority(void)
{
}

void MHal_EMAC_Set_Tx_Hang_Fix_ECO(u32 xval)
{
}

void MHal_EMAC_Set_MIU_Out_Of_Range_Fix(u32 xval)
{
}

void MHal_EMAC_Set_Rx_Tx_Burst16_Mode(u32 xval)
{
}

void MHal_EMAC_Set_Tx_Rx_Req_Priority_Switch(u32 xval)
{
}

void MHal_EMAC_Set_Rx_Byte_Align_Offset(u32 xval)
{
}

void MHal_EMAC_Write_Protect(u32 start_addr, u32 length)
{
}

//-------------------------------------------------------------------------------------------------
//  PHY INTERFACE
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Enable the MDIO bit in MAC control register
// When not called from an interrupt-handler, access to the PHY must be
// protected by a spinlock.
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_enable_mdi(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   regs->REG_EMAC_CTL_L |= EMAC_MPE; //enable management port //
   regs->REG_EMAC_CTL_H &= EMAC_ALLFF;
}

//-------------------------------------------------------------------------------------------------
//  Disable the MDIO bit in the MAC control register
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_disable_mdi(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   regs->REG_EMAC_CTL_L &= ~EMAC_MPE;    // disable management port //
   regs->REG_EMAC_CTL_H &= EMAC_ALLFF;
}

//-------------------------------------------------------------------------------------------------
// Write value to the a PHY register
// Note: MDI interface is assumed to already have been enabled.
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_write_phy (unsigned char phy_addr, unsigned char address, u32 value)
{
   u32 uRegVal;
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   uRegVal =((EMAC_HIGH | EMAC_CODE_802_3 | EMAC_RW_W
            | ((phy_addr & 0x1f) << 23) | (address << 18))) ;
   MHal_EMAC_Write_MAN(uRegVal);
   // Wait until IDLE bit in Network Status register is cleared //
   uRegVal = ((regs->REG_EMAC_SR_L)&0x0000FFFF)+((regs->REG_EMAC_SR_H)<<0x10);

   while (!(uRegVal& EMAC_IDLE))
   {
       uRegVal = ((regs->REG_EMAC_SR_L)&0x0000FFFF)+((regs->REG_EMAC_SR_H)<<0x10);
       barrier ();
   }
}
//-------------------------------------------------------------------------------------------------
// Read value stored in a PHY register.
// Note: MDI interface is assumed to already have been enabled.
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_read_phy(unsigned char phy_addr, unsigned char address,u32 *value)
{
   u32 uRegVal;
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   uRegVal=( EMAC_HIGH | EMAC_CODE_802_3 | EMAC_RW_R
   | ((phy_addr & 0x1f) << 23) | (address << 18));
   MHal_EMAC_Write_MAN(uRegVal);

   //Wait until IDLE bit in Network Status register is cleared //
   uRegVal = ((regs->REG_EMAC_SR_L)&0x0000FFFF)+((regs->REG_EMAC_SR_H)<<0x10);

   while (!(uRegVal & EMAC_IDLE))
   {
       uRegVal = ((regs->REG_EMAC_SR_L)&0x0000FFFF)+((regs->REG_EMAC_SR_H)<<0x10);
       barrier ();
   }
   *value = (MHal_EMAC_Read_MAN() & 0x0000ffff);
}

//-------------------------------------------------------------------------------------------------
// Update MAC speed and H/F duplex
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_update_speed_duplex(u32 uspeed, u32 uduplex)
{
   u32 mac_cfg_L, mac_cfg_H;
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;

   mac_cfg_L = regs->REG_EMAC_CFG_L & ~(EMAC_SPD | EMAC_FD);
   mac_cfg_H = regs->REG_EMAC_CFG_H & ~(EMAC_SPD | EMAC_FD);

   if (uspeed == SPEED_100)
   {
       if (uduplex == DUPLEX_FULL)    // 100 Full Duplex //
       {
           regs->REG_EMAC_CFG_L = mac_cfg_L | EMAC_SPD | EMAC_FD;
       }
       else                           // 100 Half Duplex ///
       {
		   regs->REG_EMAC_CFG_L = mac_cfg_L | EMAC_SPD;
       }
   }
   else
   {
       if (uduplex == DUPLEX_FULL)    //10 Full Duplex //
       {
           regs->REG_EMAC_CFG_L = mac_cfg_L |EMAC_FD;
       }
       else                           // 10 Half Duplex //
       {
           regs->REG_EMAC_CFG_L = mac_cfg_L;
       }
   }
   regs->REG_EMAC_CFG_H &= EMAC_ALLFF;//Write to CFG
}

//-------------------------------------------------------------------------------------------------
//Initialize and enable the PHY interrupt when link-state changes
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_enable_phyirq (void)
{
#if 0

#endif
}

//-------------------------------------------------------------------------------------------------
// Disable the PHY interrupt
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_disable_phyirq (void)
{
#if 0

#endif
}
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------

u32 MHal_EMAC_get_SA1H_addr(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   return (regs->REG_EMAC_SA1H_L&0x0000FFFF) + (regs->REG_EMAC_SA1H_H<<16);
}

u32 MHal_EMAC_get_SA1L_addr(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   return (regs->REG_EMAC_SA1L_L&0x0000FFFF) + (regs->REG_EMAC_SA1L_H<<16);
}

u32 MHal_EMAC_get_SA2H_addr(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   return (regs->REG_EMAC_SA2H_L&0x0000FFFF) + (regs->REG_EMAC_SA2H_H<<16);
}

u32 MHal_EMAC_get_SA2L_addr(void)
{
   TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
   return (regs->REG_EMAC_SA2L_L&0x0000FFFF) + (regs->REG_EMAC_SA2L_H<<16);
}

void MHal_EMAC_Write_SA1H(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_SA1H_L= xval&0x0000FFFF;
    regs->REG_EMAC_SA1H_H= xval>>16;
}

void MHal_EMAC_Write_SA1L(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_SA1L_L= xval&0x0000FFFF;
    regs->REG_EMAC_SA1L_H= xval>>16;
}

void MHal_EMAC_Write_SA2H(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_SA2H_L= xval&0x0000FFFF;
    regs->REG_EMAC_SA2H_H= xval>>16;
}

void MHal_EMAC_Write_SA2L(u32 xval)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
	regs->REG_EMAC_SA2L_L= xval&0x0000FFFF;
    regs->REG_EMAC_SA2L_H= xval>>16;
}

void * MDev_memset(void * s,int c,unsigned long count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

//-------------------------------------------------------------------------------------------------
// Check INT Done
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_CheckINTDone(void)
{
   u32 retIntStatus;
   retIntStatus = MHal_EMAC_Read_ISR();
   MHalThisUVE.cntChkINTCounter = (MHalThisUVE.cntChkINTCounter%MHal_MAX_INT_COUNTER);
   MHalThisUVE.cntChkINTCounter ++;
   if((retIntStatus&EMAC_INT_DONE)||(MHalThisUVE.cntChkINTCounter==(MHal_MAX_INT_COUNTER-1)))
   {
      MHalThisUVE.flagISR_INT_DONE = 0x01;
	  return TRUE;
   }
   return FALSE;
}

//-------------------------------------------------------------------------------------------------
// MAC cable connection detection
//-------------------------------------------------------------------------------------------------
u32 word_ETH_MAN_old = 0;
u32 MHal_EMAC_CableConnection(void)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    u32 retValue      = 0;
    u32 word_ETH_MAN  = 0x00000000;

    u32 word_ETH_CTL  = (regs->REG_EMAC_CTL_L) + (regs->REG_EMAC_CTL_H<<16);

    MHal_EMAC_Write_CTL(0x00000010 | word_ETH_CTL);
    MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;
	MHal_EMAC_Write_MAN(0x60860000);
    while(MHal_EMAC_CheckINTDone()!=1);

	MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;
    word_ETH_MAN = MHal_EMAC_Read_MAN();

    if(word_ETH_MAN_old != word_ETH_MAN)
    {
        word_ETH_MAN_old = word_ETH_MAN;
        printk("EMAC: Link change = %x\n",word_ETH_MAN_old);
    }
    if(word_ETH_MAN & 0x00000004)
	{
        retValue = 1;
    }
	else
	{
        retValue = 0;
    }
	MHal_EMAC_Write_CTL(word_ETH_CTL);
    return(retValue);
}

//-------------------------------------------------------------------------------------------------
// EMAC Negotiation PHY
//-------------------------------------------------------------------------------------------------
u32 MHal_EMAC_NegotiationPHY(void)
{
    // Set PHY --------------------------------------------------------------
    u32 i;
    u32 word_PHY = 0x00000000;
    u32 retValue = 0;

    // IMPORTANT: Get real duplex by negotiation with peer.
    u32 word_ETH_CTL = MHal_EMAC_Read_CTL();
    MHal_EMAC_Write_CTL(0x0000001C | word_ETH_CTL);

    MHalThisUVE.cntChkINTCounter=0;
    word_PHY = 0x50822100;
    MHal_EMAC_Write_MAN(word_PHY);

    while(MHal_EMAC_CheckINTDone()!=1);
	MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;

    word_PHY = 0x509201E1;
    MHal_EMAC_Write_MAN(word_PHY);

    while(MHal_EMAC_CheckINTDone()!=1);
	MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;

    word_PHY = 0x50821200;
    MHal_EMAC_Write_MAN(word_PHY);
    while(MHal_EMAC_CheckINTDone()!=1);
	MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;

    // IMPORTANT: (20070906) There must be some delay (about 2 ~ 3 seconds) between writing 0x57821200 and 0x67FE0000.
    // Otherwise, the later EMAC_WritReg32(REG_ETH_MAN,word_PHY) has no effect.
    MHalThisBCE.duplex = 1;   // Set default as Half-duplex if negotiation fails.
    retValue = 1;

    MHal_EMAC_CableConnection();
	word_PHY = 0x60FE0000;  // Read for 32th reg
    MHal_EMAC_Write_MAN(word_PHY);
    MHalThisUVE.cntChkINTCounter=0;
    while(MHal_EMAC_CheckINTDone()!=1);
	MHalThisUVE.flagISR_INT_DONE = 0x00;
    MHalThisUVE.cntChkINTCounter=0;
	MHalThisUVE.cntChkCableConnect = 0;
    for(i=0;i<100;i++)    // (20071026_CHARLES) No check connection here, if no connect, also init here.
    {
	    word_PHY =  MHal_EMAC_Read_MAN();
	    word_PHY &= 0x0000FFFF;
	    if(word_PHY & 0x00001000) {
	        u32 test1 = (word_PHY & 0x0000001C) >> 2;
	        if(test1 == 0x001 || test1 == 0x002) {
	            MHalThisBCE.duplex = 1;
				retValue = 1;
	        } else {
	            MHalThisBCE.duplex = 2;
				retValue = 2;
	        } // else
	        //retValue = 1;
	        break;
	    } // if
	    mdelay(1);
		MHalThisUVE.cntChkCableConnect++;
        if(MHalThisUVE.cntChkCableConnect==MAX_INT_COUNTER)
			break;
	} // for
    // NOTE: REG_ETH_CFG must be set according to new ThisBCE.duplex.
	MHal_EMAC_Write_CTL(word_ETH_CTL);
    // Set PHY --------------------------------------------------------------
    return(retValue);
}

//-------------------------------------------------------------------------------------------------
// EMAC Hardware register set
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// EMAC Timer set for Receive function
//-------------------------------------------------------------------------------------------------
void MHal_EMAC_timer_callback(unsigned long value)
{
	TITANIA_EMAC regs = (TITANIA_EMAC) REG_ADDR_BASE;
    regs->REG_EMAC_IER_L |= (EMAC_INT_RCOM );
    regs->REG_EMAC_IER_H &= EMAC_ALLFF;
}

//-------------------------------------------------------------------------------------------------
// EMAC clock on/off
//-------------------------------------------------------------------------------------------------
void EMAC_special_reg(unsigned long addr, unsigned long value, unsigned long mask)
{
    unsigned long old_value = *((volatile unsigned long*)((char*)addr));
    unsigned long new_value = old_value & (~mask);
    value = value & (mask);
    *((volatile unsigned long*)((char*)addr)) = new_value | value;
    printk("set addr[%08x or %08x]=from %08x to %08x\n",addr,(addr-0xbf800000)/4,old_value,*((volatile unsigned long*)((char*)addr)));
}
#define EMAC_USE_RMII 1
void MHal_EMAC_Power_On_Clk(void)
{
#if 1
    //set pad
    unsigned long tmp=*((volatile unsigned long*)((char*)0xbf803d00));
    tmp=(tmp&0xffffffe0)|0x12;
    *((volatile unsigned long*)((char*)0xbf803d00))=tmp;
    tmp=*((volatile unsigned long*)((char*)0xbf803d00));
#endif

#if EMAC_USE_RMII
    EMAC_special_reg(0xbf803c14,0x9000,0xFC00);//RMII: F05:        00000000 to 00009000//must
#else
    EMAC_special_reg(0xbf803c14,0x9000,0x0000);//MII: F05:        00000000 to 00009000//must
#endif
    EMAC_special_reg(0xbf803c18,0x0800,0xFFFFFFFF);     // F06:        00000c00 to 00000800//not need
    EMAC_special_reg(0xbf803c54,0x0000,0xFFFFFFFF);     // F15:        00000001 to 00000000//not need
    EMAC_special_reg(0xbf803c1c,0x0000,0xFFFFFFFF);     // F07:        00000008 to 00000000//not need
    EMAC_special_reg(0xbf803c68,0x0000,0x1E00);// F1A:        00000201 to 00000000//must
    EMAC_special_reg(0xbf803c6c,0x0000,0x3C00);// F1B:        00000400 to 00000000//must
#if EMAC_USE_RMII
    EMAC_special_reg(0xbf806c00,0x0007,0xFFFFFFFF);//RMII:emac_reg(100): 00000001 to 00000007//must
#else
    EMAC_special_reg(0xbf806c00,0x0001,0xFFFFFFFF);//MII:emac_reg(100): 00000001 to 00000007//must
#endif
    EMAC_special_reg(0xbf806c04,0x0000,0xFFFFFFFF);//emac_reg(101): 00000001 to 00000000//must
}

void MHal_EMAC_Power_Off_Clk(void)
{

}

