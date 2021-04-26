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

typedef struct tDebugData{
	//u16 magic_cookie;
	
	u32 plx_pci_int_mask;
	u16 plx_rtx_ena;

	/* preconfigured stuff (MSEL-related) */

	s16 plx_cmd_reg; 		/* posinter to local CMD read reg */
	s16 plx_param_reg;		/* posinter to local CMD PARAM reg */
	//s16 plx_dbg_reg;		/* posinter to local DBG reg */
	s16 plx_dma_ladr_reg;		/* posinter to local DMA local adr reg */
	s16 plx_dma_pciadr_reg;		/* posinter to local DMA PCI adr reg */
	s16 plx_dma_cnt_reg;		/* posinter to local DMA sz reg */
	u16 plx_dma_stacmd_start;	/* Val to write on DMA status/command reg to start DMA*/
	u16 plx_dma_stacmd_clear;	/* Val to write on DMA status/command reg to clear DMA s16 */
	s16 plx_dma_desc_reg;	 	/* pointer to local DMA descriptor reg */


	s16 plx_rtx_idx_shift;		/* shift for local to PCI interrupt status */
	u32 plx_tx_cons_idx_shm;	/* shared memory addr for consumer TX ptr pool */
	u32 plx_tx_prod_idx_shm;	/* shared memory addr for producer TX ptr pool */
	u32 plx_rx_cons_idx_shm;	/* shared memory addr for consumer RX ptr pool */
	u32 plx_rx_prod_idx_shm;	/* shared memory addr for producer RX ptr pool */
	u32 plx_rx_dma_shm;		/* shared memory full addr for RX DMA */
	u32 plx_tx_dma_shm;		/* shared memory full addr for TX DMA */
	u32 plx_idx_mask;		/* local doorbell bits that this cpu is allowed to clear */


	/* PCI DMA base addresses */
	u32 plx_rx_dma[5];
	u32 plx_tx_dma[5];


	/* indexes/posinters for rings/dma */
	s16 plx_rx_core_idx[5];		/* the RX SHM has been written up to this */
	s16 plx_rx_dma_local_idx[5];	/* the local RX dma pointer is here */
	s16 plx_rx_dma_pci_idx[5];	/* the pci RX dma pointer is here */
	u16 plx_rx_cons_idx[5];		/* host (consumer) RX idx (descriptors are available for RX up to this)*/
	//s16 plx_rx_full[5];		/* counter for RX packet not yet finished (in SHM waiting for DMA) */
	s16 plx_rx_can_wakereq[5];	/* flag for can RX awake queue request needed */

	s16 plx_can_tx_pend[5];		/* flag for can TX in progress */
	u16 plx_tx_prod_idx[5];		/* the host wrote TX pkts up to this */
	u16 plx_tx_dma_local_idx[5];	/* the local TX pointer for DMA request is here */
	u16 plx_tx_local_ready_idx[5];	/* the local TX pointer for DMA completed is here */
	u16 plx_tx_dma_pci_idx[5];	/* the pci TX dma local pointer is here */
	u16 plx_tx_can_idx[5];		/* the can has been feed up to this */	
	s16 plx_tx_queue_isfull[5];	/* this flag is set to 1 if the TX queue is full */

	u16 plx_pci_rx_sz;		/* number of RX descriptors */
	u16 plx_pci_tx_sz;		/* number of TX descriptors */
	s16 plx_dma_pend_direction;	/* flag for DMA direction */
	s16 plx_tx_dma_pend;
	s16 plx_rx_dma_pend;
	s16 plx_dma_pend;		/* flag/enumerator for dma channel in use. When TX n means dma on ring n-1 */
	s16 plx_dma_rr;			/* flag for dma RTX round robin */
	s16 plx_dma_tx_rr;		/* counter for dma TX round robin */
	s16 plx_dma_rx_rr;		/* counter for dma RX round robin */
	
	u32 plx_dma_isr;		/* local ISR flag for dma completed */
	u32 plx_cmd_isr;		/* local ISR flag for host command */
	//u32 plx_dbg_isr;
	//u32 plx_rx_idx_isr;		/* local ISR flag for RX consumer IDX updated */
	u32 plx_int_rx_full;		/* PCI isr flag for rx overflow */
	u32 plx_int_rx_ok;		/* PCI isr flag for rx ok */
	u32 plx_int_tx_ok;		/* PCI isr flag for tx ok */


} __attribute__((packed)) tDebugData;
