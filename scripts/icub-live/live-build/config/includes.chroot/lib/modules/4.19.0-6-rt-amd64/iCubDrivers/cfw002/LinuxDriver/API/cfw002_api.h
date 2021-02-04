#ifndef __CFW002API__
#define __CFW002API__
/*
 * Public API for iCub CFW002 Linux device driver
 *
 * Written by Andrea Merello <andrea.merello@iit.it>
 * Copyright 2010 Robotcub Consortium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <sys/ioctl.h>

/* An 11-bit ID can be specified in the ID field.
 * If a 29-bits ID has to be TXed, it can be specified
 * in the ID flag, adding (bitwise OR can be used)
 * also the following constant.
 * The same flag is set in 29-bit RXed packets
 */
#define CFW002_CAN_EXTID 0x80000000L

/* body, many for each read/write ioctl */
struct cfw002_rtx_payload {
	__le32 id; 	/* 11-bit or 29-bit ID*/
	__le16 len;	/* packet lenght. The 16bit type is for performace/alignment reasons */
	__u8 data[8];	/* payload data*/

}  __attribute__((packed));


/* RTX status descriptor */
struct cfw002_rtx_status {
	__u16 num;
}  __attribute__((packed));

/* header, one for each read/write ioctl */
struct cfw002_rtx_header {
	__u8 port;
	__u8 block;
	struct cfw002_rtx_payload *bufp; /* array of payloads */
}  __attribute__((packed));


/* ========== Errors & state interface =========== */
#define CFW002_IOCTL_GETSTATE 3

struct cfw002_errstate {
	__le16 tec; 	/* transmit error count */
	__le16 rec; 	/* receive error count */
	__le16 state; 	/* bitmask: port state */
	__le16 txc;	/* transmitted (submitted to CAN module) packets count */
	__le16 rxc;	/* received packets count */
	__le16 rxov;	/* RX fifo overflow count */
} __attribute__((packed));


/* the following #defines can be used in order to test for specific state flags */

/* set to 1 when the port is in WARN state */
#define CFW002_CAN_STATE_WARN		(1<<6)
/* set to 1 when the port is in BUSOFF state */
#define CFW002_CAN_STATE_BUSOFF		(1<<7)
/* set to 1 if the port has been in BUSOFF or WARN states in past */
#define CFW002_CAN_STATE_HADWARNORBOFF	(1<<5)
/* set to 1 if the port is disabled */
#define CFW002_CAN_STATE_DISABLED	(1<<8)
#define CFW002_IOCTL_MAGIC 0xcf
#define CFW002_MAJOR 180

#ifndef __cplusplus


/* =========== DATA (can RTX) interface =========== */

#define CFW002_IOCTL_READ 0
#define CFW002_IOCTL_WRITE 1
#define CFW002_IOCTL_PORTENABLE 6

#define CFW002_HDR_SIZE (sizeof(struct cfw002_rtx_header))

#define CFW002_PAYLOAD_SIZE (sizeof(struct cfw002_rtx_payload))

static inline int cfw002_can_write(int fd, struct cfw002_rtx_header *buf, int num)
{
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | (num<<16) | CFW002_IOCTL_WRITE | ((1|2)<<30), (char*)buf);
}

static inline int cfw002_can_read(int fd, struct cfw002_rtx_header *buf, int num)
{
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | (num<<16) | CFW002_IOCTL_READ | ((1|2)<<30), (char*)buf);
}

static inline int cfw002_can_portenable(int fd, int port, int ena)
{
	unsigned long arg;
	arg = port & 0xf;
	if(ena) arg |= 0x10;
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | (1<<16) | CFW002_IOCTL_PORTENABLE | (1<<30), arg);
}


/* ============= CAN RX FILTER ================ */

#define CFW002_IOCTL_SETFILTER 4

struct cfw002_filter{
	unsigned long id;
	unsigned long mask;
	int extid;
	int mide;
	int port;
};

static inline int cfw002_can_setfilter(int fd, int port, int extid, int mide,
	unsigned long mask, unsigned long id)
{
	struct cfw002_filter filter;
	filter.port = port;
	filter.extid = extid;
	filter.mide = mide;
	filter.mask = mask;
	filter.id = id;
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | CFW002_IOCTL_SETFILTER | (2<<30), (char*)&filter);
}


/* ============= CAN ID FILTER ================ */

#define CFW002_IOCTL_SETIDFILTER 5

struct cfw002_idfilter{
	unsigned int id;
	unsigned int port;
	int ena;
};

static inline int cfw002_can_setidfilter(int fd, unsigned int port, unsigned int id, int ena)
{
	struct cfw002_idfilter idfilter;

	idfilter.port = port;
	idfilter.id = id;
	idfilter.ena = ena;
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | CFW002_IOCTL_SETIDFILTER | (2<<30), (char*)&idfilter);
}



/* ========== Errors & state interface =========== */



static inline int cfw002_can_getstate(int fd, __u8 port, struct cfw002_errstate *ebuf)
{
	*((__u8*)ebuf) = port;
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | CFW002_IOCTL_GETSTATE | (2<<30), (char*)ebuf);
}



/* ============= AUDIO interface =============== */

#define CFW002_IOCTL_AUDIO_SETGAIN 2

static inline int cfw002_audio_setgain(int fd, unsigned char gain)
{
	return ioctl(fd, (CFW002_IOCTL_MAGIC<<8) | (1<<16) | CFW002_IOCTL_AUDIO_SETGAIN | (1<<30), gain);
}


#endif //cplusplus

#endif
