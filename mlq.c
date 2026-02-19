#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pcb.h"

typedef struct SchedulerMetrics {
    double total_turnaround;
    double total_waiting;
    double total_response;
    int completed_jobs;
    int total_jobs;
} SchedulerMetrics;

static void enqueue_tail(PcbPtr *queue, PcbPtr process)
{
    if (!process) {
        return;
    }
    process->next = NULL;
    if (*queue) {
        PcbPtr tail = *queue;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = process;
    } else {
        *queue = process;
    }
}

static void enqueue_front(PcbPtr *queue, PcbPtr process)
{
    if (!process) {
        return;
    }
    process->next = *queue;
    *queue = process;
}

static void append_queue(PcbPtr *dest, PcbPtr *src)
{
    if (!dest || !src || !*src) {
        return;
    }

    if (!*dest) {
        *dest = *src;
    } else {
        PcbPtr tail = *dest;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = *src;
    }
    *src = NULL;
}

static void upgrade_to_level0(PcbPtr *level0, PcbPtr *queue, int timer)
{
    PcbPtr current = *queue;
    while (current) {
        current->priority = 0;
        current->time_in_current_quantum = 0;
        current->time_at_current_level = 0;
        current->last_ready_queue_entry_time = timer;
        current->last_enqueued_time = timer;
        current = current->next;
    }
    append_queue(level0, queue);
}

static void start_running_process(PcbPtr process, int timer)
{
    if (!process) {
        return;
    }

    process->status = PCB_READY;
    process->total_wait_time += timer - process->last_ready_queue_entry_time;
    if (process->start_time < 0) {
        process->start_time = timer;
        process->response_time = timer - process->arrival_time;
    }
    startPcb(process);
}

static void handle_completion(PcbPtr *current_process, SchedulerMetrics *metrics, int timer)
{
    terminatePcb(*current_process);
    (*current_process)->completion_time = timer;
    metrics->total_turnaround += (*current_process)->completion_time - (*current_process)->arrival_time;
    metrics->total_waiting += (*current_process)->total_wait_time;
    metrics->total_response += (*current_process)->response_time;
    metrics->completed_jobs++;
    free(*current_process);
    *current_process = NULL;
}

static void demote_to_level(PcbPtr *current_process, PcbPtr *target_queue, int target_priority, int timer)
{
    if (!suspendPcb(*current_process)) {
        fprintf(stderr, "ERROR: Failed to suspend process %d\n", (*current_process)->pid);
        exit(EXIT_FAILURE);
    }
    (*current_process)->priority = target_priority;
    (*current_process)->status = PCB_READY;
    (*current_process)->time_in_current_quantum = 0;
    (*current_process)->time_at_current_level = 0;
    (*current_process)->last_ready_queue_entry_time = timer;
    (*current_process)->last_enqueued_time = timer;
    enqueue_tail(target_queue, *current_process);
    *current_process = NULL;
}

static void preempt_to_front(PcbPtr *current_process, PcbPtr *queue, int timer)
{
    if (!suspendPcb(*current_process)) {
        fprintf(stderr, "ERROR: Failed to suspend process %d\n", (*current_process)->pid);
        exit(EXIT_FAILURE);
    }
    (*current_process)->status = PCB_READY;
    if ((*current_process)->priority != 2) {
        (*current_process)->time_in_current_quantum = 0;
    }
    (*current_process)->last_ready_queue_entry_time = timer;
    (*current_process)->next = NULL;
    enqueue_front(queue, *current_process);
    *current_process = NULL;
}

static PcbPtr select_next_process(PcbPtr *level0, PcbPtr *level1, PcbPtr *level2)
{
    if (*level0) {
        return deqPcb(level0);
    }
    if (*level1) {
        return deqPcb(level1);
    }
    if (*level2) {
        return deqPcb(level2);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int t0 = 0, t1 = 0, t2 = 0, W = 0;
    FILE *input_stream = NULL;
    PcbPtr dispatch_queue = NULL;
    PcbPtr level0_queue = NULL;
    PcbPtr level1_queue = NULL;
    PcbPtr level2_queue = NULL;
    PcbPtr current_process = NULL;
    SchedulerMetrics metrics = {0};
    int timer = 0;
    int job_id_counter = 0;

    if (argc <= 0) {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    } else if (argc != 2) {
        fprintf(stderr, "Usage: %s <JOB_DISPATCH_FILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Enter time quantum for Level-0 (t0): \n");
    if (scanf("%d", &t0) != 1 || t0 < 0) {
        fprintf(stderr, "ERROR: Invalid value for t0\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Enter time quantum for Level-1 (t1): \n");
    if (scanf("%d", &t1) != 1 || t1 < 0) {
        fprintf(stderr, "ERROR: Invalid value for t1\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Enter time quantum for Level-2 (t2): \n");
    if (scanf("%d", &t2) != 1 || t2 <= 0) {
        fprintf(stderr, "ERROR: Invalid value for t2\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Enter starvation threshold (W): \n");
    if (scanf("%d", &W) != 1 || W < 0) {
        fprintf(stderr, "ERROR: Invalid value for W\n");
        exit(EXIT_FAILURE);
    }

    if (!(input_stream = fopen(argv[1], "r"))) {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while (!feof(input_stream)) {
        int arrival = 0;
        int service = 0;
        int priority = 0;
        PcbPtr process = createnullPcb();

        if (!process) {
            fclose(input_stream);
            exit(EXIT_FAILURE);
        }

        if (fscanf(input_stream, "%d, %d, %d", &arrival, &service, &priority) != 3) {
            free(process);
            continue;
        }

        if (priority < 0) {
            priority = 0;
        } else if (priority > 2) {
            priority = 2;
        }

        process->arrival_time = arrival;
        process->service_time = service;
        process->remaining_cpu_time = service;
        process->initial_priority = priority;
        process->priority = priority;
        process->status = PCB_INITIALIZED;
        process->job_id = ++job_id_counter;

        dispatch_queue = enqPcb(dispatch_queue, process);
    }
    fclose(input_stream);

    metrics.total_jobs = job_id_counter;
    if (metrics.total_jobs == 0) {
        fprintf(stderr, "No jobs to schedule.\n");
        exit(EXIT_FAILURE);
    }

    while (current_process || dispatch_queue || level0_queue || level1_queue || level2_queue) {
        while (dispatch_queue && dispatch_queue->arrival_time <= timer) {
            PcbPtr arrived = deqPcb(&dispatch_queue);
            arrived->priority = arrived->initial_priority;
            arrived->time_in_current_quantum = 0;
            arrived->time_at_current_level = 0;
            arrived->last_ready_queue_entry_time = timer;
            arrived->last_enqueued_time = timer;
            arrived->status = PCB_READY;

            if (arrived->priority == 0) {
                enqueue_tail(&level0_queue, arrived);
            } else if (arrived->priority == 1) {
                enqueue_tail(&level1_queue, arrived);
            } else {
                enqueue_tail(&level2_queue, arrived);
            }
        }

        if (level1_queue) {
            int wait_time = timer - level1_queue->last_enqueued_time;
            if (wait_time >= W) {
                upgrade_to_level0(&level0_queue, &level1_queue, timer);
                upgrade_to_level0(&level0_queue, &level2_queue, timer);
            }
        }

        if (level2_queue) {
            int wait_time = timer - level2_queue->last_enqueued_time;
            if (wait_time >= W) {
                upgrade_to_level0(&level0_queue, &level2_queue, timer);
            }
        }

        if (current_process) {
            if (current_process->priority == 1 && level0_queue) {
                preempt_to_front(&current_process, &level1_queue, timer);
            } else if (current_process->priority == 2 && (level0_queue || level1_queue)) {
                preempt_to_front(&current_process, &level2_queue, timer);
            }
        }

        if (!current_process) {
            current_process = select_next_process(&level0_queue, &level1_queue, &level2_queue);
            if (current_process) {
                if (current_process->priority != 1) {
                    current_process->time_at_current_level = 0;
                }
                start_running_process(current_process, timer);
            }
        }

        if (!current_process) {
            if (!(dispatch_queue || level0_queue || level1_queue || level2_queue)) {
                break;
            }
            sleep(1);
            timer++;
            continue;
        }

        sleep(1);
        timer++;

        current_process->remaining_cpu_time--;
        current_process->time_in_current_quantum++;
        if (current_process->priority == 1) {
            current_process->time_at_current_level++;
        }

        if (current_process->remaining_cpu_time <= 0) {
            handle_completion(&current_process, &metrics, timer);
            continue;
        }

        if (current_process && current_process->priority == 0 && t0 > 0 && current_process->time_in_current_quantum >= t0) {
            current_process->priority = 1;
            demote_to_level(&current_process, &level1_queue, 1, timer);
            continue;
        }

        if (current_process && current_process->priority == 1) {
            if (t1 > 0 && current_process->time_at_current_level >= t1) {
                current_process->priority = 2;
                demote_to_level(&current_process, &level2_queue, 2, timer);
                continue;
            }
        }

        if (current_process && current_process->priority == 2) {
            if (t2 > 0 && current_process->time_in_current_quantum >= t2) {
                demote_to_level(&current_process, &level2_queue, 2, timer);
                continue;
            }
        }
    }

    if (metrics.completed_jobs > 0) {
        printf("Average turnaround time: %.2f\n", metrics.total_turnaround / metrics.completed_jobs);
        printf("Average waiting time: %.2f\n", metrics.total_waiting / metrics.completed_jobs);
        printf("Average response time: %.2f\n", metrics.total_response / metrics.completed_jobs);
    }

    return EXIT_SUCCESS;
}

