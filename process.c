#include "3140_concur.h"

#include <stdlib.h>

#include <fsl_device_registers.h>

#include "realtime.h"


// NetID: it233, mr2265

realtime_t current_time;

int process_deadline_met;

int process_deadline_miss;

struct process_state {
  unsigned int * sp; //the stack pointer for the process
  struct process_state * next; // the pointer to the next process
  unsigned int * original_sp; // the original stac pointer for the process
  unsigned int original_pc; // the original pc for the process
  int n; // the space occupied on the stack by the process
  realtime_t arr_time; // arrival time for the process
  realtime_t deadline; // deadline for the process (initially relative to the start)
  realtime_t period; // the period for the process
  int is_periodic; //1=periodic, 0 = not periodic
  int is_real_time; //1 = is realtime. 0 = not realtime
};

/** 
    

*/

process_t * current_process = NULL; // currently running process. current_process will be NULL if no process runs, else must point to the process_t of the currently running process
process_t * process_queue = NULL; // the head of the process_quueue for the non real time processes. NULL if no non real time process in the queue
process_t * process_tail = NULL; // the tail of the process queue for non real time processes

/** initalize queue for ready and nonreafy realtame process*/

process_t * ready_realtime = NULL; // the head of the queue for the ready realtime processes
process_t * not_ready_realtime = NULL; // the head of the queue for the non-ready real time processes (those processes for which the arrival time is greater than the current time)

/** dequeue front of process from queue for non-realtime processes and return it*/

static process_t * pop_front_process() {
  if (!process_queue) return NULL; //if the queue is empty, return NULL
  process_t * proc = process_queue; //create a pointer to point to the head of the queue
  process_queue = proc -> next; //  set the head of the queue to point to the next process in the queue
  if (process_tail == process_queue) { // if the next process in the queue is the tail of the queue
    process_tail = process_queue = NULL;
  } // set the global process_tail pointer to point to null as the queue will contain only one process after the dequeue
  else proc -> next = NULL; // set the next process for the process we are dequeueing to null as it is being removed from the queue
  return proc; //return the head of the queue (process that we are dequeueing)
}

// Dequeue front from ready realtime process queue and return front.

static process_t * pop_front_rt() {
  if (!ready_realtime) return NULL; //if the global pointer for the ready_realtime process is NULL, the queue is empty so you return null
  process_t * proc = ready_realtime; // create a process_t pointer and set it to point to the process at the head of the queue
  ready_realtime = proc -> next; // set the global pointer for the ready_realtime queue to point to the next process in the queue
  proc -> next = NULL; // set the next variable to point to null as the process at the head of the queue is about to be dequeued
  return proc;

}

//Dequeue front from non-ready realtime processes and return front.

static process_t * pop_front_not_ready_rt() {
  if (!not_ready_realtime) return NULL; //if the global pointer for the not ready realtime process is NULL, the queue is empty so you return null
  process_t * proc = not_ready_realtime; // // create a process_t pointer and set it to point to the process at the head of the queue
  not_ready_realtime = proc -> next; // // set the global pointer for the ready_realtime queue to point to the next process in the queue
  proc -> next = NULL; // set the next variable to point to null as the process at the head of the queue is about to be dequeued
  return proc;

}

// Free stack for processes.

static void process_free(process_t * proc) {
  process_stack_free(proc -> original_sp, proc -> n);
  free(proc);

}

// Add process to non-realtime process queue .

static void push_tail_process(process_t * proc) {
  if (!process_queue) {
    process_queue = proc;
  }
  if (process_tail) {
    process_tail -> next = proc;
  }
  process_tail = proc;
  proc -> next = NULL;
}

// Add to ready realtime process queue, which is kept sorted based on the deadline to implement EDD

static void push_rt_ready(process_t * proc) {
  if (!ready_realtime) { //if the ready_realtime global pointer is null
    ready_realtime = proc; // set the global  pointer ready_realtime to point to the current process
    ready_realtime -> next = NULL; // as there is only one process in the queue, set the queue's next variable to point to null
  } else { //the queue has at least one process in it
    unsigned int dl = proc -> deadline.sec * 1000 + proc -> deadline.msec; //initialize variable to store te current process's deadline
    process_t * nxt = ready_realtime; // initialize nxt and curr to traverse the queue
    process_t * curr = NULL;
    while (nxt && (dl >= nxt -> deadline.sec * 1000 + nxt -> deadline.msec)) {
      curr = nxt;
      nxt = nxt -> next; //iterate through rt ready queue until it finds a current_deadline<proc_deadline<next_deadline
    } //nxt points to process with next largest deadline
    proc -> next = nxt; // set the next pointer of the process to be added to point to the process in the queue that has the smallest deadline greater than the process's deadline
    if (curr) {
      curr -> next = proc;
    } // Set the next pointer of the process with the largest deadline smaller than the process to be added to point to the current process
    else { // in the case of an empty queue, the arriving process is set as the head of the queue.
			process_t *tmp = ready_realtime; // the current process has a deadline earlier than the earliest deadline in the queue
			ready_realtime = proc; // set the head of the queue to the new process
			ready_realtime -> next = tmp; // set the next process to the previous head of the queue
    }
  }

}

// Add to not ready realtime process queue.

static void push_rt_Nready(process_t * proc) {
  /*Nondecreasing order. Checks arrival_time of each node with proc*/
  if (!not_ready_realtime) { //if the not_ready_realtime global pointer is null so the queue is empty
    not_ready_realtime = proc; // set the global  pointer not_ready_realtime to point to the current process
    not_ready_realtime -> next = NULL; // as there is only one process in the queue, set the head of the queue's next variable to point to null
  } 
	else {
    unsigned int st = proc -> arr_time.sec * 1000 + proc -> arr_time.msec; // compute the arrival time for the process
    process_t * nxt = not_ready_realtime; // set the nxt pointer to point to the current head of the qeue
    process_t * curr = NULL; // set the curr pointer to point to null
    while (nxt && (st >= nxt -> arr_time.sec * 1000 + nxt -> arr_time.msec)) { // iterate through the queue unti you either reach the end of the queue or you find a process whose arrival time is smaller than the start time for this process
      curr = nxt;
      nxt = nxt -> next;
    } //nxt points to process with next largest arrival time

    proc -> next = nxt; // set the next pointer of the process to be added to point to the process in the queue that has the smallest arrival time greater than the process's deadline

    if (curr) {
      curr -> next = proc;
    } // Set the next pointer of the process with the largest start time smaller than the process to be added to point to the current process
    else {
      process_t *tmp = not_ready_realtime; // the current process has an arrival time earlier than the earliest arrival time in the queue
			not_ready_realtime = proc; // set the head of the queue to the new process
			not_ready_realtime -> next = tmp; // set the next process to the previous head of the queue
    }
  }

}

void process_stack_reinit(process_t * proc) {
  unsigned int * sp;
  sp = proc -> original_sp;
  sp[0] = 0x3; // enable scheduling and timer interrupt
  sp[9] = 0xFFFFFFF9; // EXC_RETURN value, return to thread mode
  sp[15] = (unsigned int) process_terminated; // LR
  sp[16] = proc -> original_pc; //PC
  sp[17] = 0x01000000; // xPSR
  proc -> sp = proc -> original_sp;
}


unsigned int * process_select(unsigned int * cursp) {
  unsigned int curT = current_time.msec + current_time.sec * 1000; //find the current time in msec
	if (not_ready_realtime) { // if there are not ready realtime processees
  while ((not_ready_realtime -> arr_time.msec + not_ready_realtime -> arr_time.sec * 1000 <= curT)) { // if the arrival time of the process at the head of the queue is smaller than or equal to the current time
    process_t * node = pop_front_not_ready_rt(); // pop the process at the head of the not ready real time process queue
    push_rt_ready(node); //push that process onto the real time process queue where its position in the queue will be determined on the basis of the deadlines
  }
}
  if (cursp) { // Suspending a process which has not yet finished, save state and make it the tail
    current_process -> sp = cursp; // have the currently executing current_process's stack pointer point to cursp
    if (current_process -> is_real_time) { //if the process is a realtime process, push it onto the realtime process queue
      push_rt_ready(current_process);
    } else {
      push_tail_process(current_process); // if the process is not a realtimme process, push it onto the queue for the regular processes
    }
  }
  // Not popping off a process before you look
  else {
    // Check if a process was running, free its resources if one just finished
    if (current_process) { // If the current stack pointer is null, the process finished execution                                           
      if (current_process -> is_real_time) { // in case of real time processes
        unsigned int rt_deadline = current_process -> deadline.sec * 1000 + current_process -> deadline.msec; // compute the deadline for that process
        if (curT > rt_deadline) { // Process has completed so check if deadline is missed or not and increment the correct global variables properly
          process_deadline_miss++;
        } else {
          process_deadline_met++;
        }
      }
			
			

      if (current_process -> is_periodic) { // if the process is periodic
				process_stack_reinit(current_process);
        current_process -> arr_time.msec += current_process -> period.msec; // setting the new arrival time to be the previous arrival time + the period
        current_process -> arr_time.sec += current_process -> period.sec;
        while (current_process -> arr_time.msec >= 1000) { //ensures milsec. less than 1000 by adding 1 to sec
          current_process -> arr_time.sec++;
          current_process -> arr_time.msec -= 1000;
        }
        current_process -> deadline.msec = current_process -> period.msec + current_process -> deadline.msec; //setting the new deadline to be the previous deadline + the period
        current_process -> deadline.sec = current_process -> period.sec + current_process -> arr_time.sec;
        while (current_process -> deadline.msec >= 1000) { //ensures milliseconds is less than 1000 by adding 1 to sec
          current_process -> deadline.sec++;
          current_process -> deadline.msec -= 1000;
        }
				if (current_process -> arr_time.sec * 1000 + current_process -> arr_time.msec <= curT){ // if the new arrival time is still less than the current time, add the process to the ready realtime process quueue
					push_rt_ready(current_process);
				}
        push_rt_Nready(current_process); // push the process onto the not ready realtime process queue if its arrival time is not less than the current time.
      } else {
        process_free(current_process); // the current stack pointer is null so the process has terminated. Call process_free
      }
    }
  }
  //Fill up current process

  if (ready_realtime) { // if a realtime process is waiting in the queue, remove it from the head of the queue. Note we checked above if the process with the lowest arrival time is ready to go
    current_process = pop_front_rt(); // set the current process pointer to point be the next real time process. This has the lowest absolute deadline.
  } 
	else if (process_queue){ // if there are no ready real time processes, then check if any regular processes are waiting.
	current_process = pop_front_process ();	
	}
	else if (not_ready_realtime) {		// if there are waiting realtime processes
		if ((not_ready_realtime -> arr_time.msec + not_ready_realtime -> arr_time.sec * 1000 > curT)){
		__enable_irq(); //enable all interrupts.
		while ((not_ready_realtime -> arr_time.msec + not_ready_realtime -> arr_time.sec * 1000 > current_time.msec + current_time.sec * 1000)) { // busy wait so that the realtime processes arrival time comes
		}
		__disable_irq(); // disable interrupts
		} 
		//if the not ready realtime process with the smallest arrival time has an arrival time less than the current time
      current_process = pop_front_not_ready_rt(); //remove the not ready real time process at the head of the queue as its arrival time is <= current time. The ready realtime process queue is empty and we had to do a busy wait
// thus the process_queue was also empty. This is the only process that has to run		
  } 
	else {
		current_process = NULL; //there are no more waiting processes
	}
  if (current_process) {
    // Launch the process which was just popped from  queue
    return current_process -> sp;
  } else {
    // No process has been selected, so exit the scheduler
    return NULL;
  }
}

/* Start up  concurrent execution */

void process_start(void) {
  SIM -> SCGC6 |= SIM_SCGC6_PIT_MASK;
  PIT -> MCR = 0;
  PIT -> CHANNEL[0].LDVAL = DEFAULT_SYSTEM_CLOCK / 10;
  PIT -> CHANNEL[1].LDVAL = DEFAULT_SYSTEM_CLOCK / 1000;
  // Set interrupt priority
  NVIC_SetPriority(SVCall_IRQn, 1);
  NVIC_SetPriority(PIT0_IRQn, 1); // set lower priorities for PIT0 timer and for SVCall
  NVIC_SetPriority(PIT1_IRQn, 0); //Highest interupt priority for the 1 microsecond timer
  // Enabling interrupts

  PIT -> CHANNEL[1].TCTRL |= 3; // set to 3 to allow interrupts and enable timers

	
  NVIC_EnableIRQ(PIT0_IRQn); // Enable interrupts for PIT 0

  NVIC_EnableIRQ(PIT1_IRQn); // Enable interrupts for PIT 1

  // Timer not enabled as scheduler does itt

	// if no processes in either queue, do nothing
	if (!process_queue && !not_ready_realtime) return;
  process_begin();

}

/* Create a new process */

int process_create(void( * f)(void), int n) {

  unsigned int * sp = process_stack_init(f, n);

  if (!sp) return -1;

  process_t * proc = (process_t * ) malloc(sizeof(process_t));

  if (!proc) {

    process_stack_free(sp, n);

    return -1;

  }
  proc -> sp = proc -> original_sp = sp;
  proc -> original_pc = (unsigned int) f; // assigning this but this PC needs to be preserved only for your periodic processes for resetting
  proc -> n = n;
  proc -> is_real_time = 0; // not a real time process
  proc -> is_periodic = 0; // not a periodic process
  proc -> arr_time.sec = NULL; // no arrival time
  proc -> arr_time.msec = NULL;
  proc -> deadline.sec = NULL;// no deadline
  proc -> deadline.msec = NULL;
  proc -> period.msec = NULL; // no period 
  proc -> period.sec = NULL; // no period

  push_tail_process(proc); // push onto the process queue for non real time processes

  return 0;

}

// Realtime Process Create

int process_rt_create(void( * f)(void), int n, realtime_t * start, realtime_t * deadline) {

  unsigned int * sp = process_stack_init(f, n);

  if (!sp) return -1;

  process_t * proc = (process_t * ) malloc(sizeof(process_t));

  if (!proc) {

    process_stack_free(sp, n);

    return -1;

  }

  proc -> sp = proc -> original_sp = sp; // save original sp

  proc -> original_pc = (unsigned int) f; // save original PC. Only really needed for periodic processes

  proc -> n = n;

  proc -> is_real_time = 1; //checking that its real time

  proc -> is_periodic = 0; // set to 0 as non periodic process
  proc -> period.msec = NULL; // no period so stored NULL
  proc -> period.sec = NULL; // no period so store NULL

  proc -> arr_time = * start;

  proc -> deadline.msec = start -> msec + deadline -> msec; // absolute deadline is stored to order non periodic process properly in the not ready realtime queue

  proc -> deadline.sec = start -> sec + deadline -> sec; // absolute deadline is stored to order non periodic process

  while (proc -> deadline.msec >= 1000) { //ensures millisec lesss than 1000
    proc -> deadline.sec++;
    proc -> deadline.msec -= 1000;
  }
  push_rt_Nready(proc); // push the initialized process onto the not ready queue for initialized processes from where it can be dequeued according to arrival time.
  return 0;

}

//Extra creddit;- process
int process_rt_periodic(void( * f)(void), int n, realtime_t * start, realtime_t * deadline, realtime_t * period) {
  unsigned int * sp = process_stack_init(f, n);
  if (!sp) return -1;

  process_t * proc = (process_t * ) malloc(sizeof(process_t));
  if (!proc) {
    process_stack_free(sp, n);
    return -1;
  }
  proc -> original_pc = (unsigned int) f; // saving the original pc for the process
  proc -> sp = proc -> original_sp = sp;

  proc -> n = n;
  proc -> is_periodic = 1; //to check if periodic
  proc -> is_real_time = 1; //to check that it is real time
  proc -> period = * period;
  proc -> arr_time = * start; //copy start time in arr_time

  proc -> deadline.sec = start -> sec + deadline -> sec; // setting the absolute deadline for the periodic process for EDD purposes
  proc -> deadline.msec = start -> msec + deadline -> msec;

  while (proc -> deadline.msec >= 1000) { // ensure msec is less than 1000
    proc -> deadline.sec++;
    proc -> deadline.msec -= 1000;
  }
  push_rt_Nready(proc); // push on to the not ready real time process queue
}



void PIT1_IRQHandler(void) {

  current_time.msec++;

  if (current_time.msec >= 1000) { // if current time in msec is greater than or equal to 1000

    current_time.sec++; // increment seconds

    current_time.msec = 0; // set the msec to 0

  }

  PIT -> CHANNEL[1].TFLG = PIT_TFLG_TIF(1); //Reset timer flag             

  NVIC_ClearPendingIRQ(PIT1_IRQn); // Clear the interrupt

}