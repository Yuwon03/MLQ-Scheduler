/* PCB include header file for FCFS dispatcher */

#ifndef FCFS_PCB
#define FCFS_PCB

/* Include files */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* Process Management Definitions *****************************/
#define PCB_UNINITIALIZED 0
#define PCB_INITIALIZED 1
#define PCB_READY 2
#define PCB_RUNNING 3
#define PCB_SUSPENDED 4
#define PCB_TERMINATED 5

/* Custom Data Types */
struct pcb {
    pid_t pid;
    char * args[3];
    int arrival_time;
    int service_time;
    int remaining_cpu_time;
    int initial_priority;
    int priority;
    int time_in_current_quantum;
    int time_at_current_level;
    int total_wait_time;
    int start_time;
    int completion_time;
    int response_time;
    int last_ready_queue_entry_time;
    int last_enqueued_time;
    int job_id;
    int status;
    struct pcb * next;
};

typedef struct pcb Pcb;
typedef Pcb * PcbPtr;

/* Function Prototypes */
PcbPtr startPcb(PcbPtr);
PcbPtr suspendPcb(PcbPtr);
PcbPtr terminatePcb(PcbPtr);
PcbPtr printPcb(PcbPtr);
void   printPcbHdr(void);
PcbPtr createnullPcb(void);
PcbPtr enqPcb(PcbPtr, PcbPtr);
PcbPtr deqPcb(PcbPtr*);

#endif
