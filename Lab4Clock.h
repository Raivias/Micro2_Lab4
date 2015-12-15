#ifndef LAB4CLOCK_H
#define LAB4CLOCK_H


#define GP_I2C   (29)
#define GPIO_DIRECTION_OUT  (0)
#define ERROR               (-1)

//const int CLOCKI2CADD = 0x68;

//function prototypes
int openClockInterface();
int setClock(int deviceHandle);
int* getClock(int deviceHandle, int *rval);
int initiateGPIO (int gpio);
int initI2C();

#endif
