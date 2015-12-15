#include "SensorComs.h"
#include "Lab4Photo.h"
#include "Lab4Clock.h"
#include <pthread.h>
#include <ctype.h>
#include <stdio.h>

const int SENS_THREAD_DELAY = 0;

struct SensorInfo {
	bool run;
	char cmd;
	int resp;
	char timeStamp[7];
};


void SensorComThread(pthread_mutex_t *sensorComsMutexPt, SensorInfo *sensorDataPt){
	printf("What the fuck.\n");
	/* This function acts as a main thread for interacting with the sensor */
	
	//initialize I2C
	initI2C();
	//Set clock
	int clockHandle = openClockInterface();
	setClock(clockHandle);
	
	
	int mutexCheck;
	while(1){
		//lock mutex to check if there is a new command to run
		mutexCheck = pthread_mutex_lock(sensorComsMutexPt);
		if (mutexCheck != 0){
			exit(-6);
		}
		
		//check if there is a new command
		if( sensorDataPt->run == 0){
			printf("Recieved new command, cap'n\n");
			//if there is run it
			sensorDataPt->cmd = tolower(sensorDataPt->cmd);
			sensorDataPt->resp = cmdRun(sensorDataPt->cmd);
			
			//update timestamp
			getClock(clockHandle, sensorDataPt->timeStamp);
			
			//update as cmd run
			sensorDataPt->run = 1;
		}
		
		//unlock mutex
		mutexCheck = pthread_mutex_unlock(sensorComsMutexPt);
		if (mutexCheck != 0){
			exit(-6);
		}
		
	}
}