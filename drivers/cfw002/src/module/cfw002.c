/*
 * Linux device driver for iCub CFW002
 *
 * Written by Andrea Merello <andrea.merello@iit.it>
 * Copyright 2010 Robotcub Consortium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/firmware.h>
#include <linux/wait.h>
#include <linux/uaccess.h>

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) )
	#include <linux/module.h>
	#define IOCTL_FIELD_NAME unlocked_ioctl
	#define NEW_IOCTL_DECLARATION
#else
	#define IOCTL_FIELD_NAME ioctl
#endif

#include "cfw002.h"
#include "cfw002_hw.h"
#include "debug.h"

#define VERSION "0.6.4"

#define CFW_HAS_DEBUGFS


MODULE_AUTHOR("Andrea Merello <andrea.merello@iit.it>");
MODULE_DESCRIPTION("iCub CFW002 device driver");
MODULE_LICENSE("GPL");


int cfw002_major = CFW002_MAJOR;
int cfw002_txdescnum = TX_DESC_NUM;
int cfw002_rxdescnum = RX_DESC_NUM;

struct dentry *cfw002_debugfs;

module_param(cfw002_major, int, S_IRUGO);
module_param(cfw002_txdescnum, int, S_IRUGO);
module_param(cfw002_rxdescnum, int, S_IRUGO);

static struct pci_device_id cfw002_table[]  = {

	{ PCI_DEVICE(PCI_VENDOR_ID_IIT, 0xcf02) },
	{ }
};

MODULE_DEVICE_TABLE(pci, cfw002_table);


/* To be called only in behalf of userspace process */
static int cfw002_sendcmd(cfw002_priv *priv, int n, int cmd, u32 param)
{
	int i;
	int cmdreg;
	int paramreg;

	if(n>1){
		printk(KERN_WARNING"BUG in CFW002 driver.\nPlease report \"sendcmd with n>1\"");
		return -1;
	}

	cmdreg = ((n==0) ? CFW_CMD0 : CFW_CMD1);
	paramreg = ((n==0) ? CFW_PARAM0 : CFW_PARAM1);


	down(&priv->cmd_sem[n]);


	if(cfw002_plxread32(priv,cmdreg)){
		printk(KERN_INFO"cmd found busy state\n");

		up(&priv->cmd_sem[n]);
		return -1;
	}

	cfw002_plxwrite32(priv,paramreg, param);
	mb();
	cfw002_plxread32(priv,paramreg);
	cfw002_plxwrite32(priv,cmdreg, cmd);

	i=0;
	while(cfw002_plxread32(priv, cmdreg) != CFW_CMD_NONE){
		if(i++ == (CFW_CMD_TIMEOUT*100)){
			up(&priv->cmd_sem[n]);
			printk(KERN_INFO"cmd timeout %d %d\n",n,cmd);
			return -2;
		}
		udelay(10);
	}

	/* CMD OK */
	up(&priv->cmd_sem[n]);
	return 0;

}


static inline void cfw002_intenable(cfw002_priv *priv,unsigned int mask)
{
	cfw002_sendcmd(priv, 0,CFW_CMD_INTENABLE,mask);
	cfw002_sendcmd(priv, 1,CFW_CMD_INTENABLE,mask);

}


static inline void cfw002_txok1(cfw002_priv *priv,int isr)
{
	int i,idx;

	for(i=PCI_ISR_TXOK1,idx=0; idx<5; i = i<<1,idx++){

		if(0 == (isr & i))
			continue;

		priv->txcons[idx] = cfw002_xc1read16(priv, CFW_SHM_TXCONS + idx * 2);

		//if(priv->txidx[idx] != priv->txcons[idx]){
			wake_up(&priv->tx_wq[idx]);
		//}

	}
}

static inline void cfw002_txok2(cfw002_priv *priv,int isr)
{
	int i,idx;

	for(i=PCI_ISR_TXOK1,idx=0; idx<5; i = i<<1,idx++){

		if(0 == (isr & i))
			continue;

		priv->txcons[idx+5] = cfw002_xc2read16(priv, CFW_SHM_TXCONS + idx * 2);

		//if(priv->txidx[idx+5] != priv->txcons[idx+5]){
			wake_up(&priv->tx_wq[idx+5]);
		//}
	}
}

static int cfw002_tx(cfw002_priv *priv, char __user *buf, int port)
{
	unsigned int cons;
	unsigned int newidx;
//	int i;

	if(port < 5)
		cons = cfw002_xc1read16(priv, CFW_SHM_TXCONS + port * 2);
	else
		cons = cfw002_xc2read16(priv, CFW_SHM_TXCONS + (port - 5)*2);
	//printk(KERN_NOTICE"tx port %d cons: %d, idx %d!\n",port,cons, priv->txidx[port]);
	newidx = (priv->txidx[port]+1) % cfw002_txdescnum;

	if(unlikely(newidx == cons)){
		/* for char dev drv */
		//printk(KERN_NOTICE"TX full!\n");
		return -ENOMEM;
	}


	if (__copy_from_user((char*)&(priv->txbuf[port][priv->txidx[port]]),
			     buf,sizeof(struct cfw002_rtxdesc)))
		return -EFAULT;

	if(unlikely(priv->txbuf[port][priv->txidx[port]].len>8)){
		printk("Tx len >8\n");
		return -EINVAL;
	}

	priv->txidx[port] = newidx;
	//priv->txbuf[priv->txidx[port]][port].data[0] = 0xfa;
/*	printk("Tx idx %d port %d len %d\n",priv->txidx[port],port,priv->txbuf[port][priv->txidx[port]].len);

	for(i=0;i<priv->txbuf[port][priv->txidx[port]].len;i++)
		printk("%x ", priv->txbuf[port][priv->txidx[port]].data[i]);
	printk("\n");*/
	wmb();
	//printk("port %x prod %x\n",port,priv->txidx[port]);
	if(port < 5){
		cfw002_xc1write16(priv, CFW_SHM_TXPROD + port * 2, priv->txidx[port]);
		wmb();
		cfw002_plxwrite32(priv, CFW_IGEN, 1 << port);
	}else{
		cfw002_xc2write16(priv, CFW_SHM_TXPROD + (port - 5) * 2, priv->txidx[port]);
		wmb();
		cfw002_plxwrite32(priv, CFW_IGEN, 1 << (port-5 + 16));
	}



//	if(priv->txidx[port] == cons){
		/* for socketcan STOP queue port */
//	}


	return 0;

}


static inline int cfw002_getrxfullcount(cfw002_priv *priv, int port)
{
	int ret;
	ret = priv->rxprod[port] - priv->rxidx[port];

	if (ret < 0)
		ret += cfw002_rxdescnum;

	return ret;
}


static inline void cfw002_rx1(cfw002_priv *priv, int isr)
{
	int i,idx;
//	u32 tmp;
#if 1

	for(i=PCI_ISR_RXOK1,idx=0; idx<5; i = i<<1,idx++){
		if(0 == (isr & i))
			continue;

		priv->rxprod[idx] = cfw002_xc1read16(priv,CFW_SHM_RXPROD + idx * 2);

		/* it is not necessary to check for this: this condition is checked
		 * every time wake_up_interruptible is called, and eventually
		 * the process will return to sleep. However doing this test here
		 * reduces interrupt latency.
		 * NOTE:this brokes multiple open (currently not implemented anyway)
		 */

		if((priv->rx_num[idx] > 0) &&
			(priv->rx_num[idx] <= cfw002_getrxfullcount(priv, idx))){

			wake_up(&priv->rx_wq[idx]);
		}

	}

#else
	if(isr & (1|2)){
		tmp = cfw002_xc1read32(priv,CFW_SHM_RXPROD + 0 * 2);
		if(isr & 1){
			priv->rxprod[0] = tmp & 0xffff;
			wake_up_interruptible(&priv->rx_wq[0]);
		}
		if(isr & 2){
			priv->rxprod[1] = (tmp>>16) & 0xffff;
			wake_up_interruptible(&priv->rx_wq[1]);
		}
	}

	if(isr & (4|8)){
		tmp = cfw002_xc1read32(priv,CFW_SHM_RXPROD + 2 * 2);
		if(isr & 4){
			priv->rxprod[2] = tmp & 0xffff;
			wake_up_interruptible(&priv->rx_wq[2]);
		}
		if(isr & 8){
			priv->rxprod[3] = (tmp>>16) & 0xffff;
			wake_up_interruptible(&priv->rx_wq[3]);
		}
	}

	if(isr & 16){
		priv->rxprod[4] = cfw002_xc1read16(priv,CFW_SHM_RXPROD + 4 * 2);
		wake_up_interruptible(&priv->rx_wq[4]);
	}
#endif
}

static inline void cfw002_rx2(cfw002_priv *priv, int isr)
{
	int i,idx;
	//u32 tmp;

#if 1
	for(i=PCI_ISR_RXOK1,idx=0; idx<5; i = i<<1,idx++){

		if(0 == (isr & i))
			continue;


		priv->rxprod[idx+5] = cfw002_xc2read16(priv,CFW_SHM_RXPROD + idx * 2);
		/* it is not necessary to check for this: this condition is checked
		 * every time wake_up_interruptible is called, and eventually
		 * the process will return to sleep. However doing this test here
		 * reduces interrupt latency
 		 * NOTE: this brokes multiple open (currently not implemented anyway)
		 */

		if((priv->rx_num[idx+5] > 0) &&
			(priv->rx_num[idx+5] <= cfw002_getrxfullcount(priv, idx+5))){

			wake_up(&priv->rx_wq[idx+5]);
		}
	}
#else
	if(isr & (1|2)){
		tmp = cfw002_xc2read32(priv,CFW_SHM_RXPROD + 0 * 2);
		if(isr & 1){
			priv->rxprod[5] = tmp & 0xffff;
			wake_up_interruptible(&priv->rx_wq[5]);
		}
		if(isr & 2){
			priv->rxprod[6] = (tmp>>16) & 0xffff;
			wake_up_interruptible(&priv->rx_wq[6]);
		}
	}

	if(isr & (4|8)){
		tmp = cfw002_xc2read32(priv,CFW_SHM_RXPROD + 2 * 2);
		if(isr & 4){
			priv->rxprod[7] = tmp & 0xffff;
			wake_up_interruptible(&priv->rx_wq[7]);
		}
		if(isr & 8){
			priv->rxprod[8] = (tmp>>16) & 0xffff;
			wake_up_interruptible(&priv->rx_wq[8]);
		}
	}

	if(isr & 16){
		priv->rxprod[9] = cfw002_xc2read16(priv,CFW_SHM_RXPROD + 4 * 2);
		wake_up_interruptible(&priv->rx_wq[9]);
	}
#endif
}


static inline int cfw002_gettxemptycount(cfw002_priv *priv, int port)
{
	int full,ret;

	full = priv->txidx[port] - priv->txcons[port];

	if(full < 0)
		full += cfw002_txdescnum;

	ret = cfw002_txdescnum - full -1;
	//printk("tx idx %d cons %d empty %d\n",priv->txidx[port], priv->txcons[port], ret);

	return ret;
}

static inline int cfw002_get(cfw002_priv *priv, char __user *buf, int port, int max)
{
	int prev_idx = priv->rxidx[port];
	int count = 0;

//	printk("RX idx %d prod %d \n", priv->rxidx[port], priv->rxprod[port]);

	while(priv->rxidx[port] != priv->rxprod[port]){

		if(count == max)
			break;
		//printk("%x \n", buf+count*(sizeof(struct cfw002_rtxdesc)));
//if(port == 6) printk("%x: %x ",priv->rxidx[port],priv->rxbuf[port][priv->rxidx[port]].data[0]);
		if(__copy_to_user(buf+count*(sizeof(struct cfw002_rtxdesc)),
			&priv->rxbuf[port][priv->rxidx[port]], sizeof(struct cfw002_rtxdesc))){

			return -1;
		}


		prev_idx = priv->rxidx[port];
		priv->rxidx[port] = (priv->rxidx[port] +1) % cfw002_rxdescnum;

		count++;

	}

	/* do not waste time generating bus cycles and FW interrupts when not needed */
	if(count > 0){
		if(port < 5){
			cfw002_xc1write16(priv, CFW_SHM_RXCONS + port * 2, prev_idx);
			wmb();
			cfw002_plxwrite32(priv, CFW_IGEN, 0x20 << (port));


		}else{
			cfw002_xc2write16(priv, CFW_SHM_RXCONS + (port-5) * 2, prev_idx);
			wmb();
			cfw002_plxwrite32(priv, CFW_IGEN, 0x20 << (port-5+16));
		}
	}

	return count;

}


static irqreturn_t cfw002_interrupt(int irq, void *_priv)
{
	struct cfw002_priv *priv = _priv;
	unsigned long isr;

	spin_lock(&priv->lock);

	isr = cfw002_plxread32(priv, CFW_ISR);
	/* clear int flags */
	cfw002_plxwrite32(priv,CFW_ISR, isr);

	spin_unlock(&priv->lock);

/*	if(isr != 0)
		printk(KERN_INFO"Cfw IRQ %lx\n",isr);*/

	if(isr & PCI_ISR_RXOK_MASK1){
		cfw002_rx1(priv,isr);
	}

	if(isr & PCI_ISR_RXOK_MASK2){
		cfw002_rx2(priv,isr>>16);
	}

	if(isr & PCI_ISR_RXERR1){
		printk(KERN_INFO"Cfw rxfull0 %lx\n",isr);
	}

	if(isr & PCI_ISR_RXERR2){
		printk(KERN_INFO"Cfw rxfull1 %lx\n",isr);
	}

	if(isr & PCI_ISR_TXOK_MASK1){
		cfw002_txok1(priv,isr);
	}

	if(isr & PCI_ISR_TXOK_MASK2){
		cfw002_txok2(priv,isr>>16);
	}
	return IRQ_HANDLED;
}

static void cfw002_txflush_port(cfw002_priv *priv, int port)
{
	int j;
	int pend;
	u32 paramreg;

	/* wait for HW to drain the DMA ring HW */

	if(0 == wait_event_timeout(priv->tx_wq[port],
		(cfw002_txdescnum-1 == cfw002_gettxemptycount(priv, port)),
		CFW_FLUSH_TIMEOUT)){

		printk(KERN_WARNING"%s (cfw002) Timeout flushing DMA ring %d (%x, %x)\n",
			pci_name(priv->pdev),port, cfw002_txdescnum-1, cfw002_gettxemptycount(priv, port));
	}



	/* wait for HW to internally process all packets */
	for(j=0;j<CFW_FLUSH_TIMEOUT2;j++){

		pend = 1;


		if(port < 5){
			cfw002_sendcmd(priv,0, CFW_CMD_TXISPEND, port);

			paramreg = cfw002_plxread32(priv,CFW_PARAM0 );
		}else{
			cfw002_sendcmd(priv,1, CFW_CMD_TXISPEND, port-5);

			paramreg = cfw002_plxread32(priv,CFW_PARAM1 );
		}
		if((paramreg>>8) != CFW_CMD_TXISPEND){
			printk(KERN_WARNING"%s (cfw002) CFW flush procedure failed!\n",
				pci_name(priv->pdev));
		}else{
			pend = paramreg & 1;
			if(!pend)
				break;
		}

		msleep(100);
	}

	if(pend){
		printk(KERN_WARNING"%s (cfw002) Timeout while flushing HW %d\n",
			pci_name(priv->pdev),port);
	}


}

static void cfw002_txflush(cfw002_priv *priv)
{
	int i,j;
	int pend;
	u32 paramreg;

	/* wait for HW to drain the DMA ring HW */
	for(i=0;i<10;i++){
		if(0 == wait_event_timeout(priv->tx_wq[i],
			(cfw002_txdescnum-1 == cfw002_gettxemptycount(priv, i)),
			CFW_FLUSH_TIMEOUT)){

			printk(KERN_WARNING"%s (cfw002) Timeout flushing DMA ring %d (%x, %x)\n",
				pci_name(priv->pdev),i, cfw002_txdescnum-1, cfw002_gettxemptycount(priv, i));
		}
	}

	for(i=0;i<5;i++){
	/* wait for HW to internally process all packets */
		for(j=0;j<CFW_FLUSH_TIMEOUT2;j++){

			pend = 1;
			cfw002_sendcmd(priv,0, CFW_CMD_TXISPEND, i);

			paramreg = cfw002_plxread32(priv,CFW_PARAM0 );

			if((paramreg>>8) != CFW_CMD_TXISPEND){
				printk(KERN_WARNING"%s (cfw002) CFW flush procedure failed!\n",
					pci_name(priv->pdev));
			}else{
				pend = paramreg & 1;
				if(!pend)
					break;
			}

			msleep(100);
		}

		if(pend){
			printk(KERN_WARNING"%s (cfw002) Timeout while flushing HW %d\n",
				pci_name(priv->pdev),i);
		}
	}



	for(i=0;i<5;i++){
	/* wait for HW to internally process all packets */
		for(j=0;j<CFW_FLUSH_TIMEOUT2;j++){

			pend = 1;
			cfw002_sendcmd(priv,1, CFW_CMD_TXISPEND, i);

			paramreg = cfw002_plxread32(priv,CFW_PARAM1);

			if((paramreg>>8) != CFW_CMD_TXISPEND){
				printk(KERN_WARNING"%s (cfw002) CFW flush procedure failed!\n",
					pci_name(priv->pdev));
			}else{
				pend = paramreg & 1;
				if(!pend)
					break;
			}

			msleep(100);
		}

		if(pend){
			printk(KERN_WARNING"%s (cfw002) Timeout while flushing HW %d\n",
				pci_name(priv->pdev),i+5);
		}
	}


	//msleep(100);

	/*full = priv->txidx[0] - priv->txcons[0];

	if(full < 0)
		full += cfw002_txdescnum;
	printk("%x %x\n",cfw002_txdescnum,cfw002_gettxemptycount(priv, 0));
	printk("full %x\n",full);*/
}

static int _cfw002_close(cfw002_priv *priv)
{
	int i;
	unsigned long isr;

	cfw002_txflush(priv);

	cfw002_sendcmd(priv, 0,CFW_CMD_RTXENABLE,0);
	cfw002_sendcmd(priv, 1,CFW_CMD_RTXENABLE,0);
	mb();
	cfw002_intenable(priv,0x0);
	mb();
	msleep(100);
	isr = cfw002_plxread32(priv,CFW_ISR);
	cfw002_plxwrite32(priv,CFW_ISR,isr);
	mb();
	cfw002_plxread32(priv,CFW_ISR);

	for(i=0;i<10;i++){
		pci_free_consistent(priv->pdev, TX_DESC_SIZE * cfw002_txdescnum,
			priv->txbuf[i],priv->txdma[i]);

	}

	for(i=0;i<10;i++){
		pci_free_consistent(priv->pdev, RX_DESC_SIZE * cfw002_rxdescnum,
			priv->rxbuf[i],priv->rxdma[i]);

	}

	free_irq(priv->pdev->irq, priv);
	printk(KERN_INFO"%s (cfw002) closed\n",pci_name(priv->pdev));

	return 0;
}

int cfw002_close(struct inode *inode, struct file *filep)
{
	cfw002_priv *priv;
	int ret;

	priv = filep->private_data;

	/* in multi-open driver, here the filp private struct is deallocated.
	 * The dev_available atomic counter should be in the global private data
	 * and decremented. If and only if it reaches zero the close handler should
	 * be called
	 */

	ret = _cfw002_close(priv);
	atomic_inc(&priv->dev_available);

	return ret;
}

static int cfw002_allocdesc(cfw002_priv *priv)
{
	int i;
	/* TODO undo allocation if fails */
	for(i=0;i<10;i++){

		priv->txbuf[i] = pci_alloc_consistent(priv->pdev,
					      TX_DESC_SIZE * cfw002_txdescnum,
					      (dma_addr_t*) &(priv->txdma[i]));

		if(priv->txbuf[i] == NULL) {
			return -1;
		}
	}

	for(i=0;i<10;i++){

		priv->rxbuf[i] = pci_alloc_consistent(priv->pdev,
					      RX_DESC_SIZE * cfw002_rxdescnum,
					      (dma_addr_t*)&(priv->rxdma[i]));

		if(priv->rxbuf[i] == NULL) {
			return -1;
		}


	}

	return 0;
}

static void cfw002_sethwdesc(cfw002_priv *priv)
{
	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXDMA0, priv->txdma[0]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXDMA1, priv->txdma[1]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXDMA2, priv->txdma[2]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXDMA3, priv->txdma[3]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXDMA4, priv->txdma[4]);

	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXDMA0, priv->txdma[5]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXDMA1, priv->txdma[6]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXDMA2, priv->txdma[7]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXDMA3, priv->txdma[8]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXDMA4, priv->txdma[9]);


	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXDMA0, priv->rxdma[0]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXDMA1, priv->rxdma[1]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXDMA2, priv->rxdma[2]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXDMA3, priv->rxdma[3]);
	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXDMA4, priv->rxdma[4]);

	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXDMA0, priv->rxdma[5]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXDMA1, priv->rxdma[6]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXDMA2, priv->rxdma[7]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXDMA3, priv->rxdma[8]);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXDMA4, priv->rxdma[9]);


	cfw002_sendcmd(priv, 0,CFW_CMD_SETTXRINGSZ, cfw002_txdescnum);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETTXRINGSZ, cfw002_txdescnum);

	cfw002_sendcmd(priv, 0,CFW_CMD_SETRXRINGSZ, cfw002_rxdescnum);
	cfw002_sendcmd(priv, 1,CFW_CMD_SETRXRINGSZ, cfw002_rxdescnum);
}

static void cfw002_setidfilter(cfw002_priv *priv, int port, u16 id, int ena)
{
	u32 param;

	param = id & 0x7ff;

	if(ena)
		param |= 0x800;

	if(port<5){
		param |= port << 12;
		cfw002_sendcmd(priv, 0, CFW_CMD_SETIDFILTER, param);
	}else{
		param |= (port-5) << 12;
		cfw002_sendcmd(priv, 1, CFW_CMD_SETIDFILTER, param);
	}
}

static void cfw002_setrxfilter(cfw002_priv *priv, int port, int mide, int extid,
		unsigned long id, unsigned long mask)
{
	int card;
	u32 flags;

	if(port < 5){
		flags = port;
		card = 0;
	}else{
		flags = (port-5);
		card = 1;
	}

	flags |= (mide << 3);
	flags |= (extid << 4);

	cfw002_sendcmd(priv, card, CFW_CMD_PREPARERXFILTERMASK, mask);
	cfw002_sendcmd(priv, card, CFW_CMD_PREPARERXFILTERID, id);

	/* Command SETRXFILTER __MUST__ be the last one */
	cfw002_sendcmd(priv, card, CFW_CMD_SETRXFILTER, flags);


}

#ifdef NEW_IOCTL_DECLARATION
long cfw002_ioctl(struct file *filep,
	unsigned int cmd, unsigned long arg)
#else
int cfw002_ioctl(struct inode *inode, struct file *filep,
	unsigned int cmd, unsigned long arg)
#endif
{
	char __user *buf;

	struct cfw002_header hdr;

	/* for mask-wise HW computed 29 and 11 bits filtering */
	cfw002_filter filter;

	/* for idlist-wise FW computed 11 bits filtering */
	cfw002_idfilter idfilter;

	u8 port;
	u16 num;
	int read;
	u16 uread,uwrite;
	int i;
	int block;
	int ena;

	struct cfw002_errstate errstate;
	cfw002_priv *priv;


	priv = filep->private_data;

	read = 0; /* get rid of gcc warn */


	switch(cmd &~ IOCSIZE_MASK)
	{

		case CFW002_CAN_WRITE:
			//printk("TX!\n");
			buf = (char __user *)arg;

			num = _IOC_SIZE(cmd);

			if(unlikely(num > cfw002_txdescnum))
				return -EINVAL;

			/*if(unlikely(!access_ok(VERIFY_READ | VERIFY_WRITE, buf,
				(2 + num*(4+2+8)) * sizeof(char))))
			*/

			if(unlikely(!access_ok(VERIFY_READ | VERIFY_WRITE, buf,
				 sizeof(hdr))))

				return -EINVAL;

			if (__copy_from_user(&hdr,buf,sizeof(hdr)))
				return -EFAULT;

			if(unlikely(!access_ok(VERIFY_READ, hdr.bufp,
				 sizeof(struct cfw002_rtxdesc)*num)))

				return -EINVAL;

			port = hdr.port;
			block = hdr.block;

			if(unlikely(port > 9))
				return -EINVAL;


			/* even if multi open is not implemente yet, the following
			 * locking mechanism allows multi thread operation on the same fd
			 */
			//if(!(filep->f_flags & O_NONBLOCK)){
			if(block){
#if 1
				while(1){
					if(unlikely(down_interruptible(&priv->tx_sem[port])))
						return -ERESTARTSYS;


					if(unlikely(priv->port_ena[port] == 0)){
						up(&priv->tx_sem[port]);
						return -EBUSY;
					}


					if(num <= cfw002_gettxemptycount(priv, port))
						break; /* lock held, and we have enough room */

					up(&priv->tx_sem[port]);
					/* not enough room, lock released. Sleep! */
					if(unlikely(wait_event_interruptible(priv->tx_wq[port],
						num <= cfw002_gettxemptycount(priv, port))))

						return -ERESTARTSYS;
					/* Room has been freed, but we have to acquire the
 					 * lock and then check again before proceed
					 */
				}
#else
				if(unlikely(down_interruptible(&priv->tx_sem[port])))
						return -ERESTARTSYS;


				if(unlikely(wait_event_interruptible(priv->tx_wq[port],
						num <= cfw002_gettxemptycount(priv, port)))){

					up(&priv->tx_sem[port]);
					return -ERESTARTSYS;
				}

#endif
			}else{
				/* non blocking write */
				if(down_interruptible(&priv->tx_sem[port]))
					return -ERESTARTSYS;

				if(unlikely(priv->port_ena[port] == 0)){
					up(&priv->tx_sem[port]);
					return -EBUSY;
				}
			}

			/* here the semaphore is always held, and if the
			 * write was blocking then we are sure there is enough room
			 */

			for(i=0;i<num;i++){
				if(unlikely(0 !=
					cfw002_tx(priv, (char __user *)(hdr.bufp+i), port)))

					break;

			}

			/* release lock */
			up(&priv->tx_sem[port]);

			//__put_user((u16)i, buf);
			uwrite = i;
			if(__copy_to_user(buf,&uwrite,sizeof(u16))) BUG();
			/*if(i == 0)
				return -EAGAIN;*/


		break;

		case CFW002_CAN_READ:
			buf = (char __user *)arg;

			num = _IOC_SIZE(cmd);

			if(unlikely(num > cfw002_rxdescnum))
				return -EINVAL;

			/*if(unlikely(!access_ok(VERIFY_READ | VERIFY_WRITE, buf,
				(2+(4+2+8) * num) * sizeof(char))))
				return -EINVAL;

			__get_user(port,buf);*/

			if(unlikely(!access_ok(VERIFY_READ | VERIFY_WRITE, buf,
				 sizeof(hdr))))

				return -EINVAL;

			if(unlikely(__copy_from_user(&hdr,buf,sizeof(hdr)))) BUG();
			port = hdr.port;
			block = hdr.block;
		//	printk("read port %x num %x size %x \n",port,num, sizeof(struct cfw002_rtxdesc)*num);

			if(unlikely(!access_ok(VERIFY_WRITE, hdr.bufp,
				sizeof(struct cfw002_rtxdesc)*num)))

				return -EINVAL;



			if(unlikely(port > 9))
				return -EINVAL;


			//if(!(filep->f_flags & O_NONBLOCK)){
			if(block){
#if 1
				/* this allows mixed blocking and not blocking operations.
				 * Blocking calls are serialized by rx_block_sem.
				 * This allows to use the "rx_num" interrupt optimization
				 * because only one blocking call may be pending at any time.
				 * On the other hand the (only one) blocking call that
				 * can go ahead and acquires the rx_block_sem can
				 * ckeck for elements and eventually release the global
				 * rx semaphore (rx_sem) while waiting for elements
				 * so that non blocking calls may run across the blocking
				 * call without blocking.
				 */

				if(unlikely(down_interruptible(&priv->rx_block_sem[port])))
						return -ERESTARTSYS;
				while(1){
					if(unlikely(down_interruptible(&priv->rx_sem[port]))) {
						up(&priv->rx_block_sem[port]);
						return -ERESTARTSYS;
					}

					if(num <= cfw002_getrxfullcount(priv, port)){
 						/* mark that we do not need to be waken up */
						priv->rx_num[port] = 0;
						break; /* lock held, and we have enough room */
					}

					up(&priv->rx_sem[port]);

					priv->rx_num[port] = num;

					if(unlikely(wait_event_interruptible(priv->rx_wq[port],
						    num <= cfw002_getrxfullcount(priv, port)))) {
						up(&priv->rx_block_sem[port]);
						return -ERESTARTSYS;
					}
				}
#else
				/* If two blocking reads are issued on the same FD by two
			 	* concurrent threads, the first remains blocked until
			 	* enough data has been read.
			 	* The second is blocked because the first holds the
			 	* read semaphore. Once the 1st thread has got enough
			 	* data, the 2nd thread acquires the semaphore, then
			 	* it does wait for enough data
			 	*/
				if(unlikely(down_interruptible(&priv->rx_sem[port])))
					return -ERESTARTSYS;

				/* speed up things. To be reworked for multiple opens */
				priv->rx_num[port] = num;


				if(unlikely(wait_event_interruptible(priv->rx_wq[port],
					num <= cfw002_getrxfullcount(priv, port)))){

					up(&priv->rx_sem[port]);
					return -ERESTARTSYS;
				}

#endif
			}else{
				/* not blocking read */
				if(down_interruptible(&priv->rx_sem[port]))
					return -ERESTARTSYS;
			}

			read = cfw002_get(priv, (char __user *)(hdr.bufp), port, num);

			if(block) up(&priv->rx_block_sem[port]);

			up(&priv->rx_sem[port]);



			if(read < 0)
				return -EINVAL;


#if 0
            //Removed: it does not make sense for non-blocking calls
			/* TODO: kill this once debug is completed */
			if(!(filep->f_flags & O_NONBLOCK)){
				if(read != num)
					printk(KERN_ERR"CFW002: Debug. BUG in rx count %d %d",read,num);
			}
#endif

			uread = read;
			if(unlikely(__copy_to_user(buf,&uread,sizeof(u16)))) BUG();

		/*	if(read == 0)
				return -EAGAIN;*/

		break;

		case CFW002_AUDIO_SETGAIN &~ IOCSIZE_MASK:
			if(unlikely(arg > 7))
				return -EINVAL;
			//printk("Cfw002 set audio gain %d\n",(int)arg);
			cfw002_sendcmd(priv, 0,CFW_CMD_SETAUDIOGAIN, arg);
		break;

		case CFW002_CAN_GETSTATS:
		//	printk("stat\n");
			buf = (char __user *)arg;

			if(unlikely(!access_ok(VERIFY_READ | VERIFY_WRITE, buf,sizeof(struct cfw002_errstate))))
				return -EINVAL;

			if (__copy_from_user(&port,buf,sizeof(u8)))
				return -EFAULT;

			if(port < 5){
				errstate.tec = cfw002_xc1read16(priv,CFW_SHM_STAT_TEC+port*2);
				errstate.rec = cfw002_xc1read16(priv,CFW_SHM_STAT_REC+port*2);
				errstate.state = cfw002_xc1read16(priv,CFW_SHM_STAT_STATE+port*2);
				errstate.txc = cfw002_xc1read16(priv,CFW_SHM_STAT_TXC+port*2);
				errstate.rxc = cfw002_xc1read16(priv,CFW_SHM_STAT_RXC+port*2);
				errstate.rxov = cfw002_xc1read16(priv,CFW_SHM_STAT_RXOV+port*2);
			}else{
				port-=5;
				errstate.rec = cfw002_xc2read16(priv,CFW_SHM_STAT_REC+port*2);
				errstate.tec = cfw002_xc2read16(priv,CFW_SHM_STAT_TEC+port*2);
				errstate.state = cfw002_xc2read16(priv,CFW_SHM_STAT_STATE+port*2);
				errstate.txc = cfw002_xc2read16(priv,CFW_SHM_STAT_TXC+port*2);
				errstate.rxc = cfw002_xc2read16(priv,CFW_SHM_STAT_RXC+port*2);
				errstate.rxov = cfw002_xc2read16(priv,CFW_SHM_STAT_RXOV+port*2);
			}

			if(__copy_to_user(buf,(char*)&errstate,sizeof(errstate)))
				return -EINVAL;
		break;

		case CFW002_CAN_RXSETFIL:

			buf = (char __user *)arg;

			if(copy_from_user(&filter,buf,sizeof(cfw002_filter)))
				return -EINVAL;

			if(filter.port > 9)
				return -EINVAL;
			/* Because of firmware design, in order to minimize firmware complexity
			 * and to avoid proliferation of FW commands and reserved SHM areas, it is
			 *  _MANDATORY_ to avoid multiple filter set procedures to run concurrently
			 * on the same Infineon.
			 * We forbid even concurrent filter setting on the whole bunch of ports
			 * of the two Infineons.
			 */
			if(down_interruptible(&priv->rx_cfg_sem))
				return -ERESTARTSYS;

			cfw002_setrxfilter(priv, filter.port, filter.mide,
				filter.extid, filter.id, filter.mask);

			up(&priv->rx_cfg_sem);

		break;

		case CFW002_CAN_IDSETFIL:
			buf = (char __user *)arg;

			if(copy_from_user(&idfilter,buf,sizeof(cfw002_idfilter)))
				return -EINVAL;

			if(idfilter.port > 9)
				return -EINVAL;

			if(idfilter.id > 0x7ff)
				return -EINVAL;


			cfw002_setidfilter(priv, idfilter.port, idfilter.id, idfilter.ena);

		break;

		case CFW002_CAN_PORTENABLE & ~IOCSIZE_MASK:

			port = arg & 0xf;
			ena = arg & 0x10;

			if(unlikely( port > 9))
				return -EINVAL;

			if(unlikely(down_interruptible(&priv->tx_sem[port])))
				return -ERESTARTSYS;


			if(ena == 0)
				cfw002_txflush_port(priv, port);

			if(port < 5){

				cfw002_sendcmd(priv, 0,CFW_CMD_PORTENABLE,ena|port);
			}else{
				cfw002_sendcmd(priv, 1,CFW_CMD_PORTENABLE,ena|(port-5));
			}

			priv->port_ena[port] = ena;

			up(&priv->tx_sem[port]);
		break;

		default:
			return -EINVAL;
		break;
	}



	return 0;
}


/* FIRMWARE LOAD PROCEDURE
 * Bootloader will set SHM_BOOT_FLAG_READY
 * driver will put image in SHM
 * driver will set SHM_BOOT_FLAG_MAGIC
 * bootloader will fetch image and write PRAM
 * bootloader will set SHM_BOOT_FLAG_BOOT
 * bootloader will jump to the memory image
 * application firmware fill set CFW_SHM_BOOT_FLAG_OK
*/
static int cfw002_firmwarecheck(cfw002_priv *priv)
{
	unsigned int fwsign[2];
	int i;

	for(i=0;i<5;i++){
		msleep(100);
		mb();
		fwsign[0] = cfw002_xc1read16(priv,CFW_SHM_BOOT_FLAG);
		fwsign[1] = cfw002_xc2read16(priv,CFW_SHM_BOOT_FLAG);


		if(fwsign[0] != fwsign[1])
			continue;

		/* firmware needs to be upload */
		if(fwsign[0] == CFW_SHM_BOOT_FLAG_READY)
			return 1;

		/* firmware is going to bootstrap */
		if(fwsign[0] == CFW_SHM_BOOT_FLAG_BOOT)
			return 2;

		/* firmware OK */
		if(fwsign[0] == CFW_SHM_BOOT_FLAG_OK)
			return 0;

		/* firmware reported checksum error */
		if(fwsign[0] == CFW_SHM_BOOT_CHKSUM)
			return -2;

	}

	printk(KERN_INFO "%s (cfw002): boot flag %x %x\n",
		pci_name(priv->pdev), fwsign[0],fwsign[1]);

	return -1;

}


static long cfw002_firmwareload(cfw002_priv *priv, const struct firmware *fw)
{
	unsigned long time;
	int i;
	int ret;

	printk(KERN_INFO "%s (cfw002): uploading %lu bytes firmware\n",
		pci_name(priv->pdev), fw->size);


	for(i=0;i<fw->size;i+=2){
		cfw002_xc1write16(priv,CFW_SHM_BOOT_IMAGE+i,fw->data[i] | (fw->data[i+1] <<8));
	}
	printk(KERN_INFO "%s (cfw002): image transfer done\n",
		pci_name(priv->pdev));
	cfw002_xc1write16(priv,CFW_SHM_BOOT_LEN, fw->size);
	mb();
	cfw002_xc1write16(priv,CFW_SHM_BOOT_FLAG, CFW_SHM_BOOT_FLAG_MAGIC);
	cfw002_xc2write16(priv,CFW_SHM_BOOT_FLAG, CFW_SHM_BOOT_FLAG_MAGIC);

	time = jiffies;

	i=0;
	/* Wait for bootloader to bootstrap the image */
	while(1){
		ret = cfw002_firmwarecheck(priv);
		/* if bootloader is fast enough we will miss the
		 * sample when CFW_SHM_BOOT_FLAG_BOOT is set
		 */

		if(0 == ret || 2 == ret)
			break;
		/* checksum error */
		if(ret == -2)
			return -2;

		i++;

		if(i == CFW_BOOT_TIMEOUT)
			return -1;
	}

	/* wait for CFW firmware to initialize */
	i=0;
	while(1){
		ret = cfw002_firmwarecheck(priv);
		//printk("b %d\n",ret);
		if(0 == ret)
			break;
		i++;

		if(i == CFW_BOOT_TIMEOUT)
			return -3;
	}

	time = jiffies - time;

	return time;

}


static int _cfw002_open(cfw002_priv *priv)
{
	int i;
	int ret;
	unsigned long isr;
	const struct firmware *fw;


	ret = cfw002_firmwarecheck(priv);

	if(ret == -1){
		printk(KERN_ERR "%s (cfw002): bootloader/firmware fatal error\n",
			pci_name(priv->pdev));

			return -1;
	}

	/* if bootload is ready, or if a wrong chksum image was loaded */
	if((ret == 1) || (ret == -2)){

		printk(KERN_INFO "%s (cfw002): Firmware "CFW_FIRMWARE_BIN" needed\n",
		pci_name(priv->pdev));

		ret = request_firmware(&fw, CFW_FIRMWARE_BIN, &priv->pdev->dev);
		if(ret){
			printk(KERN_ERR "%s (cfw002): Can't open "CFW_FIRMWARE_BIN"\n",
			pci_name(priv->pdev));

			return -1;
		}

		ret = cfw002_firmwareload(priv,fw);

		release_firmware(fw);

		if(ret == -1){
			printk(KERN_ERR "%s (cfw002): Firmware upload FAILED: "
			"Device bootloader timeout\n",
			pci_name(priv->pdev));
		}else if(ret == -3){
			printk(KERN_ERR "%s (cfw002): Firmware upload FAILED: "
			"Device firmware image start timeout\n",
			pci_name(priv->pdev));
		}else if(ret == -2){
			printk(KERN_ERR "%s (cfw002): Firmware upload FAILED: "
			"Device reported checksum error\n",
			pci_name(priv->pdev));
		}

		if(ret<0){
			return -1;
		}else{
			printk(KERN_INFO "%s (cfw002): Got firmware alive rensponse after %d ticks\n",
			pci_name(priv->pdev),ret);
		}

	}
	/* TODO check for others boot flag status */

	ret = request_irq(priv->pdev->irq, &cfw002_interrupt,
		IRQF_SHARED, KBUILD_MODNAME, priv);

	if(ret){
		return ret;
	}

	for(i = 0;i < 10; i++){
		init_waitqueue_head(&priv->rx_wq[i]);
		init_waitqueue_head(&priv->tx_wq[i]);
	}

	if( 0!= cfw002_allocdesc(priv))
		return -1 ;

	cfw002_sethwdesc(priv);

	for(i=0;i<5;i++){
		//cfw002_xc1write16(priv, CFW_SHM_TXPROD + i, cfw002_txdescnum -1);
		//cfw002_xc2write16(priv, CFW_SHM_TXPROD + i, cfw002_txdescnum -1);
		cfw002_xc1write16(priv, CFW_SHM_TXPROD + i, 0);
		cfw002_xc2write16(priv, CFW_SHM_TXPROD + i, 0);

		/*priv->txcons[i] = cfw002_txdescnum -1;//cfw002_xc1read16(priv, CFW_SHM_TXCONS + i * 2);
		priv->txcons[i+5] = cfw002_txdescnum -1;//cfw002_xc2read16(priv, CFW_SHM_TXCONS + i * 2);*/
	}


	for(i=0;i<10;i++){
		priv->txidx[i] = 0;
		priv->rxidx[i] = 0;
		priv->rxprod[i] = 0;
		priv->txcons[i] = 0;
		priv->rx_num[i] = 0;
		/* default: all ports disabled*/
		priv->port_ena[i] = 0;
	}


//	for(i=0;i<cfw002_rxdescnum;i++)
//		priv->rxbuf[6][i].data[0] = 0xa5;

	isr = cfw002_plxread32(priv,CFW_ISR);
	cfw002_plxwrite32(priv,CFW_ISR,isr);
	mb();
	cfw002_plxread32(priv,CFW_ISR);

	cfw002_intenable(priv,0xffff);

	wmb();



	cfw002_sendcmd(priv, 0,CFW_CMD_RTXENABLE,1);
	cfw002_sendcmd(priv, 1,CFW_CMD_RTXENABLE,1);

	/* defaults HW filters to ACCEPT ALL.
	 * as the open callback is not returned yet here, there
	 * is no risk that the userspace caller is setting filter
	 * across this code, so no lock is required
	 */
	for(i=0;i<10;i++){
		cfw002_setrxfilter(priv, i, 0, 0, 0, 0);
	}

	printk(KERN_INFO"%s (cfw002) opened\n",pci_name(priv->pdev));
	return 0;
}


static int cfw002_open(struct inode *inode, struct file *filep)
{
	cfw002_priv *priv;
	/* The inode data is the same for all instances, it should
	 * keep the common driver stuff.
	 * The file pointer data is associated to the opened file
	 * descriptor.
	 * In a multi-open driver the filep->private_data should
	 * be ALLOCATED here. It should contains a struct
	 * with private open-instance data. Pointer to global
	 * private data carried by inode struct should be accessible
	 * to all callbacks, but eventually can be stored also in
	 * some field of filp->private_data.
	 * If for some reasons from the privata global data should
	 * be possible to access the open-instances data, a list
	 * should be mantained in the global data pointer
	 */
	priv = container_of(inode->i_cdev, cfw002_priv, cdev);
	filep->private_data = priv;

	if(!atomic_dec_and_test(&priv->dev_available)){
		atomic_inc(&priv->dev_available);
		return -EBUSY;
	}
	if(_cfw002_open(priv)){
		atomic_inc(&priv->dev_available);
		return -ENOMEM;
	}

	return 0;
}



static void  cfw002_remove(struct pci_dev *pdev)
{
	cfw002_priv *priv = pci_get_drvdata(pdev);

	if(priv->debugfs){
		debugfs_remove(priv->debugfs_fw);
		debugfs_remove(priv->debugfs);
	}

	cdev_del(&priv->cdev);

	unregister_chrdev_region(priv->dev,1);

	pci_iounmap(pdev, priv->map[0]);
	pci_iounmap(pdev, priv->map[1]);
	pci_iounmap(pdev, priv->map[2]);

	pci_release_regions(pdev);
	pci_disable_device(pdev);
	kfree(priv);

}

#ifdef CONFIG_PM
static int cfw002_suspend(struct pci_dev *pdev, pm_message_t state)
{
	pci_save_state(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
	return 0;
}

static int cfw002_resume(struct pci_dev *pdev)
{
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	return 0;
}

#endif /* CONFIG_PM */


static void cfw002_debug_getfw(struct cfw002_priv *priv)
{

	int i;
	tDebugData d;
	u8 *b;
	char *buf;
	int n;

	cfw002_sendcmd(priv,0 ,CFW_CMD_DEBUG,0);
	cfw002_sendcmd(priv,1 ,CFW_CMD_DEBUG,0);

	b = (u8*)&d;
	buf = priv->debugfs_data;
	buf[0] = 0;

	for(n=0;n<2;n++){

		for(i=0;i<sizeof(tDebugData);i++){
			if(n == 0)
				b[i] = cfw002_xc1read8(priv, CFW_SHM_DEBUG + i);
			else
				b[i] = cfw002_xc2read8(priv, CFW_SHM_DEBUG + i);
		}



		sprintf(buf+strlen(buf), "CFW001 up %d\n",n);

		sprintf(buf+strlen(buf), "plx_pci_int_mask 0x%x\n", d.plx_pci_int_mask);
		sprintf(buf+strlen(buf), "plx_rtx_ena 0x%x\n", d.plx_rtx_ena);
		sprintf(buf+strlen(buf), "plx_cmd_reg 0x%x\n", d.plx_cmd_reg);
		sprintf(buf+strlen(buf), "plx_param_reg 0x%x\n", d.plx_param_reg);
		sprintf(buf+strlen(buf), "plx_dma_ladr_reg 0x%x\n", d.plx_dma_ladr_reg);
		sprintf(buf+strlen(buf), "plx_dma_pciadr_reg 0x%x\n", d.plx_dma_pciadr_reg);
		sprintf(buf+strlen(buf), "plx_dma_cnt_reg 0x%x\n", d.plx_dma_cnt_reg);
		sprintf(buf+strlen(buf), "plx_dma_stacmd_start 0x%x\n", d.plx_dma_stacmd_start);
		sprintf(buf+strlen(buf), "plx_dma_stacmd_clear 0x%x\n", d.plx_dma_stacmd_clear);
		sprintf(buf+strlen(buf), "plx_dma_desc_reg 0x%x\n", d.plx_dma_desc_reg);
		sprintf(buf+strlen(buf), "plx_rtx_idx_shift 0x%x\n", d.plx_rtx_idx_shift);
		sprintf(buf+strlen(buf), "plx_tx_cons_idx_shm 0x%x\n", d.plx_tx_cons_idx_shm);
		sprintf(buf+strlen(buf), "plx_tx_prod_idx_shm 0x%x\n", d.plx_tx_prod_idx_shm);
		sprintf(buf+strlen(buf), "plx_rx_cons_idx_shm 0x%x\n", d.plx_rx_cons_idx_shm);
		sprintf(buf+strlen(buf), "plx_rx_prod_idx_shm 0x%x\n", d.plx_rx_prod_idx_shm);
		sprintf(buf+strlen(buf), "plx_rx_dma_shm 0x%x\n", d.plx_rx_dma_shm);
		sprintf(buf+strlen(buf), "plx_tx_dma_shm 0x%x\n", d.plx_tx_dma_shm);
		sprintf(buf+strlen(buf), "plx_idx_mask 0x%x\n", d.plx_idx_mask);
		sprintf(buf+strlen(buf), "plx_rx_dma[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_dma[0], d.plx_rx_dma[1], d.plx_rx_dma[2], d.plx_rx_dma[3], d.plx_rx_dma[4]);

		sprintf(buf+strlen(buf), "plx_tx_dma[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_dma[0], d.plx_tx_dma[1], d.plx_tx_dma[2], d.plx_tx_dma[3], d.plx_tx_dma[4]);

		sprintf(buf+strlen(buf), "plx_rx_core_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_core_idx[0], d.plx_rx_core_idx[1], d.plx_rx_core_idx[2],
			d.plx_rx_core_idx[3], d.plx_rx_core_idx[4]);

		sprintf(buf+strlen(buf), "plx_rx_dma_local_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_dma_local_idx[0], d.plx_rx_dma_local_idx[1], d.plx_rx_dma_local_idx[2],
			d.plx_rx_dma_local_idx[3], d.plx_rx_dma_local_idx[4]);

		sprintf(buf+strlen(buf), "plx_rx_dma_pci_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_dma_pci_idx[0], d.plx_rx_dma_pci_idx[1], d.plx_rx_dma_pci_idx[2],
			d.plx_rx_dma_pci_idx[3], d.plx_rx_dma_pci_idx[4]);

		sprintf(buf+strlen(buf), "plx_rx_cons_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_cons_idx[0], d.plx_rx_cons_idx[1], d.plx_rx_cons_idx[2],
			d.plx_rx_cons_idx[3], d.plx_rx_cons_idx[4]);

		sprintf(buf+strlen(buf), "plx_rx_can_wakereq[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_rx_can_wakereq[0], d.plx_rx_can_wakereq[1], d.plx_rx_can_wakereq[2],
			d.plx_rx_can_wakereq[3], d.plx_rx_can_wakereq[4]);

		sprintf(buf+strlen(buf), "plx_can_tx_pend[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_can_tx_pend[0], d.plx_can_tx_pend[1], d.plx_can_tx_pend[2],
			d.plx_can_tx_pend[3], d.plx_can_tx_pend[4]);

		sprintf(buf+strlen(buf), "plx_tx_prod_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_prod_idx[0], d.plx_tx_prod_idx[1], d.plx_tx_prod_idx[2],
			d.plx_tx_prod_idx[3], d.plx_tx_prod_idx[4]);

		sprintf(buf+strlen(buf), "plx_tx_dma_local_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_dma_local_idx[0], d.plx_tx_dma_local_idx[1], d.plx_tx_dma_local_idx[2],
			d.plx_tx_dma_local_idx[3], d.plx_tx_dma_local_idx[4]);

		sprintf(buf+strlen(buf), "plx_tx_local_ready_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_local_ready_idx[0], d.plx_tx_local_ready_idx[1], d.plx_tx_local_ready_idx[2],
			d.plx_tx_local_ready_idx[3], d.plx_tx_local_ready_idx[4]);

		sprintf(buf+strlen(buf), "plx_tx_dma_pci_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_dma_pci_idx[0], d.plx_tx_dma_pci_idx[1], d.plx_tx_dma_pci_idx[2],
			d.plx_tx_dma_pci_idx[3], d.plx_tx_dma_pci_idx[4]);

		sprintf(buf+strlen(buf), "plx_tx_can_idx[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_can_idx[0], d.plx_tx_can_idx[1], d.plx_tx_can_idx[2],
			d.plx_tx_can_idx[3], d.plx_tx_can_idx[4]);

		sprintf(buf+strlen(buf), "plx_tx_queue_isfull[5] 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			d.plx_tx_queue_isfull[0], d.plx_tx_queue_isfull[1], d.plx_tx_queue_isfull[2],
			d.plx_tx_queue_isfull[3], d.plx_tx_queue_isfull[4]);

		sprintf(buf+strlen(buf), "plx_pci_rx_sz 0x%x\n", d.plx_pci_rx_sz);
		sprintf(buf+strlen(buf), "plx_pci_tx_sz 0x%x\n", d.plx_pci_tx_sz);
		sprintf(buf+strlen(buf), "plx_dma_pend_direction 0x%x\n",d.plx_dma_pend_direction);
		sprintf(buf+strlen(buf), "plx_tx_dma_pend 0x%x\n", d.plx_tx_dma_pend);
		sprintf(buf+strlen(buf), "plx_rx_dma_pend 0x%x\n", d.plx_rx_dma_pend);
		sprintf(buf+strlen(buf), "plx_dma_pend 0x%x\n", d.plx_dma_pend);
		sprintf(buf+strlen(buf), "plx_dma_rr 0x%x\n", d.plx_dma_rr);
		sprintf(buf+strlen(buf), "plx_dma_tx_rr 0x%x\n", d.plx_dma_tx_rr);
		sprintf(buf+strlen(buf), "plx_dma_rx_rr 0x%x\n", d.plx_dma_rx_rr);
		sprintf(buf+strlen(buf), "plx_dma_isr 0x%x\n", d.plx_dma_isr);
		sprintf(buf+strlen(buf), "plx_cmd_isr 0x%x\n", d.plx_cmd_isr);
		sprintf(buf+strlen(buf), "plx_int_rx_full 0x%x\n", d.plx_int_rx_full);
		sprintf(buf+strlen(buf), "plx_int_rx_ok 0x%x\n", d.plx_int_rx_ok);

	}
}

static int cfw002_debugfs_open(struct inode *inode, struct file *filep)
{
#ifdef CFW_HAS_DEBUGFS
	int num;
	cfw002_priv *priv;

	priv = inode->i_private;
	filep->private_data = priv;

	/* 32 bytes name, 8 digits, 4 for spaces 0x string and newline */
	num = (32+8+4)*32 + (32+(8+4)*5)*16;
	priv->debugfs_data = kmalloc(num,GFP_KERNEL);

	if(NULL == priv->debugfs_data)
		return -ENOMEM;

	cfw002_debug_getfw(priv);

	priv->debugfs_num = strlen(priv->debugfs_data);
#endif //CFW_DEBUGFS
	return 0;
}


static int cfw002_debugfs_close(struct inode *inode, struct file *filep)
{
#ifdef CFW_HAS_DEBUGFS
	cfw002_priv *priv;
	priv = filep->private_data;

	priv->debugfs_num = 0;
	kfree(priv->debugfs_data);
#endif //CFW_HAS_DEBUGFS
	return 0;
}

static ssize_t cfw002_debugfs_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	cfw002_priv *priv;
	int num;

	priv = filp->private_data;

	if(*offp > priv->debugfs_num)
		return 0;

	num = priv->debugfs_num - *offp;

	if(count < num)
		num = count;

	if(copy_to_user(buff, priv->debugfs_data+*offp, num)){
		return 0;
	}

	*offp += num;

	return num;
}

static struct file_operations cfw002_ops = {
	.open = cfw002_open,
	.release = cfw002_close,
	.IOCTL_FIELD_NAME = cfw002_ioctl,
	.owner = THIS_MODULE,
	.read = NULL,
	.write = NULL
};

static struct file_operations cfw002_debugfs_ops = {
	.open = cfw002_debugfs_open,
	.release = cfw002_debugfs_close,
	.IOCTL_FIELD_NAME = NULL,
	.owner = THIS_MODULE,
	.read = cfw002_debugfs_read,
	.write = NULL
};

static int  cfw002_probe(struct pci_dev *pdev,
				   const struct pci_device_id *id)
{

	struct cfw002_priv *priv;
	static int cfw002_minor = 0;

	u32 mem_addr[3];
	unsigned long mem_len[3];
	char name[32];
	int i;

	int err;

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR "%s (cfw002): Cannot enable new PCI device\n",
		       pci_name(pdev));
		return err;
	}

	err = pci_request_regions(pdev, KBUILD_MODNAME);
	if (err) {
		printk(KERN_ERR "%s (cfw002): Cannot obtain PCI resources\n",
		       pci_name(pdev));
		return err;
	}

	mem_addr[0] = pci_resource_start(pdev, 0);
	mem_len[0] = pci_resource_len(pdev, 0);
	mem_addr[1] = pci_resource_start(pdev, 2);
	mem_len[1] = pci_resource_len(pdev, 2);
	mem_addr[2] = pci_resource_start(pdev, 3);
	mem_len[2] = pci_resource_len(pdev, 3);

	if ((err = pci_set_dma_mask(pdev, 0xFFFFFFFCULL)) ||
	    (err = pci_set_consistent_dma_mask(pdev, 0xFFFFFFFCULL))) {
		printk(KERN_ERR "%s (cfw002): No suitable DMA available\n",
		       pci_name(pdev));
		goto err_free_reg;
	}

	pci_set_master(pdev);


	priv = kmalloc(sizeof(cfw002_priv),GFP_KERNEL);
	priv->pdev = pdev;

	pci_set_drvdata(pdev, priv);

	priv->map[0] = priv->map[1] = priv->map[2] = 0;

	priv->map[0] = pci_iomap(pdev, 0, mem_len[0]);
	priv->map[1] = pci_iomap(pdev, 2, mem_len[1]);
	priv->map[2] = pci_iomap(pdev, 3, mem_len[2]);

	if ((!priv->map[0]) || (!priv->map[1]) || (!priv->map[2])) {
		printk(KERN_ERR "%s (cfw002): Cannot map device memory\n",
		       pci_name(pdev));
		goto err_iounmap;
	}


	printk(KERN_INFO "Cfw002 found!\n");

	atomic_set(&priv->dev_available, 1);
	spin_lock_init(&priv->lock);
	for (i = 0; i < 10; i++){
		init_MUTEX(&priv->tx_sem[i]);
		init_MUTEX(&priv->rx_sem[i]);
		init_MUTEX(&priv->rx_block_sem[i]);
	}
	init_MUTEX(&priv->cmd_sem[0]);
	init_MUTEX(&priv->cmd_sem[1]);
	init_MUTEX(&priv->rx_cfg_sem);

	priv->dev = MKDEV(cfw002_major,cfw002_minor++);
	err = register_chrdev_region(priv->dev,1,CFW002_NAME);

	if(err < 0){
		printk(KERN_ERR "%s (cfw002): can't register MAJOR\n",
		       pci_name(pdev));

		goto err_iounmap;
	}

	cdev_init(&priv->cdev,&cfw002_ops);
	priv->cdev.ops = &cfw002_ops;
	priv->cdev.owner = THIS_MODULE;

	cdev_add(&priv->cdev, priv->dev, 1);

	printk(KERN_INFO "Cfw002 registered with major %d, minor %d\n",
		MAJOR(priv->dev), MINOR(priv->dev));


	sprintf(name,"minor %d", MINOR(priv->dev));

	if(cfw002_debugfs){
		priv->debugfs = debugfs_create_dir(name,cfw002_debugfs);
		/* debugfs provide informations about the GENERAL cfw002 driver struct.
		 * if infos on all opened instance should be given, a list of opened devices
		 * should be mantained in the global private data
		 */
		priv->debugfs_fw = debugfs_create_file(CFW002_DEBUGFS_FWFILE, 444, priv->debugfs,priv,&cfw002_debugfs_ops);

	}else{
		priv->debugfs = NULL;
	}

	return 0;

 err_iounmap:
 	if(priv->map[0])
		iounmap(priv->map[0]);
	if(priv->map[1])
		iounmap(priv->map[1]);
	if(priv->map[2])
		iounmap(priv->map[2]);


	pci_set_drvdata(pdev, NULL);
	kfree(priv);

 err_free_reg:
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	return err;
}

static struct pci_driver cfw002_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= cfw002_table,
	.probe		= cfw002_probe,
	.remove		= cfw002_remove,
#ifdef CONFIG_PM
	.suspend	= cfw002_suspend,
	.resume		= cfw002_resume,
#endif /* CONFIG_PM */
};


static int __init cfw002_init(void)
{

	printk(KERN_INFO"Cfw002 device driver V %s\n",VERSION);
	cfw002_debugfs = debugfs_create_dir(CFW002_DEBUGFS_DIR,NULL);
	return pci_register_driver(&cfw002_driver);
}

static void __exit cfw002_exit(void)
{
	pci_unregister_driver(&cfw002_driver);

	if(cfw002_debugfs)
		debugfs_remove(cfw002_debugfs);
}

module_init(cfw002_init);
module_exit(cfw002_exit);
