#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"


/** Periodic test case*/
/** Test Case 2- In this test case one periodic real time process is executed and runs forever.
Hence, there are only blue flashes running forever. Also, we would never see a green led flash as
the process runs forever (the green LED never flashes ).*/

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
realtime_t t_15sec = {15, 0};
realtime_t t_1sec = {1, 0};
realtime_t t_2sec = {2, 0};
realtime_t t_5sec = {5, 0};



/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}

/*-------------------
 * Non Real-time process
 *-------------------*/

 void pNRT(void) {
	int i;
	for (i=0; i<6;i++){
	LEDRed_On();
	shortDelay();
	LEDRed_Toggle();
	shortDelay();
	}
	
}


/*-------------------
 * Real-time process
 *-------------------*/

void pRT2(void) {
	int i;
	for (i=0; i<7;i++){
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
	 if (process_create(pNRT, NRT_STACK) < 0) { return -1; }
	 if (process_rt_periodic(pRT2, RT_STACK, &t_5sec, &t_15sec, &t_5sec) < 0) { return -1; } 
		
    /* Launch concurrent execution */
	process_start();

  LED_Off();
  
		LEDGreen_On();
		shortDelay();
		LED_Off();
		shortDelay();
		

	
	//infinite loop
	while (1);
	return 0;
}