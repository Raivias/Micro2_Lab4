/************************************************************************/
/*  This code is designed to set DS1307 chip then get data from the		*/
/*photo resister over serial											*/
/* Authors: Group Steve													*/
/************************************************************************/

#include "Lab4Clock.h"

const int WAITTIME = 00;

void main(){	
	//Set clock
	int clockHandle = openClockInterface();
	setClock(clockHandle);
	
	//loop
	while(1)
	{
		int photoData = 0;
		//wait time
		delay(WAITTIME);
		
		//get photores data
		photoData = getPhotores();
		printf("Light was: %d at ", photoData); 
		
		//get time
		int timeData[10];
		getClock(clockHandle);
		//display
		printf("%d %d %d %d:%d:%d\n", timeData[6], timeData[5], timeData[7], timeData[2], timeData[1], timeData[0]);	//hour, min, sec0
	}
}