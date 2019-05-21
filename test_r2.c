#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"

/** Test Case 2- Two real time processes (prt1 and prt2) with one 
having the arrival time before the other process and absolute deadline after it.
prt1 has the earlier arrival time but the later absolute deadline.*/
/**Expected output:- Red flashes corresponding to PRT1 and then blue flashes (for PRT2)
followed by the remaining red flashes for prt1, with a green flash signalling end of the process. This is because when prt1 runs prt2 is
executed and pre-empts prt1. In this case both the processes meet their deadline, hence a
green flash is displayed in the end corresponding to process_deadline_met == 2.*/

/*--------------------------*/
/* Parameters for test case */
/*--------------------------*/


 
/* Stack space for processes */
#define NRT_STACK 20
#define RT_STACK  20
 


/*--------------------------------------*/
/* Time structs for real-time processes */
/*--------------------------------------*/

/* Constants used for 'work' and 'deadline's */

realtime_t t_2msec = {0, 2};
realtime_t t_4msec = {0, 4};
realtime_t t_5sec = {5, 0};
realtime_t t_10sec = {10, 0};
realtime_t t_15sec = {15, 0};
realtime_t t_25sec = {25, 0};
/* Process start time */
realtime_t t_pRT1 = {1, 0};

 
/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}



/*----------------------------------------------------
 * Non real-time process
 *   Blinks red LED 10 times. 
 *   Should be blocked by real-time process at first.
 *----------------------------------------------------*/
 
void pNRT(void) {
	int i;
	for (i=0; i<17;i++){
	LED_Off();
	LEDRed_Toggle();
	shortDelay();
	LED_Off();
	shortDelay();
	}
	
}

/*-------------------
 * Real-time process
 *-------------------*/

void pRT1(void) {
	int i;
	for (i=0; i<7;i++){
	LEDRed_On();
	mediumDelay();
	LEDRed_Toggle();
	mediumDelay();
	}
}

void pRT2(void) {
	int i;
	for (i=0; i<7;i++){
	LEDBlue_On();
	mediumDelay();
	LEDBlue_Toggle();
	mediumDelay();
	}
}




/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
    
    if (process_rt_create(pRT1, RT_STACK, &t_5sec, &t_25sec) < 0) { return -1; } 
		if (process_rt_create(pRT2, RT_STACK, &t_10sec, &t_15sec) < 0) { return -1; } 
   
    /* Launch concurrent execution */
	process_start();

  LED_Off();
  while(process_deadline_met==2) {
		LEDGreen_On();
		
		
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_met=process_deadline_met-2;
	}
	
	/* Hang out in infinite loop (so we can inspect variables if we want) */ 
	while (1);
	return 0;
}