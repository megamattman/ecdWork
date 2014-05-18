#ifndef EMU_H_
#define EMU_H_

#endif /*EMU_H_*/
/*PWM defines*/
#define DEFAULT_BASE 75
#define DEFAULT_SHOULDER 75
#define DEFAULT_ELBOW 83
#define DEFAULT_GRIPPER 100

#define BASE 0
#define SHOULDER 1
#define ELBOW 2
#define GRIPPER 3

//angle array indexes
#define X 0
#define Y 1
#define Z 2

#define BASE 0
#define SHOULDER1 1
#define SHOULDER2 2
#define ELBOW1 3
#define ELBOW2 4

#define LINK_LENGTH_1  0.15    //lenght of link1 - between floor and robot base
#define LINK_LENGTH_2  0.1     //lenght of link2
#define LINK_LENGTH_3  0.12    //lenght of link3
#define LINK_LENGTH_4  0.07    //lenght of link4
#define PI  3.1415926535897


/*PWM/ARM prototypes*/
void InitialisePWM();
void WriteToPWM(int* newBSEG, int* currentBSG);
