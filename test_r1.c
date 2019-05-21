#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"

/** Test Case 1- Two real time processes with all having same arrival time and deadline*/
/** We expect that there would be 15 flashes of red and 15 flashes of blue (one red flash after one blue flash) corresponding 
to prt1 and prt2. Due to high number of flashes, both the processes miss their deadline, hence, we
see a green led flash in the end, which is exoected when there are two misses.*/





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
realtime_t t_2sec = {2, 0};
realtime_t t_10sec = {10, 0};
realtime_t t_15sec = {15, 0};

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
 /**not needed for this test cases Ask=> to include or not*/
void pNRT(void) {
	int i;
	for (i=0; i<17;i++){
	LEDGreen_On();
	shortDelay();
	LEDGreen_Toggle();
	shortDelay();
	}
	
}


/*-------------------
 * Real-time process
 *-------------------*/

void pRT1(void) {
	int i;
	for (i=0; i<15;i++){
	LEDRed_Toggle();
	shortDelay();
		
	LEDRed_Toggle();
	shortDelay();
	
	}
}
void pRT2(void) {
	int i;
	for (i=0; i<15;i++){
	LEDBlue_Toggle();
	shortDelay();
		
	LEDBlue_Toggle();
	shortDelay();
		
	}
}






/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
		
		if (process_rt_create(pRT1, RT_STACK, &t_2sec, &t_15sec) < 0) { return -1; } 
    if (process_rt_create(pRT2, RT_STACK, &t_2sec, &t_15sec) < 0) { return -1; } 
   
  
   
    /* Launch concurrent execution */
	process_start();

  LED_Off();
  while(process_deadline_miss==2) {
		LEDGreen_On();//
		shortDelay();
		LED_Off();
		shortDelay();
		process_deadline_miss=2-process_deadline_miss;
	}
	
	/* Hang out in infinite loop (so we can inspect variables if we want) */ 
	while (1);
	return 0;
}