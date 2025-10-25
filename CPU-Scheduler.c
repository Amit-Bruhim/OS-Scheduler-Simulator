#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#define _XOPEN_SOURCE 700

/* Global: number of processes read from CSV */
int PROCESSES_AMOUNT;

/* Reusable print buffer used for formatted output */
char buffer[512];

/* Counter used by Round Robin to track completed children */
int done_processes2 = 0;

/* Process structure matching CSV fields and runtime state */
typedef struct
{
    char Name[51];
    char Description[101];
    int Arrival_Time;
    int Burst_Time;
    int Priority;
    int compeletion_time;
    int remainning_time;
    int is_done;
} process;

/* Array that will hold pointers to allocated process structs */
process *processes[1000];

void log_event(int start_time, int end_time, process *p, int is_idle);

/* ---------------------------------------------------------------------
   parse
   Read PROCESSES_AMOUNT lines from CSV file and populate processes array.
   CSV format (no header): Name,Description,Arrival,Burst,Priority
   --------------------------------------------------------------------- */
void parse(char *processesCsvFilePath)
{
    /* open file for reading */
    FILE *file = fopen(processesCsvFilePath, "r");

    /* iterate expected number of lines and parse fields */
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        /* allocate a process struct for this entry */
        processes[i] = malloc(sizeof(process));

        /* read one line from file and remove trailing newline */
        char line[256];
        fgets(line, sizeof(line), file);
        int index = strcspn(line, "\n");
        if (index < strlen(line))
            line[index] = 0;

        /* tokenize CSV fields and copy into struct fields */
        char *token = strtok(line, ",");
        strncpy(processes[i]->Name, token, 50);

        token = strtok(NULL, ",");
        strncpy(processes[i]->Description, token, 100);

        token = strtok(NULL, ",");
        processes[i]->Arrival_Time = atoi(token);

        token = strtok(NULL, ",");
        processes[i]->Burst_Time = atoi(token);

        token = strtok(NULL, ",");
        processes[i]->Priority = atoi(token);
    }

    /* close file */
    fclose(file);
}

/* ---------------------------------------------------------------------
   count_processes
   Count non-empty lines in the CSV file. Assumes no header row.
   Returns number of process lines.
   --------------------------------------------------------------------- */
int count_processes(char *processesCsvFilePath)
{
    /* open file and iterate lines while tracking file position */
    FILE *file = fopen(processesCsvFilePath, "r");
    int lines = 0;
    char line[256];
    long last;
    while (fgets(line, sizeof(line), file))
    {
        lines++;
        last = ftell(file);
    }

    /* if the last line was just a newline, reduce count */
    fseek(file, last, SEEK_SET);
    if (fgets(line, sizeof(line), file))
    {
        if (strcmp(line, "\n") == 0)
        {
            lines--;
        }
    }

    /* close and return */
    fclose(file);
    return lines;
}

/* ---------------------------------------------------------------------
   initialize
   Reset runtime fields for all processes before running a scheduler.
   --------------------------------------------------------------------- */
void initialize()
{
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        processes[i]->compeletion_time = 0;
        processes[i]->is_done = 0;
        processes[i]->remainning_time = processes[i]->Burst_Time;
    }
}

/* ---------------------------------------------------------------------
   sort
   Simple in-place sort by Arrival_Time (stable in practice for input).
   --------------------------------------------------------------------- */
void sort()
{
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        for (int j = i + 1; j < PROCESSES_AMOUNT; j++)
        {
            if (processes[i]->Arrival_Time > processes[j]->Arrival_Time)
            {
                process *temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }
}

/* ---------------------------------------------------------------------
   FCFS
   First-Come-First-Served (non-preemptive) simulation.
   Uses fork/ alarm / pause / wait to emulate run segments.
   --------------------------------------------------------------------- */
void FCFS()
{
    /* print header for FCFS */
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "FCFS");
    write(STDOUT_FILENO, buffer, strlen(buffer));

    /* scheduler runtime variables */
    int total_time_waiting = 0;
    int curr = 0;
    int time = 0;
    int starting_time = 0;
    int is_idle = 0;

    /* iterate processes in arrival order (array already sorted) */
    while (curr < PROCESSES_AMOUNT)
    {
        process *p = processes[curr];
        int burst = 0;
        is_idle = 0;

        /* if next process hasn't arrived yet, CPU is idle until arrival */
        if (time < p->Arrival_Time)
        {
            is_idle = 1;
        }

        /* compute how long to advance time: idle gap or full burst */
        if (is_idle)
        {
            burst = p->Arrival_Time - time;
        }
        else
        {
            burst = p->Burst_Time;
        }

        /* fork a child to emulate a process (child pauses until scheduled) */
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            /* parent waits for child, advances the simulated time, prints entry */
            wait(NULL);
            starting_time = time;
            time += burst;

            log_event(starting_time, time, p, is_idle);

            if (!is_idle)
            {
                p->compeletion_time = time;
                curr++;
            }
        }
    }

    /* compute average waiting time */
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }
    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;

    /* print FCFS summary */
    snprintf(buffer, sizeof(buffer),
             "\n──────────────────────────────────────────────\n"
             ">> Engine Status  : Completed\n"
             ">> Summary        :\n"
             "   └─ Average Waiting Time : %.2f time units\n"
             ">> End of Report\n"
             "══════════════════════════════════════════════\n\n",
             avg_waiting_time);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

/* ---------------------------------------------------------------------
   SJF
   Shortest Job First (non-preemptive) scheduler simulation.
   Selects ready process with smallest Burst_Time.
   --------------------------------------------------------------------- */
void SJF()
{
    /* print header for SJF */
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "SJF");
    write(STDOUT_FILENO, buffer, strlen(buffer));

    int starting_time = 0;
    int is_idle = 0;
    int curr = -1;
    int time = 0;
    int total_time_waiting = 0;
    int done_processes = 0;

    /* loop until all processes are handled */
    while (done_processes < PROCESSES_AMOUNT)
    {
        curr = -1;
        int burst = 0;
        int min_burst = INT_MAX;

        /* find the ready process with the smallest burst time */
        for (int i = 0; i < PROCESSES_AMOUNT; i++)
        {
            process *p = processes[i];
            if (p->Arrival_Time > time || p->is_done == 1)
                continue;
            if (p->Burst_Time < min_burst)
            {
                min_burst = p->Burst_Time;
                curr = i;
            }
        }

        /* if no process is ready, advance to next arrival (idle) */
        if (curr == -1)
        {
            is_idle = 1;
            int next = 0;
            for (int i = 0; i < PROCESSES_AMOUNT; i++)
            {
                if (processes[i]->is_done == 1)
                    continue;
                next = i;
                break;
            }
            burst = processes[next]->Arrival_Time - time;
        }
        else
        {
            is_idle = 0;
            burst = processes[curr]->Burst_Time;
        }

        /* fork a child to emulate the chosen process or idle period */
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            /* parent waits for child, updates time and prints record */
            wait(NULL);
            starting_time = time;
            time += burst;

            process *p1 = processes[curr];
            log_event(starting_time, time, p1, is_idle);
            if (!is_idle)
            {
                done_processes++;
                p1->compeletion_time = time;
                p1->is_done = 1;
            }
        }
    }

    /* compute average waiting time after loop */
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }

    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;

    /* print SJF summary */
    snprintf(buffer, sizeof(buffer),
             "\n──────────────────────────────────────────────\n"
             ">> Engine Status  : Completed\n"
             ">> Summary        :\n"
             "   └─ Average Waiting Time : %.2f time units\n"
             ">> End of Report\n"
             "══════════════════════════════════════════════\n\n",
             avg_waiting_time);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

/* ---------------------------------------------------------------------
   Priority_Scheduling
   Non-preemptive priority scheduler. Lower Priority value => higher priority.
   --------------------------------------------------------------------- */
void Priority_Scheduling()
{
    /* header for Priority scheduling */
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "Priority");
    write(STDOUT_FILENO, buffer, strlen(buffer));

    int starting_time = 0;
    int is_idle = 0;
    int curr = -1;
    int time = 0;
    int total_time_waiting = 0;
    int done_processes = 0;

    /* main loop until all are scheduled */
    while (done_processes < PROCESSES_AMOUNT)
    {
        curr = -1;
        int burst = 0;
        int min_priority = INT_MAX;

        /* choose ready process with smallest priority value */
        for (int i = 0; i < PROCESSES_AMOUNT; i++)
        {
            process *p = processes[i];
            if (p->Arrival_Time > time || p->is_done == 1)
                continue;
            if (p->Priority < min_priority)
            {
                min_priority = p->Priority;
                curr = i;
            }
        }

        /* if none ready, advance to next arrival time */
        if (curr == -1)
        {
            is_idle = 1;
            int next = 0;
            for (int i = 0; i < PROCESSES_AMOUNT; i++)
            {
                if (processes[i]->is_done == 1)
                    continue;
                next = i;
                break;
            }
            burst = processes[next]->Arrival_Time - time;
        }
        else
        {
            is_idle = 0;
            burst = processes[curr]->Burst_Time;
        }

        /* fork to emulate CPU running the selected slot */
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            /* parent handles timing and output */
            wait(NULL);
            starting_time = time;
            time += burst;

            process *p1 = processes[curr];
            log_event(starting_time, time, p1, is_idle);
            if (!is_idle)
            {
                done_processes++;
                p1->compeletion_time = time;
                p1->is_done = 1;
            }
        }
    }

    /* compute average waiting time for priority scheduler */
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }

    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;

    /* print summary for Priority scheduling */
    snprintf(buffer, sizeof(buffer),
             "\n──────────────────────────────────────────────\n"
             ">> Engine Status  : Completed\n"
             ">> Summary        :\n"
             "   └─ Average Waiting Time : %.2f time units\n"
             ">> End of Report\n"
             "══════════════════════════════════════════════\n\n",
             avg_waiting_time);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

/* ---------------------------------------------------------------------
   wakeup_handler
   Minimal signal handler used by child processes for SIGCONT.
   --------------------------------------------------------------------- */
void wakeup_handler(int sig)
{
    (void)sig; /* intentionally unused */
}

/* ---------------------------------------------------------------------
   Round_Robin
   Round Robin scheduler: forks children that decrement remaining_time,
   parent controls execution by sending SIGCONT/SIGSTOP and updating time.
   --------------------------------------------------------------------- */
void Round_Robin(int timeQuantum)
{
    /* print header for Round Robin */
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "Round Robin");
    write(STDOUT_FILENO, buffer, strlen(buffer));

    /* array to store child PIDs (0 means finished) */
    pid_t pids[1000];

    /* fork one child per process; children will pause until scheduled */
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            /* child installs handler and waits for SIGCONT */
            signal(SIGCONT, wakeup_handler);
            pause();

            /* child simulates work by sleeping and decrementing remaining_time */
            process *p = processes[i];
            while (p->remainning_time > 0)
            {
                sleep(1);
                p->remainning_time--;
            }
            exit(0);
        }
        else
        {
            /* parent stores child's pid for later control */
            pids[i] = pid;
        }
    }

    /* Round Robin dispatch loop variables */
    int time = 0;
    int idle_time = 0;
    int curr = -1;

    /* loop until all children have finished */
    while (done_processes2 < PROCESSES_AMOUNT)
    {
        /* find the next ready process in circular order */
        int temp = 0;
        process *p1;
        for (int i = 0; i < PROCESSES_AMOUNT; i++)
        {
            temp = (curr + i + 1) % PROCESSES_AMOUNT;
            p1 = processes[temp];
            if (p1->Arrival_Time <= time && pids[temp] != 0)
            {
                break;
            }
        }

        /* if no process ready, advance time (idle) */
        if (!(p1->Arrival_Time <= time && pids[temp] != 0))
        {
            idle_time++;
            time++;
            sleep(1);
            continue;
        }

        /* when we were idle before, print idle interval */
        if (idle_time > 0)
        {
            log_event(time - idle_time, time, NULL, 1); // NULL for process, is_idle=1
            idle_time = 0;
        }

        /* schedule found process: send SIGCONT and sleep for the burst */
        curr = temp;
        kill(pids[curr], SIGCONT);
        int burst = timeQuantum < p1->remainning_time ? timeQuantum : p1->remainning_time;
        sleep(burst);

        /* update remaining time and possibly mark finished */
        p1->remainning_time -= burst;
        if (p1->remainning_time == 0)
        {
            pids[curr] = 0;
            done_processes2++;
        }
        else
            kill(pids[curr], SIGSTOP);

        /* print timeline entry for this time slice */
        log_event(time, time + burst, p1, 0); // is_idle=0
        time += burst;
    }

    /* print Round Robin summary with total turnaround time */
    snprintf(buffer, sizeof(buffer),
             "\n──────────────────────────────────────────────\n"
             ">> Engine Status  : Completed\n"
             ">> Summary        :\n"
             "   └─ Total Turnaround Time : %d time units\n"
             ">> End of Report\n"
             "══════════════════════════════════════════════\n\n",
             time);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

// Function to log the event of a process running or CPU being idle
// This centralizes all output formatting for easier maintenance
void log_event(int start_time, int end_time, process *p, int is_idle)
{

    // Check if the CPU was idle during this time slice
    if (is_idle)
    {
        // Format the idle time message
        snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", start_time, end_time);
    }
    else
    {
        // Format the message for a running process including its name and description
        snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n",
                 start_time, end_time, p->Name, p->Description);
    }

    // Write the formatted message to standard output
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

/* ---------------------------------------------------------------------
   runCPUScheduler
   Top-level driver: loads CSV, initializes, sorts and runs each scheduler.
   --------------------------------------------------------------------- */
void runCPUScheduler(char *processesCsvFilePath, int timeQuantum)
{
    PROCESSES_AMOUNT = count_processes(processesCsvFilePath);
    parse(processesCsvFilePath);
    initialize();
    sort();
    FCFS();
    initialize();
    sort();
    SJF();
    initialize();
    sort();
    Priority_Scheduling();
    initialize();
    sort();
    Round_Robin(timeQuantum);
}

/* ---------------------------------------------------------------------
   main
   Minimal launcher that calls runCPUScheduler with CLI args (no checks).
   --------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    char *processesCsvFilePath = argv[1];
    int timeQuantum = atoi(argv[2]);

    runCPUScheduler(processesCsvFilePath, timeQuantum);

    return 0;
}
