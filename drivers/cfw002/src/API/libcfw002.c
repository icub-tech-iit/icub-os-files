#include "libcfw002.h"
#include "cfw002_api.h"

#include <pthread.h>
#include <unistd.h>
// Added critical section -- March 2010 Lorenzo

static pthread_mutex_t critical_section=PTHREAD_MUTEX_INITIALIZER;

#ifdef __cplusplus
extern "C" {
#endif

int fd = -1;
int portrefcount[10] = {0,0,0,0,0,0,0,0,0,0};
int audiorefcount = 0;

/**
 * Open audio device driver.
 * @param h: valid (allocated) device handler
 * @return: 0 on success
 */
int cfwAudioOpen(CFWAUDIO_HANDLE *ah)
{
    pthread_mutex_lock(&critical_section);

	if(fd == -1){
		fd = open(CFW_DEV_FILE, O_RDWR);
	}

	if(fd == -1)
        {
            pthread_mutex_unlock(&critical_section);
            return -2;
        }

	ah->gain = 0;
	audiorefcount++;

	cfw002_audio_setgain(fd, ah->gain);

    pthread_mutex_unlock(&critical_section);

	return 0;
}


/**
 * Close audio device driver.
 * @param h: valid (allocated) audio device handler
 * @return: 0 on success
 */
int cfwAudioClose(CFWAUDIO_HANDLE *ah)
{
	int used;
	int i;

    pthread_mutex_lock(&critical_section);


	if(fd == -1)
        {
            pthread_mutex_unlock(&critical_section);
            return -1;
        }

	if(ah->gain < 0)
        {
            pthread_mutex_unlock(&critical_section);
            return -2;
        }

	if(audiorefcount <= 0)
        {
            pthread_mutex_unlock(&critical_section);
            return -3;
        }

	audiorefcount--;
	ah->gain = -1;

	used = 0;

	if(audiorefcount == 0){
		for(i=0;i<10;i++){
			used = used || portrefcount[i];
		}
	}

	if(!used){
		close(fd);
		fd = -1;
	}

    pthread_mutex_unlock(&critical_section);
	return 0;
}


/**
 * Set audio gain.
 * @param h: valid audio device handler
 * @param gain: audio gain setting (0 to 7)
 * @return: 0 on success, -1 when the handle is invalid,
 *         -2 if the device driver is not opened, -3 if gain is invalid
 */
int cfwAudioSetGain(CFWAUDIO_HANDLE *ah, int gain)
{
	if(ah->gain == -1)
		return -1;

	if(fd == -1)
		return -2;

	if(gain<0 || gain>7)
		return -3;

	ah->gain = gain;

	cfw002_audio_setgain(fd, ah->gain);

	return 0;
}

/**
 * Close can device driver.
 * @param networkN: canbus network id, 0-10.
 * @param txQSize: transmit buffer size (ignored)
 * @param rxQSize: receiver buffer size (ignored)
 * @param txTimeout: transmit timeout (ms) (ignored)
 * @param rxTimeout: receiver timeout (ms) (ignored)
 * @param h: valid (allocated) device handler
 * @return: 0 on success, -1 when the networkN is invalid, -2 if the device driver can't be opened
 */
int cfwCanOpen(int networkN, int txQSize, int rxQSize, int txTimeout, int rxTimeout, CFWCAN_HANDLE *h)
{

	if((networkN < 0) || (networkN > 9))
		return -1;

    pthread_mutex_lock(&critical_section);

	if(fd == -1){
		fd = open(CFW_DEV_FILE, O_RDWR);

		if(fd == -1)
            {
                pthread_mutex_unlock(&critical_section);
                return -2;
            }
	}

	h->port = networkN;
	portrefcount[networkN]++;
    pthread_mutex_unlock(&critical_section);

	return 0;
}

/**
 * Close the handle.
 * If the closing handler is the last one, it waits for
 * all TXed packets to be flushed, and it stops the driver/HW
 * @param h:  device handler
 * @return: 0 on success, -1 otherwise
 */
int cfwCanClose(CFWCAN_HANDLE h)
{
	int used;
	int i;

	if((h.port < 0) || ( h.port > 9))
		return -1;

    pthread_mutex_lock(&critical_section);

	if(portrefcount[h.port] == 0){
        pthread_mutex_unlock(&critical_section);
		return -1;
	}else{
		portrefcount[h.port]--;
	}

	used = 0;

	if(portrefcount[h.port] == 0){
		for(i=0;i<10;i++){
			used = used || portrefcount[i];
		}
	}

	if(audiorefcount>0) used =1;

	if(!used){
		close(fd);
		fd = -1;
	}

	h.port = -1;

    pthread_mutex_unlock(&critical_section);

	return 0;
}

/**
 * Write messages to the bus, blocking (or until timeout), timeout currently
 * not implemented.
 * @param h: device handler
 * @param wb: message buffer
 * @param l: when called size of the buffer (number of messages to send)
 *  when returned, transmitted messages.
 * @param block: when !=0 the blocking-version syscall will be invoked
 */
int cfwCanWrite(CFWCAN_HANDLE h, CFWCAN_MSG *wb, unsigned int *l, int block)
{
	struct cfw002_rtx_header hdr;

	hdr.port = h.port;
	hdr.bufp = wb;
	hdr.block = block;

    if (fd==-1)
        return -1;

	cfw002_can_write(fd, &hdr,*l);
	*l = ((struct cfw002_rtx_status*) &hdr)->num;
    return 0;
}

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
int cfwCanRead(CFWCAN_HANDLE h, CFWCAN_MSG *rb, unsigned int *m, int block)
{
	struct cfw002_rtx_header hdr;

	hdr.port = h.port;
	hdr.bufp = rb;
	hdr.block = block;

    if (fd==-1)
        return -1;

	cfw002_can_read(fd, &hdr,*m);
	*m = ((struct cfw002_rtx_status*) &hdr)->num;
    return 0; //return always success
}

/**
 * Retrieve statistics for the can port.
 * @param h: handler
 * @param sb: statistics buffer to be filled
 * @return: 0 on success.
 */
int cfwCanStat(CFWCAN_HANDLE h, CFWCAN_STAT *sb)
{
	return cfw002_can_getstate(fd,h.port,sb);
}

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
int cfwCanSetFilter(CFWCAN_HANDLE h, int mide, int extid, unsigned long mask, unsigned long id)
{
	return cfw002_can_setfilter(fd, h.port, extid, mide, mask, id);

}

/**
 * Sets the software ID filter for the can port.
 * @param h: handler
 * @param id: the 11-bits ID to be added to or removed from the allowed ID list
 * @param ena: 1 to add the ID in the allowed-list, 0 to remove
 * @return: 0 on success.
 */
int cfwCanSetIdFilter(CFWCAN_HANDLE h, unsigned int id, int ena)
{
	return cfw002_can_setidfilter(fd,h.port,id,ena);

}

/**
 * Enables CAN communication over this CAN port.
 * @param h: handler
 * @return: 0 on success.
 */
int cfwCanRtxEnable(CFWCAN_HANDLE h)
{
	return cfw002_can_portenable(fd,h.port,1);

}

/**
 * Disables CAN communication over this CAN port.
 * @param h: handler
 * @return: 0 on success.
 */
int cfwCanRtxDisable(CFWCAN_HANDLE h)
{
	return cfw002_can_portenable(fd,h.port,0);

}

#ifdef __cplusplus
}
#endif
