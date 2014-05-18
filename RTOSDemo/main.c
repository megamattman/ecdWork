/*
    FreeRTOS V6.0.1 - Copyright (C) 2009 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/*******************************************************************
 * This example has been heavily modified from the original to model the 
 * examples in the RTOS manual 
 * NJG 02/12/09 */

/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_lcd_16207_regs.h"
#include "alt_types.h"
#include "altera_avalon_timer_regs.h"

/*state machine states*/
#define EMERGENCY        0
#define RESET            1
#define MOVE_LEFT        2
#define MOVE_RIGHT       3
#define MOVE_UP          4
#define MOVE_DOWN        5
#define MOVE_FORWARD     6
#define MOVE_BACKWARD    7
#define OPEN_GRIPPER     8
#define CLOSE_GRIPPER    9
#define RECORD_PATH     10
#define PLAYBACK        11
#define MENU            12
#define EXIT            13
/*LCD function characters*/
#define DELETE_LINE "\r\e[k"
#define CLEAR_SCREEN "\e[2J"
/*Queue defines*/
#define QUEUE_LENGTH 5
#define QUEUE_ITEM_SIZE sizeof(xQueueItem)
/*LCD defines*/
#define MAX_BUFFER_LENGTH 16
/*PWM defines*/
#define DEFAULT_X 29
#define DEFAULT_Y 0
#define DEFAULT_Z 15

#define X 0
#define Y 1
#define Z 2

#define BASE 0
#define SHOULDER 1
#define GRIPPER 2

#define PWM_BASE_0 0
#define PWM_BASE_1 1
#define PWM_BASE_2 2
#define PWM_BASE_3 3

#define ANGLE_BASE 0
#define ANGLE_SHOULDER_1 1
#define ANGLE_SHOULDER_2 2
#define ANGLE_ELBOW_1 3
#define ANGLE_ELBOW_2 4
#define ANGLE_WRIST 5

#define LINK_LENGTH_1  0.15    //lenght of link1 - between floor and robot base
#define LINK_LENGTH_2  0.1     //lenght of link2
#define LINK_LENGTH_3  0.12    //lenght of link3
#define LINK_LENGTH_4  0.07    //lenght of link4
#define PI  3.1415926535897
/*Item Struct*/
typedef struct
{
    short sTaskID;
    char* lineBuffer1;
    char* lineBuffer2;
}xQueueItem;


xSemaphoreHandle xMutex;

/*RTOS Scheduler Gloabls*/
xQueueHandle xQueue;

/*RTOS Shecduler tasks*/
void vTask1(void *pvParameters);
void vTask2(void *pvParameters);
void vTaskRecieve(void *pvParameters);
void vStateMachine(void* pvParameters);


/*LCD function Prototypes*/
void writeLine(int destLine, char* buffer);
void writeScreen(char* buffer1, char* buffer2);
void clearScreen();
void initialiseLCD();

/*PWM/ARM prototypes*/
void initialisePWM();

/*PWM Globals*/
int pwmBase[3] = {MY_PWM_0_BASE, MY_PWM_1_BASE, MY_PWM_2_BASE, MY_PWM_3_BASE};
int jointValues[3] = {0,0,0};
double currentXYZ[3] = {0.0,0.0,0.0};

/*LCD Globals*/
FILE *lcdFp;
char* currentLineBuffer1 = "DEADDEADDEADDEAD";
char* currentLineBuffer2 = "BEEFBEEFBEEFBEEF";

int main( void )
{
  initialiseLCD();
 
 xQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
 writeScreen("Starting Queue",""); 
 IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 0);
 xTaskCreate(vStateMachine, "task 1", 1000,(void *)1, 2, NULL);
 xTaskCreate(vTaskRecieve, "task 2", 1000,(void *)2, 1, NULL);
 
 writeScreen("Starting Scheduler","");      
    /* Finally start the scheduler. */
   vTaskStartScheduler();
    
   /* Will only reach here if there is insufficient heap available to start
   the scheduler. */
   for( ;; );
   
   
}
//this task is responsible for taking things off the queue
//and then calling the appropriate functions
void vTaskRecieve(void *pvParameters)
{
    xQueueItem xReceivedItem;
    portBASE_TYPE xStatus;    
    //writeScreen("Task Recieve",""); 
    //here the task is wating for a full queue
    for (;;)
    {        
        if (uxQueueMessagesWaiting(xQueue) != QUEUE_LENGTH)
        {
            //writeScreen("queue","not full");
            //usleep(10000);
            taskYIELD();
        }
    
    xStatus = xQueueReceive( xQueue, &xReceivedItem, 0);
    if (xStatus == pdPASS)
    {
        //writeScreen("Task Recieve",""); 
        switch (xReceivedItem.sTaskID)
        {           
            case 1:     writeScreen(xReceivedItem.lineBuffer1, xReceivedItem.lineBuffer2);
                        //IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, count);
                        break;
            default:    printf("Strange ID recieved\n");
                        break;
        }
    }
    //IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, count);
    vTaskDelay(100);
    }
    
}
//clear screen function, this will move the cursor to the begining on each
//line and then clear all characters on the line
void clearScreen()
{
   fprintf(lcdFp,CLEAR_SCREEN); 
   usleep(100);
}

void writeScreen(char* buffer1, char* buffer2)
{   
    char line1Write, line2Write;
    line1Write = strcmp(buffer1, currentLineBuffer1);   
    line2Write = strcmp(buffer2, currentLineBuffer2);
    IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, line1Write);
    if (line1Write != 0)
    {
       writeLine(1, buffer1);
       //free(currentLineBuffer1);       
       currentLineBuffer1 = buffer1;
       //free(buffer1);
       
    }
    if (line2Write !=0)
    {
        writeLine(2, buffer2);
        //free(currentLineBuffer2);
        currentLineBuffer2 = buffer2;
        //free(buffer2);
    }
}
void writeLine(int destLine, char* buffer)
{
  fprintf(lcdFp,"\e[%d;1H\e[K%s",destLine,buffer);  
  usleep(1000);
}

void vTask2(void *pvParameters)
{
 portBASE_TYPE xStatus;
 xQueueItem xTxPacket;
 char countText[16];
 int switchVal = 0;
 
 xTxPacket.lineBuffer1 = (char *) malloc (sizeof(char[MAX_BUFFER_LENGTH]));
 xTxPacket.lineBuffer2 = (char *) malloc (sizeof(char[MAX_BUFFER_LENGTH]));
 
 const portTickType xTicksToWait = 250 / portTICK_RATE_MS;
 //writeScreen("Task 2"," "); 
 for (;;)
 {
  //writeScreen("Inc Count","");     
  switchVal =   IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
  sprintf(countText,"%d", switchVal);
  xTxPacket.sTaskID = 1;
  xTxPacket.lineBuffer1 = SWITCH_TEXT;
  xTxPacket.lineBuffer2 = (char *)countText;
  xStatus = xQueueSendToBack(xQueue, &xTxPacket,portMAX_DELAY);
  if (xStatus != pdPASS)
  {
       writeScreen("Could not write to Q ","");
  }  
  vTaskDelay(xTicksToWait * xTxPacket.sTaskID * xTxPacket.sTaskID);
  
 }     
}

void vStateMachine(void* pvParameters)
{
    char* lcdTop;
    char* lcdBot;
    int state;
        
    portBASE_TYPE xStatus;
    xQueueItem xTxPacket;
    
    const portTickType xTicksToWait = 250 / portTICK_RATE_MS;
    
    for (;;)
    {
      state = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
      switch(state)
      {
              // Emergency Stop
          case EMERGENCY :
            {
                lcdTop = "Emergency Stop";
                lcdBot = "Press Q to enable";                
                // Stop ALL movements immediatly
                // Do not allow movement until some condition has been met
                break;
            }
        
            // Reset
        case RESET :
            {
                lcdTop = "Reset";
                lcdBot = ". . . . .";
                
                // Move the arm to the central position
                break;
            }
            // Move Left
        case MOVE_LEFT :
            {
                lcdTop = "Arm Moving Left";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move to the left
                break;
            }
            // Move Right
        case MOVE_RIGHT : 
            {
                lcdTop = "Arm Moving Right";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move to the right
                break;
            }
            // Move Up
        case MOVE_UP :
            {
                lcdTop = "Arm Moving Up";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move upwards
                break;
            }
            //Move Down
        case MOVE_DOWN :
            {
                lcdTop = "Arm Moving Down";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move downwards
                break;
            }
            //Move Forwards
        case MOVE_FORWARD :
            {
                lcdTop = "Arm Moving Fwd";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move forwards
                break;
            }
            // Move Backwards
        case MOVE_BACKWARD :
            {
                lcdTop = "Arm Moving Bck";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Cause the arm to move backwards
                break;
            }
            // Open gripper
        case OPEN_GRIPPER :
            {
                lcdTop = "Gripper Opening";
                lcdBot = "";
                
                // Cause the gripper to open
                break;
            }
            // Close gripper
        case CLOSE_GRIPPER :
            {
                lcdTop = "Gripper Closing";
                lcdBot = "";
                
                // Cause the gripper to close
                break;
            }
        
            // Record Path
        case RECORD_PATH :
            {
                lcdTop = "Recording Path";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Record all movements made until recoding is stopped
                break;
            }
            // Playback
        case PLAYBACK :
            {
                lcdTop = "Replaying Path";
                lcdBot = "X= x, Y= x, Z= z";
                
                // Carry out all recorded moves until stopped or completed
                break;
            }
            // Menu Function
        case MENU :
            {
                //TODO: think about menu options
                lcdTop = "Menu L/R Scrolls";
                lcdBot = "1: Option One";
                
                // Display a menu which allows control over settings etc.
            
                break;
            }
        case EXIT :
            {
                lcdTop = "Exiting";
                lcdBot = ". . . . ";
                break;
            }
        default :
            {
                printf ("No valid option was selected\n");
            }
        }//endcase  
        xTxPacket.sTaskID = 1;
        xTxPacket.lineBuffer1 = (char*) malloc (sizeof(char[MAX_BUFFER_LENGTH]));
        xTxPacket.lineBuffer2 = (char*) malloc (sizeof(char[MAX_BUFFER_LENGTH]));
        xTxPacket.lineBuffer1 = lcdTop;
        xTxPacket.lineBuffer2 = lcdBot;
        xStatus = xQueueSendToBack(xQueue, &xTxPacket,portMAX_DELAY);
        if (xStatus != pdPASS)
        {
            writeScreen("Could not write to Q ","");
        }  
        vTaskDelay(xTicksToWait * xTxPacket.sTaskID * xTxPacket.sTaskID);   
    }//end for
}//endfunc

void initialiseLCD()
{
    lcdFp = fopen("/dev/lcd","w+"); 
}

void initialiePWM(int* angle4In)
{    
    double defaultArmValues[3] = {DEFAULT_X, DEFAULT_Y, DEFAULT_Z}; 
    for (i = 0 ; i < 3 ; i++)
    {
      IOWR_ALTERA_AVALON_PWM_CLK_DIV(pwmBase[i], 100);
      IOWR_ALTERA_AVALON_PWM_PERIOD(pwmBase[i], 1000);
      IOWR_ALTERA_AVALON_PWM_CONTROL(pwmBase[i], 1);
    } 
    *angle4In = 0;
    iKRun(defaultArmValues, angle4in);
}

double convertToMeters(double ValueIn)
{
    return valueIn / 100;
}
/*
  Converts raw information to usable angle information that is written to the arm
  using inverse kinematics
  @xyz: the new data, this is a modified version of the current position
  @currentXyz: the current position this is used to write to the arm if xyz 
               is outside the acceptable range
  @grabberAngle: a value that is unique to manipulating the gripper
 */
void ikrun(double* xyz, double* currentXyz, int grabberAngle)
{
  double roationAngles[6] = {0.0,0.0,0.0,0.0,0.0,0.0}; 
  int roationAnglesInt[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
  double wristPosition[3] = {0.0,0.0,0.0};
  double cq1 = 0.0, c3 = 0.0, s3a = 0.0, s3b = 0.0, k1 = 0.0;
  double k2 = 0.0;
  double xyzMeters[3] = xyz;
    
  int angle2aint = 0;
  int angle3bint = 0;
    
  //change to meters  
  for (i = 0; i < 3 ; i++)
  {
    xyzMeters[i] = convertToMeters(xyz[i]);
  }
  //angle1 Base?
  rotationAngles[ANGLE_BASE] = atan2(xyzMeters[Y],xyzMeters[X]);
  cq1 = cos(rotationAngles[ANGLE_BASE]);
  //sq1 = sin(rotationAngles[ANGLE_BASE]);
  rotationAngles[ANGLE_BASE]  = rotationAngles[ANGLE_BASE] *(180/PI);

  wristPosition[X] = (xyzMeters[X] - (LINK_LENGTH_4*cq1))/cq1;  //wrist positions  T14
  wristPosition[Y] = 0;
  wristPosition[Z] = xyzMeters[Z] - LINK_LENGTH_1;

//angle 3 <-- WHAT DOES THIS EQUATE TO, WHAT IS ANGLE 3 ?ELBOW?
  c3 = ((wristPosition[X] * wristPosition[X]) + 
        (wristPosition[Z] * wristPosition[Z]) - 
        (LINK_LENGTH_2 * LINK_LENGTH_2) - 
        (LINK_LENGTH_3 * LINK_LENGTH_3)) /
        (LINK_LENGTH_2 * LINK_LENGTH_3 * 2);
        
  s3a = (1-(c3*c3));
  
  if (s3a < 0)
  {
    s3a = 0;
  }
  s3a = sqrt(s3a);
  s3b = -1* sqrt(s3a);
  rotationAngles[ANGLES_ELBOW_1] = (atan2(s3a,c3))/(PI/180);
  rotationAngles[ANGLES_ELBOW_2] = (atan2(s3b,c3))/(PI/180);
  

  //angle 2 WHAT DOES THIS EQUATE TO, WHAT IS ANGLE 2 Shoulder
  //cq3 = cos((angle3a)*(pi/180));
  //sq3 = sin((angle3a)*(pi/180));
  k1 = LINK_LENGTH_2+LINK_LENGTH_3*c3;
  k2 = LINK_LENGTH_3*s3b;
  r = sqrt((k1*k1) + (k2*k2));
  //val5 = (z4ik/r)/(x4ik/r);
  //val6 = (k2/k1);
  roationAngles[ANGLES_SHOULDER_1] =  ((atan2((xyzMeters[Z]/r),(xyzMeters[X]/r)))-(atan2(k2,k1))) / (PI/180);
  //  angle2a = angle2a/(pi/180);
  //val7 = (z4ik/r)/(x4ik/r);
  //val8 = ((-1*k2)/k1);
  roationAngles[ANGLES_SHOULDER_2] = ((atan2((z4ik/r),(x4ik/r)))-(atan2((-1*k2),k1))) / (PI/180);
  //roationAngles[ANGLES_SHOULDER_2] = angle2b/(pi/180);

  for (i = 0 ; i < 5; i++)
  {
    rotationAnglesInt[i] = (int)floor(rotationAngles[i]);  
  }
  //if within appropraite range update the current xyz co-ords            
  if((rotationAnglesInt[ANGLES_BASE] <= 45 && rotationAnglesInt[ANGLES_BASE] >= -45) && 
     (roationAngles[ANGLES_SHOULDER_2] <= 45 && roationAngles[ANGLES_SHOULDER_2] >= -45) )
  {
    jointValues[BASE] = map(rotationAnglesInt[ANGLES_BASE],-45,45,50,100);
    jointValues[SHOULDER] = map(roationAngles[ANGLES_SHOULDER_2],45,-45,50,100);
    jointValues[GRIPPER] = map(grabberAngle,0,1,50,100);
    
    for i = 0 ; i < 3; i++)
    {
      currentXyz[i] = xyz[i] -5;//what even is 5?
    }
  }
  //otherwise do not do anything - this is providing that Eholders retian the previous value
/*
  if(angle1int > 45 || angle1int < -45 || angle2bint > 45 || angle2bint < -45 )
  {
   //is this how it works? Does the value need to be kept and written?
    
    //parts[i] = parts[i];
    //   shoulder = shoudler;
    //   gripper = gripper;
    
    // xE = xEholder;
    //yE = yEholder;
    //zE = zEholder;
  }
*/  
  //the values that are written are global, what this means is that if they are
  //not updated then the previous values will be written to the PWMs                    
  IOWR_ALTERA_AVALON_PWM_DUTY(pwmBase[PWM_BASE_0], jointValues[BASE]);
  IOWR_ALTERA_AVALON_PWM_DUTY(pwmBase[PWM_BASE_1], jointValues[SHOULDER]);
  IOWR_ALTERA_AVALON_PWM_DUTY(pwmBase[PWM_BASE_3], jointValues[GRIPPER]);
}
