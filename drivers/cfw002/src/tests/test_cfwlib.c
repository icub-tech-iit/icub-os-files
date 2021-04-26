/* compile with -lcfw002*/

#include <fcntl.h>
#include <stdio.h>
#include <libcfw002.h>

#define NUM 50

int main()
{
	int sz;
	int i,j;
	int r;
 
	CFWCAN_HANDLE h[10];
	CFWCAN_MSG rx[NUM],tx[NUM];

	int flags;	
	int res[3];
	int pnum;
	CFWCAN_STAT e; 
	
	for(i=0;i<10;i++)
		cfwCanOpen(i, 0, 0, 0, 0, &(h[i]));
		
	
	tx[0].id = 4;
	tx[0].len = 1;
		
	tx[0].data[0] = 0xab;
	
	sz = 1;
	cfwCanWrite(h[0], tx, &sz, 0);
		
	tx[0].data[0] = 0xcd;
	sz = 1;
	cfwCanWrite(h[0], tx, &sz, 0);
		

	tx[0].data[0] = 0xef;
	sz = 1;
	cfwCanWrite(h[0], tx, &sz, 0);
	
	if(fork()){
		printf("stats\n");
		while(1){
			sleep(1);
			printf("===stats===\n");
			for(i=0;i<10;i++){

				cfwCanStat(h[i], &e);
				printf("PORT %d : TEC %x, REC %x, STATE %x, TXC %d, RXC %d, ROV %d\n", 
					i, e.tec, e.rec, e.state, e.txc, e.rxc, e.rxov);
			
			}
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
	//cfwCanSetFilter(h[pnum], 1, 0, 0, 0);	
	j = 0;
	while(1){
	
		if(((pnum == 0) || (pnum == 1) ||(pnum == 5) ||(pnum == 6) )	) 
			sz = 1; 
		else 
			sz = NUM;
			
		cfwCanRead(h[pnum], rx, &sz, 1);
		

		if(sz>0){	
			if((pnum == 0) || (pnum == 1) ||(pnum == 5) ||(pnum == 6)){
				
				for(i=0;i<15;i++){
				
					tx[i].id = 5;//4;
					tx[i].len = 2;
					tx[i].data[0] = i;
					tx[i].data[1] = rx[0].data[0];
				
				
				}
				sz = 15;
				
				cfwCanWrite(h[pnum], tx, &sz, 1);
			}
			
	
			
		}
	}
	
}
