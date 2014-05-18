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
  char* mpLCDTopLineBuffer;
  char* mpLCDBotLineBuffer;
  int* mpNewBSEG;
  char mPositionChanged;

} xQueueItem;

void
TaskRecieveFromQueue(void* pParameters);
void
StateMachine(void* pParameters);
