#ifndef __LIBCFW2H__
#define __LIBCFW2H__

/*
 * Public API for iCub CFW002 Linux device driver
 *
 * Authors Lorenzo Natale and Andrea Merello
 * Copyright 2010 Robotcub Consortium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "cfw002_api.h"

/* ID filters are implemented. CAN IDs must
 * be added to white-lists and ports need to
 * be enabled before packets could be RXed
 */
#define CFW2_HAS_FILTERS

#define CFW_DEV_FILE "/dev/cfw002"

/* CFW audio handle */
typedef struct __CFWAUDIO_HANDLE{
    int gain;
} CFWAUDIO_HANDLE;

/* CFW device handle */
typedef struct __CFWCAN_HANDLE{
    int port;
} CFWCAN_HANDLE;

/* CFW CAN payload */
typedef struct cfw002_rtx_payload CFWCAN_MSG;

typedef struct cfw002_errstate CFWCAN_STAT;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open audio device driver.
 * @param h: valid (allocated) device handler
 * @return: 0 on success
 */
int cfwAudioOpen(CFWAUDIO_HANDLE *ah);

/**
 * Close audio device driver.
 * @param h: valid (allocated) audio device handler
 * @return: 0 on success
 */
int cfwAudioClose(CFWAUDIO_HANDLE *ah);

/**
 * Set audio gain.
 * @param h: valid audio device handler
 * @param gain: audio gain setting (0 to 7)
 * @return: 0 on success, -1 when the handle is invalid, 
 *         -2 if the device driver is not opened, -3 if gain is invalid
 */
int cfwAudioSetGain(CFWAUDIO_HANDLE *ah, int gain);


/**
 * Open can device driver.
 * @param networkN: canbus network id, 0-10.
 * @param txQSize: transmit buffer size (ignored)
 * @param rxQSize: receiver buffer size (ignored)
 * @param txTimeout: transmit timeout (ms) (ignored)
 * @param rxTimeout: receiver timeout (ms) (ignored)
 * @param h: valid (allocated) device handler
 * @return: 0 on success, -1 when the networkN is invalid, -2 if the device driver can't be opened
 */
int cfwCanOpen(int networkN, int txQSize, int rxQSize, int txTimeout, int rxTimeout, CFWCAN_HANDLE *h);


/**
 * Close the can handle.
 * If the closing handler is the last one, it waits for
 * all TXed packets to be flushed, and it stops the driver/HW
 * @param h:  device handler 
 * @return: 0 on success, -1 otherwise
 */
int cfwCanClose(CFWCAN_HANDLE h);

/**
 * Write messages to the bus, blocking (or until timeout), timeout currently
 * not implemented.
 * @param h: device handler
 * @param wb: message buffer 
 * @param l: when called size of the buffer (number of messages to send)
 *  when returned, transmitted messages.
 * @param block: when !=0 the blocking-version syscall will be invoked
 */
int cfwCanWrite(CFWCAN_HANDLE h, CFWCAN_MSG *wb, unsigned int *l, int block);

/**
 * Read for messages in the queue. Wait if queue is empty. Return on reception 
 * of one message or rxtimeout.
 * @param h: handler 
 * @param rb: when called size of rb messages
 * @param m: max number of messages to read, when returned number of 
 * read messages
 * @param block: when !=0 the blocking-version syscall will be invoked
 * @return: 0 on success.
 */
int cfwCanRead(CFWCAN_HANDLE h, CFWCAN_MSG *rb, unsigned int *m, int block);

/**
 * Retrieve statistics for the can port.
 * @param h: handler 
 * @param sb: statistics buffer to be filled
 * @return: 0 on success.
 */
int cfwCanStat(CFWCAN_HANDLE h, CFWCAN_STAT *sb);

/**
 * Sets the hardware ID filter for the can port.
 * @param h: handler 
 * @param mide: match ID. If zero then extid is ignored and both 29 and 11 bits IDs are accepted
 * @param extid: valid only when mide is set, setting this to 1 causes that only 29 bits ids
 *  are accepted, setting this to 0 causes only 11 bits ids are accepted
 * @param mask: the mask value for the ID filter logic
 * @param id: the id value for the ID filter logic
 * @return: 0 on success.
 */
int cfwCanSetFilter(CFWCAN_HANDLE h, int mide, int extid, unsigned long mask, unsigned long id);


/**
 * Sets the software ID filter for the can port.
 * @param h: handler 
 * @param id: the 11-bits ID to be added to or removed from the allowed ID list
 * @param ena: 1 to add the ID in the allowed-list, 0 to remove
 * @return: 0 on success.
 */
int cfwCanSetIdFilter(CFWCAN_HANDLE h, unsigned int id, int ena);


/**
 * Enables CAN communication over this CAN port.
 * @param h: handler 
 * @return: 0 on success.
 */
int cfwCanRtxEnable(CFWCAN_HANDLE h);

/**
 * Disables CAN communication over this CAN port.
 * @param h: handler 
 * @return: 0 on success.
 */
int cfwCanRtxDisable(CFWCAN_HANDLE h);


#ifdef __cplusplus
}
#endif

#endif
