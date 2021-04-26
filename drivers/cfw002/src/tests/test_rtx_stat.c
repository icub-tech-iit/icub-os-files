#include <fcntl.h>
#include <stdio.h>
#include "../API/cfw002_api.h"

#define NUM 50

int main()
{
	int sz;
	int i,j;
	int r;
	int f; 
	struct cfw002_rtx_header rhdr;
	char rbuf[CFW002_PAYLOAD_SIZE*NUM];
	struct cfw002_rtx_header thdr;
	char tbuf[CFW002_PAYLOAD_SIZE*NUM];
	struct cfw002_rtx_payload *tx,*rx;
	int flags;	
	int res[3];
	int pnum;
	struct cfw002_errstate e[10]; 
	
	f =  open("dev", O_RDWR);	
	sz = 1;
	
	rhdr.bufp = rbuf;
	thdr.bufp = tbuf;
	
	thdr.port = 2;

	tx =(struct cfw002_rtx_payload *) tbuf;
	tx->id = 4;
	tx->len=1;
	tx->data[0] = 0xab;

	cfw002_setblocking(f,1);
	
	if(-1 != f){

		cfw002_audio_setgain(f,2);
		
		cfw002_can_write(f,&thdr,sz);
		
		tx->data[0] = 0xcd;
		thdr.port = 2;
		thdr.bufp = tbuf;
		
		cfw002_can_write(f,&thdr,sz);
		

		tx->data[0] = 0xef;
		thdr.port = 2;
		thdr.bufp = tbuf;
	 	
		cfw002_can_write(f,&thdr,sz);

		if(fork()){
			printf("stats\n");
			while(1){
				sleep(1);
				printf("===stats===\n");
				cfw002_can_getstate(f, e);
				for(i=0;i<10;i++)
					printf("PORT %d : TEC %x, REC %x, STATE %x, TXC %d, RXC %d, ROV %d\n", 
						i, e[i].tec, e[i].rec, e[i].state, e[i].txc, e[i].rxc, e[i].rxov);
			
			}
		}
		
		
		res[0] = fork();
		res[1] = fork();
		res[2] = fork();
		
		pnum = (!!res[0]) | ((!!res[1])<<1) | ((!!res[2])<<2);

		if(pnum == 0){
			res[0] = fork();
			
			if(res[0] == 0){
				res[1] = fork();
				if(res[1] == 0)
					pnum = 8;
				else
					pnum = 9;
			}
			
		} 

		printf("port %d\n",pnum);
		
		j = 0;
		while(1){
		
			
			
			
			rx =  (struct cfw002_rtx_payload *) rbuf;
			
			if(((pnum == 0) || (pnum == 1) ||(pnum == 5) ||(pnum == 6) )	) 
				sz = 1; 
			else 
				sz = NUM;
			
			rhdr.bufp = rbuf;
		//	printf("buf %x\n", rbuf);
			rhdr.port = pnum; /*port*/
			r=cfw002_can_read(f,&rhdr,sz);

			if(r) fprintf(stderr,"ERR\n");
			if(((pnum == 0) || (pnum == 1) ||(pnum == 5) ||(pnum == 6)) && 
				(((struct cfw002_rtx_status*)(&rhdr))->num > 0 )){
				//fprintf(stderr,"n %x\n",*((__u16*)(&rhdr)) );
				thdr.port = pnum; /* port */
				thdr.bufp = tbuf;
				for(i=0;i<11;i++){
					tx = (struct cfw002_rtx_payload *)
						&tbuf[CFW002_PAYLOAD_SIZE*i];
					
					tx->id = 4;
					tx->len = 2;
					tx->data[0] = i;
					tx->data[1] = rx->data[0];
					
					
				}
				sz = 11;
				
				r=cfw002_can_write(f,&thdr,sz);
				
			
			}
			
		}
	}
	
}
