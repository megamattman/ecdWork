#ifndef EMU_H_
#define EMU_H_

#endif /*EMU_H_*/
/*PWM defines*/
#define DEFAULT_BASE 75
#define DEFAULT_SHOULDER 75
#define DEFAULT_ELBOW 83
#define DEFAULT_GRIPPER 51

#define BASE_MAX 95
#define BASE_MIN 40
#define ELBOW_MAX 110
#define ELBOW_MIN 53
#define SHOULDER_MAX 90
#define SHOULDER_MIN 45
#define GRIPPER_MIN 50
#define GRIPPER_MAX 100

#define GRIPPER_OPEN 99
#define GRIPPER_CLOSE 51


#define BASE 0
#define SHOULDER 1
#define ELBOW 2
#define GRIPPER 3

#define JOINTS 4

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

