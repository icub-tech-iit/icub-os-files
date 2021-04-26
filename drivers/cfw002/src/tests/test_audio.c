#include <fcntl.h>
#include <stdio.h>
#include <libcfw002.h>



int main(int argc, char *argv[])
{
	int res; 
	unsigned char gain;
	CFWAUDIO_HANDLE ah;
	
	if(argc != 2){
		fprintf(stderr,"please specify (only) audiogain\n");
		return -1;
	}
	gain = atoi(argv[1]);
	

	printf("setting audio gain to %d\n",gain);

	res =  cfwAudioOpen(&ah);	

	if(res != -1){
		
		res =cfwAudioSetGain(&ah,gain);
		//printf("res %d\n",res);
		
	}else{
		fprintf(stderr,"can't open CFW002 res %d\n",res);
	
	}
	
	
	
}
