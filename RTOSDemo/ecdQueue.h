#ifndef ECDQUEUE_H_
#define ECDQUEUE_H_

#endif /*ECDQUEUE_H_*/


/*Queue defines*/
#define QUEUE_LENGTH 3
#define QUEUE_ITEM_SIZE sizeof(xQueueItem)

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

