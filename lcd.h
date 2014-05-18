#ifndef LCD_H_
#define LCD_H_

#endif /*LCD_H_*/

/*LCD function Prototypes*/
void WriteToLCDLine(int destLine, char* buffer);
void WriteToLCDScreen(char* buffer1, char* buffer2);
void ClearLCDScreen();
void InitialiseLCDScreen();
char* CreateBSEGString(int* apNewBSEG);

/*LCD function characters*/
#define DELETE_LINE "\r\e[k"
#define CLEAR_SCREEN "\e[2J"

#define MAX_BUFFER_LENGTH 16
