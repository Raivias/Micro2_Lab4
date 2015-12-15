#ifndef LAB4PHOTO_H
#define LAB4PHOTO_H



#define Strobe     (26) // IO8
#define GP_4       (28) // IO4
#define GP_5       (17) // IO5
#define GP_6       (24) // IO6
#define GP_7       (27) // IO7
#define GP_9       (19) // IO9
#define GPIO_DIRECTION_IN      (1)
#define GPIO_DIRECTION_OUT     (0)
#define ERROR                  (-1)



//Buses that will be turned high and low
int BUS_A;
int BUS_B;
int BUS_C;
int BUS_D;
int BUS_STROBE; //This bus is the clock

int BUS_DEBUG; //This bus is used strictly for off board debugging

int openGPIO(int gpio, int direction);
int closeGPIO(int gpio, int fileHandle);
void writeGPIO(int fHandle, int value);
int readGPIO(int fHandle);
void write_msg(char msg);
char read_msg();
void set_write();
void set_read();
void delay(int milis);

int initPhotores();
int getPhotores();
int pingPhotores();
int resetPhotores();


#endif