#include "Lab4Clock.h"

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
	
	time_t result = time(NULL);
	
	int sysSeconds, sysMinutes, sysHours, sysYears, sysDays, sysMonths;

	if(result != -1)
	{
		struct tm *timeInfo;

		timeInfo = gmtime(&result);
		
		//Get time from user
		int userHours, userMinutes, userSeconds;
		printf("Hours?\n");
		scanf("%d", &userHours);
		printf("Min?\n");
		scanf("%d", &userMinutes);
		printf("Sec?\n");
		scanf("%d", &userSeconds);
		
	
		printf("%d %d %d\n", userHours, userMinutes, userSeconds);
		
		sysSeconds = timeInfo->tm_sec;
		sysMinutes = timeInfo->tm_min;
		sysHours = (timeInfo->tm_hour)%24;//HOUR
		printf("Sys time is: %d %d %d\n", sysHours, sysMinutes, sysSeconds);
		
		sysSeconds = userSeconds;
		sysMinutes = userMinutes;
		sysHours = userHours %24;
		printf("Set time is: %d %d %d\n", sysHours, sysMinutes, sysSeconds);
		
		sysYears = (timeInfo->tm_year+1900) %100 ;

		sysDays = timeInfo->tm_mday;
		sysMonths = (timeInfo->tm_mon+1);
		
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
		printf("Set Date is: %d %d %d\n", sysYears, sysMonths, sysDays);
		
	}
	
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

	if(status != 7){
		printf("Error: more error! (no ack bit)\n");
		exit(-1);
	}
}

int* getClock(int deviceHandle, int *rval){
	
	int lowSec, highSec;
	int lowMin, highMin;

	int miltime = 0b10111111;

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
		status = read(deviceHandle, buffer, 7);
		if(status != 7){
			printf("Read Failure\n");
			exit(-1);
		}
	}

	int year, month, day, hours, minutes, seconds;

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

	
	
	//printf("Date: %d%d - %d%d - %d%d\n", highYear,lowYear,highMonth,lowMonth, highDay,lowDay);
	//printf("Time: %d%d : %d%d : %d%d\n", highHour,lowHour, highMin,lowMin, highSec, lowSec);
	
	rval[0] = lowSec + highSec*10;
	rval[1] = lowMin + highMin*10;
	rval[2] = lowHour + highHour*10;
	rval[3] = lowDay + highDay*10;
	rval[4] = lowMonth + highMonth*10;
	rval[5] = lowYear;
	return rval;
}

int initiateGPIO (int gpio)
{

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
