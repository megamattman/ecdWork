#ifndef PWM_H_
#define PWM_H_
#define IORD_ALTERA_AVALON_PWM_DUTY(base)             IORD(base, 0) 
#define IOWR_ALTERA_AVALON_PWM_DUTY(base, data)       IOWR(base, 0, data)
#define IORD_ALTERA_AVALON_PWM_CLK_DIV(base)          IORD(base, 1) 
#define IOWR_ALTERA_AVALON_PWM_CLK_DIV(base, data)    IOWR(base, 1, data)
#define IORD_ALTERA_AVALON_PWM_PERIOD(base)          IORD(base, 2) 
#define IOWR_ALTERA_AVALON_PWM_PERIOD(base, data)    IOWR(base, 2, data)
#define IORD_ALTERA_AVALON_PWM_CONTROL(base)           IORD(base, 3) 
#define IOWR_ALTERA_AVALON_PWM_CONTROL(base, data)     IOWR(base, 3, data)
#endif /*PWM_H_*/


