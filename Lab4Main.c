/************************************************************************/
/*  This code is designed to set DS1307 chip then get data from the		*/
/*photo resister over serial											*/
/* Authors: Group Steve													*/
/************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "colors.h"

#include "Lab4Clock.h"
#include "Lab4Photo.h"

const int WAITTIME = 00;

void main(){	
	
	initI2C();
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
		getClock(clockHandle, timeData);
		//display
		printf("%d:%d:%d on %d/%d/%d\n", 
		 timeData[2], timeData[1], timeData[0],
		 timeData[4], timeData[3], timeData[5]);
	}
}