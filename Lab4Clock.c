#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#include "colors.h"
#include "Lab4Photo.h"

#include "Lab4Clock.h"

const int CLOCKI2CADD = 0x68;

int openClockInterface(){
	//Set I2C lines
	int i2cHandle;
	initiateGPIO(GP_I2C);
	i2cHandle = openGPIO(GP_I2C, GPIO_DIRECTION_OUT);
	
	writeGPIO(i2cHandle, 0);
	close(i2cHandle);
	
	//Set or get time
	int deviceHandle;
	char buffer[7];
	
	buffer[0] = 0x00;

	if ((deviceHandle = open("/dev/i2c-0", O_RDWR)) < 0){
		printf("Couldn't connect to Clock\n");
		exit(-1);
	}

	if(ioctl(deviceHandle, I2C_SLAVE, CLOCKI2CADD) < 0){
		printf("Error in ioctl\n");
		exit(-1);
	}
	
	return deviceHandle;
}

int setClock(int deviceHandle){
	int status;

	char buffer[7];
	
	buffer[0] = 0x00;
	
	int sysSeconds, sysMinutes, sysHours, sysYears, sysDays, sysMonths;

	//Get time from user
	int userHours, userMinutes, userSeconds;
	printf("Hours?\n");
	scanf("%d", &userHours);
	printf("Min?\n");
	scanf("%d", &userMinutes);
	printf("Sec?\n");
	scanf("%d", &userSeconds);

	sysSeconds = userSeconds;
	sysMinutes = userMinutes;
	sysHours = userHours;

	printf("Sys time is: %d %d %d\n", sysHours, sysMinutes, sysSeconds);
		
	int userYears, userDay, userMonth;		
	printf("Year?\n");
	scanf("%d", &userYears);
	printf("Month?\n");
	scanf("%d", &userMonth);
	printf("Day?\n");
	scanf("%d", &userDay);
	sysYears = userYears %100 ;
	sysDays = userDay;
	sysMonths = userMonth;
	printf("Set Date is: %d/%d/%d\n", sysMonths, sysDays, sysYears);

	int lowSec = sysSeconds % 10;
	lowSec &= 0x0f;
	int highSec = (sysSeconds/10) << 4;
	highSec &= 0x70;
	int lowMin = sysMinutes % 10;
	lowMin &= 0x0f;
	int highMin = (sysMinutes/10) << 4;
	highMin &= 0x70;

	int miltime = 0b10111111;

	int lowHour = sysHours % 10;
	lowHour &= 0x0f;
	int highHour = (sysHours /10) << 4;
	highHour &= 0x30;
	int lowDay = sysDays % 10;
	lowDay &= 0x0f;
	int highDay = (sysDays /10) <<4;
	highDay &= 0x30;
	int lowMonth = sysMonths % 10;
	lowMonth &= 0x0f;
	int highMonth = (sysMonths / 10) << 4;
	highMonth &= 0x10;
	int lowYear = sysYears % 10;
	lowYear &= 0x0f;
	int highYear = (sysYears / 10 ) << 4;
	highYear &= 0xf0;

	
	buffer[0] = 0x00;	//first word, before we start writing data
	buffer[1] = ( highSec  | lowSec );
	buffer[2] = ( highMin | lowMin); //
	buffer[3] = ( highHour | lowHour);
	buffer[4] = 0x01;
	buffer[5] = (lowDay | highDay);
	buffer[6] = (lowMonth | highMonth);
	buffer[7] = (lowYear | highYear);

	//Enable these lines to see time data while storing it
	printf("Date: %d - %d - %d\n", sysYears, sysMonths, sysDays);
	printf("Time: %d : %d : %d\n", sysHours, sysMinutes, sysSeconds);
	

	status = write(deviceHandle, buffer, 8	);
	
	if(status != 8){
		printf("Error: more error! (no ack bit)\n");
		printf("%s(%d)\n", strerror(errno), errno);
		exit(-1);
	}
	return 1;
}

int* getClock(int deviceHandle, int *rval){

	int miltime = 0b10111111;
	
	int lowSec, highSec;
	int lowMin, highMin;
	int lowHour, highHour;
	int lowDay, highDay;
	int lowMonth, highMonth;
	int lowYear, highYear;
	
	int readStatus;
	int status;

	char buffer[7];
	buffer[0] = 0x00;
	
	status = write(deviceHandle, buffer, 1);
	if(status != 1){
		printf("Write Failure\n");
		exit(-1);
	}else{
		status = read(deviceHandle, buffer, 8);
		if(status != 8){
			printf("Read Failure\n");
			exit(-1);
		}
	}

	highSec = (0x70 & buffer[0])>>4;
	lowSec = 0x0f & buffer[0];
	
	lowMin = 0x0f & buffer[1];
	highMin = (0x70 & buffer[1])>>4;
	
	lowHour = 0x0f & buffer[2];
	highHour = (0x30 & buffer[2])>>4;
	
	lowDay = 0x0f & buffer[4];
	highDay = (0x30 & buffer[4])>>4;
	
	
	lowMonth = 0x0f & buffer[5];
	highMonth = (0x10 & buffer[5])>>4;


	lowYear = 0x0f & buffer[6];
	highYear = (0xf0 & buffer[6])>>4;

	 //// DEBUG STUFF!
	 //printf(ANSI_COLOR_RED);
  //   printf("Date: %d%d - %d%d - %d%d\n", highYear,lowYear,highMonth,lowMonth, highDay, lowDay);
  //   printf("Time: %d%d : %d%d : %d%d\n", highHour,lowHour, highMin,lowMin, highSec, lowSec);
  //   printf(ANSI_COLOR_RESET);
	 
	rval[0] = lowSec + highSec*10;
	rval[1] = lowMin + highMin*10;
	rval[2] = lowHour + highHour*10;
	rval[3] = lowDay + highDay*10;
	rval[4] = lowMonth + highMonth*10;
	rval[5] = lowYear + highYear*10;
	return rval;
}

int initiateGPIO (int gpio){

	int fileHandle;
	char buffer[256];
	
	fileHandle = open("/sys/class/gpio/export", O_WRONLY);
	if (ERROR == fileHandle)
	{
		printf("[\033[0;31m Error \033[m]\t Unable to open /sys/class/gpio/export\n");
		sleep(1);
		exit (-1);
	}
	else
	{
		sprintf(buffer, "%d", gpio);
		write(fileHandle, buffer, strlen(buffer));

		close(fileHandle);
	}

	return (0);
}

int initI2C() {
	printf("How the fuck is this getting called\n");
	
	int fileDirectory, ret;
	
	char *dev = "/dev/i2c-0";
	int addr = 0x68;
	
	fileDirectory = open(dev, O_RDWR);
	if(fileDirectory < 0){
		perror("opening i2c device node\n");
		return 1;
	}
	
	ret = ioctl(fileDirectory, I2C_SLAVE, addr);
	
	if(ret < 0){
		perror("Selecting I2C device\n");
	}
	
	return fileDirectory;
}