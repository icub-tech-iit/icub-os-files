/*
 * Linux device driver for iCub CFW002
 *
 * Written by Andrea Merello <andrea.merello@iit.it>
 * Copytight 2010 Robotcub Consortium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef CFW002_HW_H
#define CFW002_HW_H


#define CFW_PLX_MBOX0	0x40
#define CFW_PLX_MBOX1	0x44
#define CFW_PLX_MBOX2	0x48
#define CFW_PLX_MBOX3	0x4C
#define CFW_PLX_MBOX4	0x50
#define CFW_PLX_MBOX5	0x54
#define CFW_PLX_MBOX6	0x58
#define CFW_PLX_MBOX7	0x5C
#define CFW_PLX_LDOOR	0x60
#define CFW_PLX_PDOOR	0x64


#define CFW_IGEN	CFW_PLX_LDOOR /* generates an interrupt on the CFW2 */
#define CFW_ISR		CFW_PLX_PDOOR /* PCI interrupt status register */
#define CFW_CMD0	CFW_PLX_MBOX0
#define CFW_CMD1	CFW_PLX_MBOX1
#define CFW_PARAM0	CFW_PLX_MBOX4
#define CFW_PARAM1	CFW_PLX_MBOX5

#if 0
#define CFW_RXCONSIDX0	CFW_PLX_MBOX2
#define CFW_RXCONSIDX1	CFW_PLX_MBOX3
#define CFW_RXPRODIDX0	CFW_PLX_MBOX6
#define CFW_RXPRODIDX1	CFW_PLX_MBOX7
#endif

#define CFW_CMD_NONE			0
#define CFW_CMD_SETTXDMA0		1
#define CFW_CMD_SETTXDMA1		2
#define CFW_CMD_SETTXDMA2		3
#define CFW_CMD_SETTXDMA3		4
#define CFW_CMD_SETTXDMA4		5
#define CFW_CMD_SETRXDMA0		6
#define CFW_CMD_SETTXRINGSZ		7
#define CFW_CMD_SETRXRINGSZ		8
#define CFW_CMD_RTXENABLE		9
#define CFW_CMD_INTENABLE		10
#define CFW_CMD_SETAUDIOGAIN		11
#define CFW_CMD_SETRXDMA1		12
#define CFW_CMD_SETRXDMA2		13
#define CFW_CMD_SETRXDMA3		14
#define CFW_CMD_SETRXDMA4		15
#define CFW_CMD_TXISPEND		16
#define CFW_CMD_SETRXFILTER		17
#define CFW_CMD_PREPARERXFILTERMASK	18
#define CFW_CMD_PREPARERXFILTERID	19
#define CFW_CMD_SETIDFILTER		20
#define CFW_CMD_PORTENABLE		21

#define CFW_CMD_DEBUG		0xde

#define PCI_ISR_RXOK_MASK1	0x1F
#define PCI_ISR_TXOK_MASK1	(0x1F << 5) /*0x3e0*/
#define PCI_ISR_RXOK_MASK2	(0x1F << 16)
#define PCI_ISR_TXOK_MASK2	(0x1F << 21) /*0x3e0*/
#define PCI_ISR_RXOK1		1
#define PCI_ISR_RXOK2		(1<<16)
#define PCI_ISR_TXOK1		0x20
#define PCI_ISR_TXOK2		(0x20 << 16)

#define PCI_ISR_RXERR1	0x7C00
#define PCI_ISR_RXERR2	(0x7C00<<16)
#define PCI_ISR_TXERR	0xF800



/* SW header from driver */
struct cfw002_header{
	__u8 port;
	__u8 block;
	struct cfw002_rtxdesc *bufp;
} __attribute__((packed));

/* SW data from driver */
struct cfw002_rtxdesc {
	__le32 id;
	__le16 len;
	u8 data[8];
	/*u16 pad;*/
	
}  __attribute__((packed));


/* HW firmware descriptors */
struct cfw002_txdesc {
	__le32 id;
	__le16 len;
	u8 data[8];
	u16 pad;
	
}  __attribute__((packed));

#define cfw002_rxdesc cfw002_txdesc


struct cfw002_errstate {
	__le16 tec;
	__le16 rec;
	__le16 state;
	__le16 txc;
	__le16 rxc;
	__le16 rxov;
} __attribute__((packed));


/*
struct cfw002_rxdesc {
	__le32 id;
	__le16 len;
	u8 data[8];
	u16 pad;
	
	
}  __attribute__((packed));
*/
#define TX_DESC_SIZE sizeof(struct cfw002_txdesc)
#define RX_DESC_SIZE sizeof(struct cfw002_rxdesc)

#if 0
#define SHM_TXPROD 0x1FE0
#define SHM_TXCONS 0x1FEA
#define SHM_RXCONS 0x1FF4
#define SHM_RXPROD 0x1FD6
#endif

#define CFW_SHM_TXPROD 0xFFE0
#define CFW_SHM_TXCONS 0xFFEA
#define CFW_SHM_RXCONS 0xFFF4
#define CFW_SHM_RXPROD 0xFFD6
#define CFW_SHM_DEBUG 0xFE00
#define CFW_SHM_STATS 0xF000
#define CFW_SHM_STAT_REC (CFW_SHM_STATS)
#define CFW_SHM_STAT_TEC (CFW_SHM_STATS + 0xA)
#define CFW_SHM_STAT_STATE (CFW_SHM_STATS+0xA*2)
#define CFW_SHM_STAT_TXC (CFW_SHM_STATS+0xA*3)
#define CFW_SHM_STAT_RXC (CFW_SHM_STATS+0xA*4)
#define CFW_SHM_STAT_RXOV (CFW_SHM_STATS+0xA*5)



#define TX_DESC_NUM 256
#define RX_DESC_NUM 4096

#define CFW_CMD_TIMEOUT 200
#define CFW_FLUSH_TIMEOUT (HZ*2)
#define CFW_FLUSH_TIMEOUT2 20

#define CFW_BOOT_TIMEOUT 20
#define CFW_SHM_BOOT_CHKSUM 0xC5FD
#define CFW_SHM_BOOT_FLAG 0xFFFE
#define CFW_SHM_BOOT_FLAG_READY 0xDEAD
#define CFW_SHM_BOOT_FLAG_MAGIC 0xC0DE
#define CFW_SHM_BOOT_FLAG_OK 0xF0CA
#define CFW_SHM_BOOT_FLAG_BOOT 0x5A5A
#define CFW_SHM_BOOT_IMAGE 0x2
#define CFW_SHM_BOOT_LEN 0

#endif
