Multi-Level Queue Scheduler

This project implements a multi-level queue (MLQ) CPU scheduler that simulates process execution using priority queues, quantum limits, and starvation prevention.

The scheduler reads jobs from a file and executes them according to priority rules, queue transitions, and timing constraints.

Overview

Each job in the input file is formatted as:

<arrival_time>, <cpu_time>, <priority (0|1|2)>


At runtime, the scheduler prompts for:

t0 — quantum for Level-0

t1 — maximum CPU time allowed at Level-1 before demotion

t2 — quantum for Level-2 round-robin

W — starvation threshold

Ready Queues

Level-0 (L0Q) — highest priority

Level-1 (L1Q) — medium priority

Level-2 (L2Q) — lowest priority

Jobs move between queues based on quantum expiration, accumulated runtime, and starvation rules.

Scheduler Design
Data Structures

dispatchQ — jobs sorted by arrival time

L0Q, L1Q, L2Q — ready queues

current — currently running job

timer — simulated clock

Each job tracks:

arrival, service, and remaining CPU time

current and initial priority

time used in current quantum

accumulated runtime at Level-1

timestamps for waiting and starvation checks

start, completion, response, and waiting times

Global metrics track turnaround, waiting, and response times.

Scheduling Logic
Job Arrival

At each tick:

newly arrived jobs are moved into their priority queue

queue timing fields are reset

Starvation Prevention

If a job waits longer than W:

waiting jobs are promoted to Level-0

timers are reset to prevent repeated promotions

Preemption Rules

Higher-priority jobs interrupt lower-priority execution:

Level-1 job is preempted if Level-0 becomes non-empty

Level-2 job is preempted if Level-0 or Level-1 becomes non-empty

Preempted jobs return to the head of their queue.

Dispatching

When CPU is idle:

select from L0Q

otherwise L1Q

otherwise L2Q

Waiting time is updated when execution begins.

Execution Cycle

Each tick:

decrement remaining CPU time

increment quantum counters

update Level-1 accumulated time

Completion

When a job finishes:

turnaround, waiting, and response times are computed

global metrics are updated

Quantum Expiry & Demotion

Level-0

FCFS with quantum t0

if expired → demote to Level-1

Level-1

runs only when L0Q is empty

if accumulated time ≥ t1 → demote to Level-2

Level-2

round robin using quantum t2

rotates within L2 queue

Performance Metrics

After all jobs complete:

Turnaround time = completion − arrival

Waiting time = turnaround − service

Response time = first run − arrival

The program outputs average values for all jobs.

Testing Strategy

The scheduler is validated using small workloads to verify each rule independently.

Core Scenarios

priority ordering & FCFS within levels

Level-0 arrival preemption

demotion when quantum expires

Level-1 accumulated time demotion

Level-2 round-robin fairness

starvation prevention

metric verification against manual calculations

Edge Cases

t0 = 0 or t1 = 0 → immediate demotion

very small t2 → frequent RR rotations

simultaneous arrivals → strict FCFS per level

invalid priorities → clamped to range 0–2

starvation promotions correctly reset timers

Parameter Optimization

To balance turnaround and response time, multiple configurations were tested.

Recommended Configuration
t0 = 1
t1 = 5
t2 = 6
W  = 100


Results

Average Turnaround ≈ 30.00

Average Response ≈ 0.47

This configuration provides near-minimum turnaround with excellent responsiveness.

Why These Values Work

t0 (Level-0 quantum)
A small value minimizes response time and benefits short jobs.

t1 (Level-1 budget)
A moderate value prevents premature demotion and excess context switching.

t2 (Level-2 quantum)
A slightly larger slice reduces overhead for long jobs.

W (starvation threshold)
A large value prevents unnecessary promotions for this workload.

Comparison
Setting	t0	t1	t2	Avg Turnaround	Avg Response
Recommended	1	5	6	30.00	0.47
Min Turnaround	6	6	1	29.53	4.79

The minimum turnaround configuration significantly worsens response time, making the recommended configuration the better overall choice.

Potential Implementation Issues
Signal & Process Control

Some environments restrict signals like SIGTSTP and SIGCONT.

Mitigation: run outside sandbox or provide simulation mode.

Tick Granularity

Scheduling occurs at 1-second intervals.

Mitigation: use a simulated clock if finer resolution is required.

Starvation Timer Accuracy

Timers must reset only when a job is placed at the tail.

Level-1 Accumulation

Runtime must accumulate across preemptions.

FCFS Queue Integrity

Correct head/tail placement preserves ordering.

Build & Run
Build
make

Run
./random jobs.txt
./mlq jobs.txt


Enter values for t0, t1, t2, and W when prompted.