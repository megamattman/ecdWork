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
#include <math.h>

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
#include "pwm.h"
#include "emu.h"
#include "lcd.h"
#include "ecdQueue.h"
/* Altera HAL includes for timestamp timer */
#include "sys/alt_timestamp.h"

/*state machine states*/
#define NO_BUTTON        0
#define MOVE_RIGHT       1
#define MOVE_LEFT        2
#define MOVE_UP          3
#define MOVE_DOWN        4
#define MOVE_FORWARD     5
#define MOVE_BACKWARD    6
#define MOVE_GRIPPER     7
#define RECORD           8
#define PLAY_PAUSE       9
//#define RECORD_PATH     10
//#define PLAYBACK        11
//#define MENU            12
#define RESET            15

#define MOVE_STEP     3

/*RTOS Scheduler Gloabls*/
xQueueHandle rtosQueue;

//PWM Globals
//BSEG = base shoulder elbow gripper
int PWMBase[4] =
  { MY_PWM_0_BASE, MY_PWM_1_BASE, MY_PWM_2_BASE, MY_PWM_3_BASE };
  
int currentBSEG[4] =  {DEFAULT_BASE, DEFAULT_SHOULDER, 
                            DEFAULT_ELBOW, DEFAULT_GRIPPER};
/*LCD Globals*/
FILE* pLCDFp;

//Hold the current value printed to the LCD screen
char* pCurrentLCDTopLineBuffer = "DEADDEADDEADDEAD";
char* pCurrentLCDBotLineBuffer = "BEEFBEEFBEEFBEEF";

int main(void)
{
  //initialistation
  InitialiseLCDScreen();
  InitialisePWM();
  
  rtosQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
  //WriteToLCDScreen("Starting Queue", "");
  
  xTaskCreate(StateMachine, "task 1", 1000, (void *) 1, 2, NULL);
  xTaskCreate(TaskRecieveFromQueue, "task 2", 1000, (void *) 2, 1, NULL);

 //WriteToLCDScreen("Starting Scheduler", "");
  /* Finally start the scheduler. */
  vTaskStartScheduler();

  /* Will only reach here if there is insufficient heap available to start
   the scheduler. */
  for (;;)
    ;

}
//this task is responsible for taking things off the queue
//and then calling the appropriate functions
void TaskRecieveFromQueue(void *pParameters)
{
  xQueueItem receivedItem;
  portBASE_TYPE status;
  //here the task is wating for a full queue, task yeilds cpu if not found
  for (;;)
    {
        
      if (uxQueueMessagesWaiting(rtosQueue) != QUEUE_LENGTH)
        {
          //writeScreen("queue","not full");
          //usleep(10000);
          taskYIELD();
        }
        

      status = xQueueReceive(rtosQueue, &receivedItem, 0);
      if (status == pdPASS)
        {
          //writeScreen("Task Recieve",""); 
          switch (receivedItem.mTaskID)
            {
          case 1:
            WriteToLCDScreen(receivedItem.mpLCDTopLineBuffer,
                receivedItem.mpLCDBotLineBuffer);
            if (receivedItem.mPositionChanged == 1)
              {
                WriteToPWM(receivedItem.mpNewBSEG, currentBSEG);
              }
            break;
          default:
            printf("Strange ID recieved\n");
            break;
            }
        }
      //IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, count);
      vTaskDelay(150);
    }

}
//clear screen function, this will move the cursor to the begining on each
//line and then clear all characters on the line
void ClearLCDScreen()
{
  fprintf(pLCDFp, CLEAR_SCREEN);
  usleep(100);
}
/*
 Determines if a write is needed by comparing current buffers with the passed in
 buffers
 @apTopLinebuffer: String to go on the top line of the LCD screen
 @apBottomLineBuffer: String to go on the bottom line of the LCD screen 
 */
void WriteToLCDScreen(char* apTopLinebuffer, char* apBottomLineBuffer)
{
  char lCDTopLineWrite, lCDBotLineWrite;
  lCDTopLineWrite = strcmp(apTopLinebuffer, pCurrentLCDTopLineBuffer);
  lCDBotLineWrite = strcmp(apBottomLineBuffer, pCurrentLCDBotLineBuffer);
  IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, lCDTopLineWrite);
  if (lCDTopLineWrite != 0)
    {
      WriteToLCDLine(1, apTopLinebuffer);
      pCurrentLCDTopLineBuffer = apTopLinebuffer;

    }
  if (lCDBotLineWrite != 0)
    {
      WriteToLCDLine(2, apBottomLineBuffer);
      pCurrentLCDBotLineBuffer = apBottomLineBuffer;
    }
}
/*
Writes characters to the specified line(destLine 1=top, 2=bottom) of the LCD screen
@pDestLine: The line to write to on the LCD Screen
@apBuffer: string to write to the LCD
*/
void WriteToLCDLine(int aDestLine, char* apBuffer)
{
  fprintf(pLCDFp, "\e[%d;1H\e[K%s", aDestLine, apBuffer);
  usleep(1000);
}

/*
 Task to recieve keypad presses and to create packets for other tasks to deal with
 @apParameters: used for passing in parameters, not implemented  
 
*/
void StateMachine(void* apParameters)
{
    
  char* lCDTopLineBuffer;
  char* lCDBotLineBuffer;
  int state;
  int positionChanged = 0;
  char stale;
  portBASE_TYPE status;
  xQueueItem txPacket;

  const portTickType mTicksToWait = 250 / portTICK_RATE_MS;

  for (;;)
    {
      //read from keypad  
      state = IORD_ALTERA_AVALON_PIO_DATA(COLLS_BASE);
      state &= 0x0F; 
      //state = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);

      int newBSEG[4];
        
      int iA;

      for (iA = 0; iA < 4; iA++)
        {
          newBSEG[iA] = currentBSEG[iA];
        }     
       
        IOWR_ALTERA_AVALON_PIO_DATA(ROWS_BASE, 1);  
        //state = (state & 0x10);      
      switch (state)
        {           
     /*
      // Emergency Stop
     // case EMERGENCY:
        {
          lCDTopLineBuffer = "Emergency Stop";
          lCDBotLineBuffer = "Press Q to enable";
          // Stop ALL movements immediatly
          // Do not allow movement until some condition has been met
          break;
      //  }
*/
        // Reset
/*      case RESET:
        {
          newXYZ[X] = DEFAULT_X;
          newXYZ[Y] = DEFAULT_Y;
          newXYZ[Z] = DEFAULT_Z;
          positionChanged = 1;
          lCDTopLineBuffer = "Reset";
          lCDBotLineBuffer = ". . . . .";

          // Move the arm to the central position
          break;
       }
       */
        // Move Left
        
      case MOVE_LEFT:
        {
          lCDTopLineBuffer = "Arm Moving Left";
          newBSEG[BASE] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1;  

          // Cause the arm to move to the left
          break;
        }
        
        
        // Move Right
      case MOVE_RIGHT:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[BASE] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1;         
          
          // Cause the arm to move to the right
          break;
        }   
        
        // Move Up
      case MOVE_UP:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[SHOULDER] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1; 
          // Cause the arm to move upwards
          break;
        }
        
        //Move Down
        
      case MOVE_DOWN:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[SHOULDER] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1; 
          // Cause the arm to move downwards
          break;
        }
        
        
        //Move Forwards
      case MOVE_FORWARD:
        {
           lCDTopLineBuffer = "Arm Moving fwd";
          newBSEG[ELBOW] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1; 
          // Cause the arm to move forwards
          break;
        }
                
        // Move Backwards        
      case MOVE_BACKWARD:
        {
          lCDTopLineBuffer = "Arm Moving bck";
          newBSEG[ELBOW] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          positionChanged = 1;
          // Cause the arm to move backwards
          break;
        }
        
        /*
        // Open gripper
      case MOVE_GRIPPER:
        {
          lCDTopLineBuffer = "Gripper Opening";
          lCDBotLineBuffer = "";
          positionChanged = 1;
          //Cause the gripper to open
          break;
        }
        */
        // Close gripper
        /*
      case CLOSE_GRIPPER:
        {
          lCDTopLineBuffer = "Gripper Closing";
          lCDBotLineBuffer = "";
          //mPositionChanged = 0;
          // Cause the gripper to close
          break;
        }
*/
        // Record Path
        /*
      case RECORD_PATH:
        {
          lCDTopLineBuffer = "Recording Path";
          lCDBotLineBuffer = CreateXYZString(newXYZ);

          // Record all movements made until recoding is stopped
          break;
        }
        */
        // Playback
        /*
      case PLAYBACK:
        {
          lCDTopLineBuffer = "Replaying Path";
          lCDBotLineBuffer = CreateXYZString(newXYZ);

          // Carry out all recorded moves until stopped or completed
          break;
        }
        */
        // Menu Function
        /*
      case MENU:
        {
          //TODO: think about menu options
          lCDTopLineBuffer = "Menu L/R Scrolls";
          lCDBotLineBuffer = "1: Option One";

          // Display a menu which allows control over settings etc.

          break;
        }
        */
        /*
      case EXIT:
        {
          lCDTopLineBuffer = "Exiting";
          lCDBotLineBuffer = ". . . . ";
          break;
        }
        */
      default:
        {
          //printf("No valid option was selected\n");
        }
        }                //endcase       
          
      txPacket.mTaskID = 1;
      txPacket.mpLCDTopLineBuffer = lCDTopLineBuffer;
      txPacket.mpLCDBotLineBuffer = lCDBotLineBuffer;
      txPacket.mPositionChanged = positionChanged;
      txPacket.mpNewBSEG = (int *) newBSEG;
      status = xQueueSendToBack(rtosQueue, &txPacket, portMAX_DELAY);
      if (status != pdPASS)
        {
          WriteToLCDScreen("Could not write to Q ", "");
        }
      //vTaskDelay(mTicksToWait * txPacket.mTaskID * txPacket.mTaskID);
      vTaskDelay(250);
     // }//end If oldbutton!=state      
    }//end foe    
}//endfunc

char* CreateBSEGString(int* apNewBSEG)
{
  char* pAngleStringBuffer;
  pAngleStringBuffer = (char *) malloc(sizeof(char[MAX_BUFFER_LENGTH]));
  
  sprintf(pAngleStringBuffer, "B=%d S=%d E=%d\n", apNewBSEG[BASE],
      apNewBSEG[SHOULDER], apNewBSEG[ELBOW]);
  return pAngleStringBuffer;
}

void InitialiseLCDScreen()
{
  pLCDFp = fopen("/dev/lcd", "w+");
}

void InitialisePWM()
{
  int i; 
  for (i = 0; i < 3; i++)
    {
      IOWR_ALTERA_AVALON_PWM_CLK_DIV(PWMBase[i], 100);
      IOWR_ALTERA_AVALON_PWM_PERIOD(PWMBase[i], 1000);
      IOWR_ALTERA_AVALON_PWM_CONTROL(PWMBase[i], 1);
    }
  
  IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[ELBOW], currentBSEG[ELBOW]);
  IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[BASE], currentBSEG[BASE]);
  IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[SHOULDER], currentBSEG[SHOULDER]);
  IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[GRIPPER], currentBSEG[GRIPPER]);
  
  //CalculateMovementAngles(defaultArmValues, cur_xyz, lastLegalXYZ,
  //  *gripperState);
}

void WriteToPWM(int* newBSEG, int* currentBSG)
{
    int iA = 0;
   for (iA = 0; iA < 3; iA++)
   {
    if (newBSEG[iA] > 50 && newBSEG[iA] < 100)
    {
       IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[iA], newBSEG[iA]);
       currentBSEG[iA] = newBSEG[iA];
    }
    else
    {
       IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[iA], currentBSEG[iA]);
    }
   }    
    
}
