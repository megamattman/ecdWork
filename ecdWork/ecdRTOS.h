#ifndef ECDQUEUE_H_
#define ECDQUEUE_H_

#endif /*ECDQUEUE_H_*/

//Task specific defines
#define KEYPAD_TASK_DELAY 24
#define PWM_TASK_DELAY 51

#define KEYPAD_TASK_ID 2
#define PWM_TASK_ID 1

#define KEYPAD_TASK_PRIORITY 2 
#define PWM_TASK_PRIORITY 1

#define TASK_STACK_DEPTH 100

/*Item Struct*/
typedef struct
{
  short mTaskID;
  char* mpNewLCDTopLineBuffer;
  char* mpNewLCDBotLineBuffer;
  char* mpCurrentLCDTopLineBuffer;
  char* mpCurrentLCDBotLineBuffer;
  int  mpNewBSEG[4];
  int  mpCurrentBSEG[4];
} DeviceData;

void LCDWriteTask(void* pParameters);
void PWMWriteTask(void* pParameters);
void KeyPadReadTask(void* pParameters);

