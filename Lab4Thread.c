/*
*	This file contains the thread setup for Lab 4
*
*
*/

#include <pthread.h>
#include <sys/ipc.h>
#include <ctype.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>


const int CMD_WAITING = 0; //Waiting for command
const int CMD_RUN = 1; //run the command
const int CMD_RAN = 2; //ran the command

typedef struct SensorInfo {
	int run; //used as enum
	char cmd;
	int resp;
	char timeStamp[10];
};

typedef struct ThreadInput{
	pthread_mutex_t *sensorComsMutexPt;
	struct SensorInfo *sensorDataPt;
	int *clockHandlePt;
	int *killSwitchPt;
	pthread_mutex_t *killSwitchMutexPt;
};


void * SensorComThread(void * param){
	/* This function acts as a main thread for interacting with the sensor clock is set in 
		UserFaceThread
	*/
	struct ThreadInput *input = param;
	
	int localKill = 0;
	while(localKill == 0){
		//lock mutex to check if there is a new command to run
		pthread_mutex_lock(input->sensorComsMutexPt);
		
		//check if there is a new command
		if( input->sensorDataPt->run == CMD_RUN){ //wait for a cmd to exe
			//if there is run it
			input->sensorDataPt->cmd = tolower(input->sensorDataPt->cmd);
			input->sensorDataPt->resp = cmdRun(input->sensorDataPt->cmd);
			
			//update timestamp
			getClock(*(input->clockHandlePt), &(input->sensorDataPt->timeStamp));
			
			//update as cmd run
			input->sensorDataPt->run = CMD_RAN; //Set run to have ran
		}
		//unlock mutex
		pthread_mutex_unlock(input->sensorComsMutexPt);	
		
		//Check if thread can kill itself
		pthread_mutex_lock(input->killSwitchMutexPt);
		localKill = *(input->killSwitchPt);
		pthread_mutex_unlock(input->killSwitchMutexPt);
	}
	pthread_exit(0);
	return;
}


void * UserThread(void * param){
	/* when this program begins it's assumed sensorComsMutexPt is locked */
	
	struct ThreadInput *input = param;
	
	//Lock sensorComMutex until clock is set
	pthread_mutex_lock(input->sensorComsMutexPt);
	
	printf("User Interface Open\n");
	printf("Please set clock: \n");
	setClock(*(input->clockHandlePt));
	
	//unlock mutex so Sensor Coms can run
	pthread_mutex_unlock(input->sensorComsMutexPt);
	
	//get and run commands
	char command;
	int commandRun;
	while(1){
		printf("Enter Command: ");
		scanf(" %c", &command); //get command
		command = tolower(command);
		
		//quit if command is q
		if(command == 'q'){
			pthread_mutex_lock(input->killSwitchMutexPt);
			*(input->killSwitchPt) = 1;
			pthread_mutex_unlock(input->killSwitchMutexPt);
			
			pthread_exit(0);
			return;
		}
		
		
		//give sensor coms the command
		int didItGoIn = 0;
		while (didItGoIn == 0){
			//check if the coms are open
			pthread_mutex_lock(input->sensorComsMutexPt);
			
			//if sensor coms is waiting for cmd set new cmd
			if(input->sensorDataPt->run == CMD_WAITING){
				input->sensorDataPt->cmd = command;
				input->sensorDataPt->run = CMD_RUN;
				didItGoIn = 1;
			}
			
			pthread_mutex_unlock(input->sensorComsMutexPt);

		}
		
		
		
		//loop until command is run
		int hasItRun = 0;
		struct SensorInfo sensorDataCopy;
		while (hasItRun == 0)
		{
			//lock mutex to check if it's run
			pthread_mutex_lock(input->sensorComsMutexPt);

			//if run is waiting for cmd set new cmd
			if(input->sensorDataPt->run == CMD_RAN){
				hasItRun = 1;
				sensorDataCopy = *(input->sensorDataPt);
				//let the world know it ran
				input->sensorDataPt->run = CMD_WAITING;
			}
			
			pthread_mutex_unlock(input->sensorComsMutexPt);

		}
		
		//print command results
		printf("Results are back:\n");
		// response hr:min:sec day/month/year
		printf("%d %d:%d:%d %d/%d/%d\n", sensorDataCopy.resp,
		sensorDataCopy.timeStamp[0], sensorDataCopy.timeStamp[1], sensorDataCopy.timeStamp[2],
		sensorDataCopy.timeStamp[3], sensorDataCopy.timeStamp[4], sensorDataCopy.timeStamp[5]);
		
	}
}

int main(){
	//initialize I2C
	initI2C();
	
	int killSwitch; //tells threads to end
	
	struct SensorInfo sensorData;
		sensorData.run = CMD_WAITING;
		sensorData.resp = 0x0F;
		sensorData.cmd = 0;
		//SensorData.timeStamp is not initialized
	
	//Set clock
	int clockHandle = openClockInterface();
	
	pthread_t sensorThreadAdd, clientThreadAdd, userThreadAdd; //Threads
	int eRetSen, eRetCli, eRetUser; //thread checkers
	
	
	
	//Mutexs
	pthread_mutex_t sensorComMutex;
	pthread_mutex_t killSwitchMutex;
	
		//create parameters for threads
		struct ThreadInput threadInput;
			threadInput.sensorComsMutexPt = &sensorComMutex;
			threadInput.sensorDataPt = &sensorData;
			threadInput.clockHandlePt = &clockHandle;
			threadInput.killSwitchPt = &killSwitch;
			threadInput.killSwitchMutexPt = &killSwitchMutex;
	
	
	pthread_mutex_init(&sensorComMutex, NULL);
    pthread_mutex_init(&killSwitchMutex, NULL);
    
	
	//TODO change UserFunction to actual function
	
	//Clock is set in UserFaceThread
	pthread_create( &userThreadAdd, NULL, &UserThread, &threadInput);
	
	//Wait a second for User interface to lock the sensor
	delay(1000);
	
	//Wait for clock to be set
	pthread_mutex_lock(&sensorComMutex);
	pthread_mutex_unlock(&sensorComMutex);
	
	//TODO change SensorComThread inputs!!!
	pthread_create( &sensorThreadAdd, NULL, &SensorComThread, &threadInput);
	
	//TODO change clientFunction to actual function
	//pthread_create( &clientThreadAdd, NULL, clientFunction, (void*) NULL);
	
	
	
	
	//wait for child producer to finish
	pthread_join(userThreadAdd, NULL);
	pthread_join(sensorThreadAdd,NULL);
	//pthread_join(clientThreadAdd,NULL);
	
	//close up shop
	pthread_mutex_destroy(threadInput.sensorComsMutexPt);
	pthread_mutex_destroy(threadInput.killSwitchMutexPt);
	
	exit(1);
	return 1;
	
}