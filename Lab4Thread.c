/*
*	This file contains the thread setup for Lab 4, and now a test of the emergency file transfer system
*
*	Also, appears to be the only file that does anything? Not sure.
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
#include <stdio.h>
#include <curl/curl.h>

//locals! Not that they're used! What the fuck!
//#include "SensorComs.h"
#include "colors.h"


const int CMD_WAITING = 0; //Waiting for command
const int CMD_RUN = 1; //run the command
const int CMD_RAN = 2; //ran the command
const int CMD_DIE = 3;

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
	pthread_mutex_t *killSwitchMutexPt;
	int *killSwitchPt;
};

//Globals is easies
struct SensorInfo mostRecentSense;
pthread_mutex_t record_lock;

//WIP talk to Xav about this
void * SensorComThread(void * param){
	/* This function acts as a main thread for interacting with the sensor clock is set in 
		UserFaceThread
	*/
	struct ThreadInput *input = param;
	
	while(1){
		//lock mutex to check if there is a new command to run
		pthread_mutex_lock(input->sensorComsMutexPt);
		
		//check if there is a new command
		if( input->sensorDataPt->run == CMD_RUN){ //wait for a cmd to exe
			if (input->sensorDataPt->cmd == CMD_DIE) {
				printf("Ending sensor communications thread...\n");
				return;
			}
			
			printf("Recieved a new command, cap'n\n");
			//if there is run it
			input->sensorDataPt->resp = cmdRun(tolower(input->sensorDataPt->cmd));
			
			//update timestamp
			int buff[6];
			getClock(*(input->clockHandlePt), buff);

			input->sensorDataPt->timeStamp[0] = buff[0];
			input->sensorDataPt->timeStamp[1] = buff[1];
			input->sensorDataPt->timeStamp[2] = buff[2];
			input->sensorDataPt->timeStamp[3] = buff[3];
			input->sensorDataPt->timeStamp[4] = buff[4];
			input->sensorDataPt->timeStamp[5] = buff[5];

			//Debug print!
			//printf(ANSI_COLOR_RED "%d:%d:%d %d/%d/%d\n" ANSI_COLOR_RESET, input->sensorDataPt->timeStamp[2], input->sensorDataPt->timeStamp[1], input->sensorDataPt->timeStamp[0], input->sensorDataPt->timeStamp[3], input->sensorDataPt->timeStamp[4], input->sensorDataPt->timeStamp[5]);
			
			//update as cmd ran
			input->sensorDataPt->run = CMD_RAN; //Set run to have ran
		}
		//unlock mutex
		pthread_mutex_unlock(input->sensorComsMutexPt);	
	}
}


void * UserThread(void * param){
	/* when this program begins it's assumed sensorComsMutexPt is locked */ //->WIP not sure this is true, ask Xav
	
	struct ThreadInput *input = param;	//type safety is for languages that aren't C.
	
	//Lock sensorComMutex until clock is set
	pthread_mutex_lock(input->sensorComsMutexPt);
	
	//WIP this is UI handling across threads - probably revisit if time
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
			pthread_mutex_lock(input->sensorComsMutexPt);
			input->sensorDataPt->run = CMD_RUN;
			input->sensorDataPt->cmd = CMD_DIE;
			pthread_mutex_unlock(input->sensorComsMutexPt);

			pthread_mutex_lock(input->killSwitchMutexPt);
			*(input->killSwitchPt) = 1;
			pthread_mutex_unlock(input->killSwitchMutexPt);

			pthread_exit(0);
			return;	//Eh?
		}
		
		
		//give sensor coms the command
		int didItGoIn = 0;	//I think this made sense to xav when he wrote it
		while (didItGoIn == 0){
			//check if the coms are open
			pthread_mutex_lock(input->sensorComsMutexPt);
			
			//if sensor coms is waiting for cmd set new cmd
			if(input->sensorDataPt->run == CMD_WAITING){
				printf("Submitting new command\n");
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
				printf("reading results of last command\n");
				sensorDataCopy = *(input->sensorDataPt);

				pthread_mutex_lock(&record_lock);

				mostRecentSense.resp = sensorDataCopy.resp;
				mostRecentSense.timeStamp[0] = sensorDataCopy.timeStamp[0];
				mostRecentSense.timeStamp[1] = sensorDataCopy.timeStamp[1];
				mostRecentSense.timeStamp[2] = sensorDataCopy.timeStamp[2];
				mostRecentSense.timeStamp[3] = sensorDataCopy.timeStamp[3];
				mostRecentSense.timeStamp[4] = sensorDataCopy.timeStamp[4];
				mostRecentSense.timeStamp[5] = sensorDataCopy.timeStamp[5];

				pthread_mutex_unlock(&record_lock);


				input->sensorDataPt->run = CMD_WAITING;
				hasItRun = 1;
			}
			
			pthread_mutex_unlock(input->sensorComsMutexPt);

		}
		
		//print command results
		printf("Results are back:\n");
		// response hr:min:sec day/month/year
		printf("%d %d:%d:%d %d/%d/%d\n", sensorDataCopy.resp,
		sensorDataCopy.timeStamp[2], sensorDataCopy.timeStamp[1], sensorDataCopy.timeStamp[0],
		sensorDataCopy.timeStamp[3], sensorDataCopy.timeStamp[4], sensorDataCopy.timeStamp[5]);
		
	}
}

//laziness inlining - the linking here is already really weird and my brain isn't working all that well right now.
void HTTP_GET(const char* url) {
	CURL *curl;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
}

//more laziness inlining
void * doWebStuff(void* params) {
	while (1) {


		pthread_mutex_lock(&record_lock);
		if (mostRecentSense.resp == -1) {
			pthread_mutex_unlock(&record_lock);
			sleep(1);
			continue;
		}

		char buf[1024];
		const char* hostname = "localhost";
		const int   port = 8000;
		const int   id = 1;
		const char* password = "password";
		const char* name = "Team_Steve";
		const int   adcval = mostRecentSense.resp;
		const char* status = "Tired";
		snprintf(buf, 1024, "%d%d%d-%d:%d:%d\n", mostRecentSense.timeStamp[5], mostRecentSense.timeStamp[4], mostRecentSense.timeStamp[3], mostRecentSense.timeStamp[2], mostRecentSense.timeStamp[1], mostRecentSense.timeStamp[0]);
		const char* timestamp = buf;

		pthread_mutex_unlock(&record_lock);

		snprintf(buf, 1024, "http://%s:%d/update?id=%d&password=%s&name=%s&data=%d&status=%s&timestamp=%s", hostname, port, id, password, name, adcval, status, timestamp);
		HTTP_GET(buf);
		printf("%s", buf);
		sleep(1);
	}
	return NULL;
}


int main(){
	//initialize I2C
	initI2C();
	
	pthread_mutex_init(&record_lock, NULL);
	mostRecentSense.resp = -1;

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

	//I'm really tired
	pthread_t webthread;
	pthread_create(&webthread, NULL, &doWebStuff, NULL);
	
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