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


#ifndef CFW002_H
#define CFW002_H

#include "cfw002_hw.h"
#include <linux/wait.h>
#include <linux/debugfs.h>

// Added for kernel 3.2 compatibility
// Versions before 3 use init_MUTEX, the newer use sema_init
// ( originally from RT patch, now as standard)
#ifndef init_MUTEX
  #define init_MUTEX(x) sema_init(x,1)
#endif

#define PCI_VENDOR_ID_IIT 0x11f0

#define CFW002_MAJOR 280
#define CFW002_NAME "Cfw002drv"
#define CFW002_DEBUGFS_DIR "Cfw002"
#define CFW002_DEBUGFS_FWFILE "fwvar"
#define CFW002_DEBUGFS_DRVFILE "drvvar"
#define CFW_FIRMWARE_BIN "cfw002_fw.bin"


#define CFW002_IOCTL_MAGIC 0xcf

#define CFW002_CAN_READ _IOC(_IOC_READ|_IOC_WRITE,CFW002_IOCTL_MAGIC,0,0) 
#define CFW002_CAN_WRITE _IOC(_IOC_READ|_IOC_WRITE,CFW002_IOCTL_MAGIC,1,0)
#define CFW002_AUDIO_SETGAIN _IOW(CFW002_IOCTL_MAGIC,2,int)
#define CFW002_CAN_GETSTATS _IOC(_IOC_READ,CFW002_IOCTL_MAGIC,3,0)
#define CFW002_CAN_RXSETFIL _IOC(_IOC_READ,CFW002_IOCTL_MAGIC,4,0)
#define CFW002_CAN_IDSETFIL _IOC(_IOC_READ,CFW002_IOCTL_MAGIC,5,0)
#define CFW002_CAN_PORTENABLE _IOW(CFW002_IOCTL_MAGIC,6,int)

typedef struct cfw002_filter{
	unsigned long id;
	unsigned long mask;
	int extid;
	int mide;
	int port;
} cfw002_filter;

typedef struct cfw002_idfilter{
	unsigned int id;
	unsigned int port;
	int ena;
} cfw002_idfilter;

typedef struct cfw002_priv {

	void __iomem *map[3];
	
	spinlock_t lock;
	struct cfw002_txdesc *txbuf[10];
 	struct cfw002_rxdesc *rxbuf[10];
	u32 txdma[10];
	u32 rxdma[10];
	int txidx[10];
	int rxidx[10];
	int rxprod[10];
	int txcons[10];
	wait_queue_head_t rx_wq[10];
	wait_queue_head_t tx_wq[10];
	struct semaphore tx_sem[10];
	struct semaphore rx_sem[10];
	struct semaphore rx_block_sem[10];
	struct semaphore rx_cfg_sem;
	struct semaphore cmd_sem[2];
	int rx_num[10];
	int port_ena[10];

	atomic_t dev_available;
	
	struct dentry *debugfs;
	struct dentry *debugfs_fw;
	char *debugfs_data;
	int debugfs_num;
	
	dev_t dev;
	struct cdev cdev;
	struct pci_dev *pdev;
	
} cfw002_priv;



static inline u8 cfw002_plxread8(struct cfw002_priv *priv, int addr)
{
	return ioread8(addr + priv->map[0]);
}

static inline u16 cfw002_plxread16(struct cfw002_priv *priv, int addr)
{
	return ioread16(addr + priv->map[0]);
}

static inline u32 cfw002_plxread32(struct cfw002_priv *priv, int addr)
{
	return ioread32(addr + priv->map[0]);
}

static inline void cfw002_plxwrite8(struct cfw002_priv *priv,
				     int addr, u8 val)
{
	iowrite8(val, addr + priv->map[0]);
}

static inline void cfw002_plxwrite16(struct cfw002_priv *priv,
				      int addr, u16 val)
{
	iowrite16(val, addr + priv->map[0]);
}

static inline void cfw002_plxwrite32(struct cfw002_priv *priv,
				     int addr, u32 val)
{
	iowrite32(val, addr + priv->map[0]);
}

/////////////////////////////////////////////////////////////////////////

static inline u8 cfw002_xc1read8(struct cfw002_priv *priv,  int addr)
{
	return ioread8(addr + priv->map[1]);
}

static inline u16 cfw002_xc1read16(struct cfw002_priv *priv,  int addr)
{
	return ioread16(addr + priv->map[1]);
}

static inline u32 cfw002_xc1read32(struct cfw002_priv *priv,  int addr)
{
	return ioread32(addr + priv->map[1]);
}

static inline void cfw002_xc1write8(struct cfw002_priv *priv,
				     int addr, u8 val)
{
	iowrite8(val, addr + priv->map[1]);
}

static inline void cfw002_xc1write16(struct cfw002_priv *priv,
				      int addr, u16 val)
{
	iowrite16(val, addr + priv->map[1]);
}

static inline void cfw002_xc1write32(struct cfw002_priv *priv,
				      int addr, u32 val)
{
	iowrite32(val, addr + priv->map[1]);
}

/////////////////////////////////////////////////////////////////////////

static inline u8 cfw002_xc2read8(struct cfw002_priv *priv,  int addr)
{
	return ioread8(addr + priv->map[2]);
}

static inline u16 cfw002_xc2read16(struct cfw002_priv *priv, int addr)
{
	return ioread16(addr + priv->map[2]);
}

static inline u32 cfw002_xc2read32(struct cfw002_priv *priv,  int addr)
{
	return ioread32(addr + priv->map[2]);
}

static inline void cfw002_xc2write8(struct cfw002_priv *priv,
				     int addr, u8 val)
{
	iowrite8(val, addr + priv->map[2]);
}

static inline void cfw002_xc2write16(struct cfw002_priv *priv,
				      int addr, u16 val)
{
	iowrite16(val, addr + priv->map[2]);
}

static inline void cfw002_xc2write32(struct cfw002_priv *priv,
				      int addr, u32 val)
{
	iowrite32(val, addr + priv->map[2]);
}

#endif /* CFW002_H */
