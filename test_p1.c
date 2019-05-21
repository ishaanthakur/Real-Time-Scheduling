#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"

/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/


/** Periodic test case*/
/** Test Case 1- Two real time processes with prt1 having an earlier arrival time compared
to prt2. As both the processes are periodic, we observe prt 1 executes first and after its completion
prt2 is executed. However, as both of them are periodic the cycle is repeated again. Hence expected output
is blue flashes followed by red running forever. We would also never notice a green led flash as bot the 
processes do not miss their deadline.*/


 
/* Stack space for processes */
#define NRT_STACK 20
#define RT_STACK  20
 


/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/

/* Constants used for 'work' and 'deadline's */
realtime_t t_2msec = {0, 2};

realtime_t t_1sec = {1, 0};
realtime_t t_6sec = {6, 0};
realtime_t t_3sec = {3, 0};
realtime_t t_7sec = {7, 0};
realtime_t t_8sec = {8, 0};
realtime_t t_10sec = {10, 0};
realtime_t t_20sec = {20, 0};
realtime_t t_5sec = {5, 0};



/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}


 


/*-------------------
 * Real-time process
 *-------------------*/

void pRT2(void) {
	int i;
	for (i=0; i<3;i++){
	LEDBlue_Toggle();
	shortDelay();
	LEDBlue_Toggle();
	shortDelay();
	}
}

void pRT1(void) {
	int i;
	for (i=0; i<3;i++){
	LEDRed_Toggle();
	shortDelay();
	LEDRed_Toggle();
	shortDelay();
	}
}



/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
	
	 if (process_rt_periodic(pRT2, RT_STACK, &t_1sec, &t_3sec, &t_8sec) < 0) { return -1; } 
		if (process_rt_periodic(pRT1, RT_STACK, &t_5sec, &t_7sec, &t_8sec) < 0) { return -1; }
    /* Launch concurrent execution */
	process_start();

  LED_Off();
  while(process_deadline_miss<1) {
		LEDGreen_On();
		shortDelay();
		LED_Off();
		shortDelay();
	}
	
	//infinite loop
	while (1);
	return 0;
}