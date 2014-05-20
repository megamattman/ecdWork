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
#define PLAYBACK        11
//#define MENU            12
#define RESET            15

#define MOVE_STEP     3

#define KEYPAD_TASK_DELAY 24
#define PWM_TASK_DELAY 51
#define LCD_TASK_DELAY 56




#define LCD_TASK_ID 4
#define KEYPAD_TASK_ID 2
#define PWM_TASK_ID 1

/*RTOS Scheduler Gloabls*/


//PWM Globals
//BSEG = base shoulder elbow gripper
int PWMBase[4] =
  { MY_PWM_0_BASE, MY_PWM_1_BASE, MY_PWM_2_BASE, MY_PWM_3_BASE }; 
int PWMMax[4] = {95,90,110,100};
int PWMMin[4] = {50,45,53,0};
  
/*LCD Globals*/
FILE* pLCDFp;

//Hold the current value printed to the LCD screen
DeviceData* deviceData;

int storeOffset =0,playbackOffset = 0, playback =0, record =0; 
int storedBSEG[300];

int main(void)
{
  //initialistation
  deviceData = malloc (sizeof(DeviceData));   
  InitialiseLCDScreen();
  InitialisePWM();
   usleep(200);
  //printf("%ld\n", sizeof(DeviceData));
  //getchar();
  
  //xTaskCreate(LCDWriteTask, "lcd write task", 100, (void *) 1, 3, NULL);
  xTaskCreate(PWMWriteTask, "pwm write task", 100, (void *) 3, 1, NULL);
  xTaskCreate(KeyPadReadTask, "Keypad Read task", 100, (void *) 2, 2, NULL);
  
  //WriteToLCDScreen("Starting Scheduler", "");
  /* Finally start the scheduler. */
  vTaskStartScheduler();

  /* Will only reach here if there is insufficient heap available to start
   the scheduler. */
  for (;;)
    ;
}
/*
clear screen function, this will move the cursor to the begining on each
line and then clear all characters on the line
*/
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
void LCDWriteTask(void *pParameters)
{
  char lCDTopLineWrite = 1 , lCDBotLineWrite =1;
  for (;;)
  { 
    IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 4);
    if (lCDTopLineWrite != 0)
      {
        WriteToLCDLine(1, deviceData->mpNewLCDTopLineBuffer);
        deviceData->mpCurrentLCDTopLineBuffer = deviceData->mpNewLCDTopLineBuffer;  
      }
    if (lCDBotLineWrite != 0)
      {
        WriteToLCDLine(2, deviceData->mpNewLCDBotLineBuffer);
        deviceData->mpCurrentLCDBotLineBuffer = deviceData->mpNewLCDBotLineBuffer;
      }
      vTaskDelay(LCD_TASK_DELAY);
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
void KeyPadReadTask(void* apParameters)
{
    
  char* lCDTopLineBuffer;
  char* lCDBotLineBuffer;
  int state, iA, prevState;  
  int newBSEG[4];
  
  for (;;)
    {
      IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 1);
      //read from keypad  
      state = IORD_ALTERA_AVALON_PIO_DATA(COLLS_BASE);
      state &= 0x0F; //mask out stale bit      
      //printf("keypad read task %d\n", state);
      //state = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);  //read switches for debugging w/o keypad

    // printf("key curr: B=%d S=%d E=%d G=%d\n", deviceData->mpCurrentBSEG[BASE], deviceData->mpCurrentBSEG[SHOULDER], 
    //                              deviceData->mpCurrentBSEG[ELBOW], deviceData->mpCurrentBSEG[GRIPPER]);

      for (iA = 0; iA < 4; iA++)
        {
          newBSEG[iA] = deviceData->mpCurrentBSEG[iA];
        }     
       
        IOWR_ALTERA_AVALON_PIO_DATA(ROWS_BASE, 1); 
        //printf("keypad state = %x\n", state);
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
     case RESET:
        {
          newBSEG[BASE] = DEFAULT_BASE;
          newBSEG[SHOULDER] = DEFAULT_SHOULDER;
          newBSEG[ELBOW] = DEFAULT_ELBOW;          
          lCDTopLineBuffer = "Reset";
          lCDBotLineBuffer = ". . . . .";
          playback = 0;
          record = 0;

          // Move the arm to the central position
          break;
       }
       
        // Move Left
        
      case MOVE_LEFT:
        {
          lCDTopLineBuffer = "Arm Moving Left";
          newBSEG[BASE] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);

          // Cause the arm to move to the left
          break;
        }
        
        
        // Move Right
      case MOVE_RIGHT:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[BASE] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG); 
          // Cause the arm to move to the right
          break;
        }   
        
        // Move Up
      case MOVE_UP:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[SHOULDER] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          // Cause the arm to move upwards
          break;
        }
        
        //Move Down
        
      case MOVE_DOWN:
        {
          lCDTopLineBuffer = "Arm Moving Right";
          newBSEG[SHOULDER] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          // Cause the arm to move downwards
          break;
        }
        
        
        //Move Forwards
      case MOVE_FORWARD:
        {
           lCDTopLineBuffer = "Arm Moving fwd";
          newBSEG[ELBOW] += MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
          // Cause the arm to move forwards
          break;
        }
                
        // Move Backwards        
      case MOVE_BACKWARD:
        {
          lCDTopLineBuffer = "Arm Moving bck";
          newBSEG[ELBOW] -= MOVE_STEP;
          lCDBotLineBuffer = CreateBSEGString(newBSEG);
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
        
      case RECORD:
        {
          lCDTopLineBuffer = "Recording Path";
          lCDBotLineBuffer = ".....";
          record = 1;
          // Record all movements made until recoding is stopped
          break;
        }
        
        // Playback
        
      case PLAY_PAUSE:
        {
          lCDTopLineBuffer = "Replaying Path";
          lCDBotLineBuffer = ".....";
          playback =1;
          record = 0;
          // Carry out all recorded moves until stopped or completed
          break;
        }
        
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
          lCDTopLineBuffer = deviceData->mpCurrentLCDTopLineBuffer;
          lCDBotLineBuffer = deviceData->mpCurrentLCDBotLineBuffer;
        }
        }                //endcase       
      WriteToLCDLine(1, lCDTopLineBuffer);    
      WriteToLCDLine(2, lCDBotLineBuffer);
      deviceData->mpCurrentLCDTopLineBuffer = lCDTopLineBuffer;
      deviceData->mpCurrentLCDBotLineBuffer = lCDBotLineBuffer;
      
       // printf("keypadlocal: B=%d S=%d E=%d G=%d\n", newBSEG[BASE], newBSEG[SHOULDER], 
        //                          newBSEG[ELBOW], newBSEG[GRIPPER]);
      
      for (iA = 0; iA < 4; iA++)
      {
        deviceData->mpNewBSEG[iA] = newBSEG[iA];  
      }
      
     
      
      vTaskDelay(KEYPAD_TASK_DELAY);
     // }//end If oldbutton!=state      
    }//end foe    
}//endfunc

char* CreateBSEGString(int* apNewBSEG)
{
  char* pPositionBuffer;
  pPositionBuffer = (char *) malloc(sizeof(char[MAX_BUFFER_LENGTH]));
  
  sprintf(pPositionBuffer, "B=%d S=%d E=%d\n", apNewBSEG[BASE],
      apNewBSEG[SHOULDER], apNewBSEG[ELBOW]);
  return pPositionBuffer;
}

void InitialiseLCDScreen()
{
  pLCDFp = fopen("/dev/lcd", "w+");
  deviceData->mpNewLCDTopLineBuffer = "Starting System";
  deviceData->mpNewLCDBotLineBuffer = ".......";
  deviceData->mpCurrentLCDTopLineBuffer = "DEAD";
  deviceData->mpCurrentLCDBotLineBuffer = "BEEF";
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
    
  deviceData->mpNewBSEG[BASE] = DEFAULT_BASE;
  deviceData->mpNewBSEG[ELBOW] = DEFAULT_ELBOW;
  deviceData->mpNewBSEG[SHOULDER] = DEFAULT_SHOULDER;
  deviceData->mpNewBSEG[GRIPPER] = DEFAULT_GRIPPER;
  
  deviceData->mpCurrentBSEG[BASE] = DEFAULT_BASE;
  deviceData->mpCurrentBSEG[ELBOW] = DEFAULT_ELBOW;
  deviceData->mpCurrentBSEG[SHOULDER] = DEFAULT_SHOULDER;
  deviceData->mpCurrentBSEG[GRIPPER] = DEFAULT_GRIPPER;
 // printf("B=%d S=%d E=%d G=%d\n", deviceData->mpNewBSEG[BASE], deviceData->mpNewBSEG[SHOULDER], 
  //                                deviceData->mpNewBSEG[ELBOW], deviceData->mpNewBSEG[GRIPPER]);
}

void PWMWriteTask(void* pParameters)
{
   int iA = 0;
   int PWMBSEG[4] = {0,0,0,0};    
   for(;;){
    IOWR_ALTERA_AVALON_PIO_DATA(LEDS_BASE, 2);
    //printf("pwm write task\n");
       //printf("pwm: B=%d S=%d E=%d G=%d\n", deviceData->mpNewBSEG[BASE], deviceData->mpNewBSEG[SHOULDER], 
       //                           deviceData->mpNewBSEG[ELBOW], deviceData->mpNewBSEG[GRIPPER]);
    //printf("B=%d S=%d E=%d G=%d\n", base, shoulder, elbow, gripper);
    
    //determine mode, if playback active look at stored values 
     if (playback == 1)
     {
       for (iA = 0; iA < 4; iA++)
       { 
         PWMBSEG[iA] = storedBSEG[(iA+(playbackOffset*3))];
         playbackOffset += 1;
       } 
     }
     else
     {
       for (iA = 0; iA < 4; iA++)
       { 
         PWMBSEG[iA] = deviceData->mpNewBSEG[iA];
       } 
     }  
       
       
     for (iA = 0; iA < 3; iA++)
     { 
        if (PWMBSEG[iA] > PWMMin[iA] && PWMBSEG[iA] < PWMMax[iA])
        {
           IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[iA], PWMBSEG[iA]);
           if (playback == 0)
           {            
             deviceData->mpCurrentBSEG[iA] = PWMBSEG[iA];
           }
           if (record == 1 && deviceData->mpNewBSEG[iA] != storedBSEG[(iA+(storeOffset*3))])
           {
             //increment offset and save current pwm values
             storeOffset +=1;
             storedBSEG[(iA+storeOffset)] = PWMBSEG[iA]; 
             if (storeOffset >= 100)
             {
                record = 0;                
             }          
           }
        }
        else
        {
           IOWR_ALTERA_AVALON_PWM_DUTY(PWMBase[iA], deviceData->mpCurrentBSEG[iA]);
        }      
      }      
     vTaskDelay(PWM_TASK_DELAY);
   }    
    
}
