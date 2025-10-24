#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#define _XOPEN_SOURCE 700

int PROCESSES_AMOUNT;
char buffer[512];
int done_processes2 = 0;

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
    // int is_started;

} process;

process *processes[1000];

void parse(char *processesCsvFilePath)
{
    FILE *file = fopen(processesCsvFilePath, "r");
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        processes[i] = malloc(sizeof(process));
        char line[256];
        // process* p;
        fgets(line, sizeof(line), file);
        int index = strcspn(line, "\n");
        if (index < strlen(line))
            line[index] = 0;
        char *token = strtok(line, ",");
        // printf("%s\n", token);
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
    fclose(file);
}

int count_processes(char *processesCsvFilePath)
{
    FILE *file = fopen(processesCsvFilePath, "r");
    int lines = 0;
    char line[256];
    long last;
    while (fgets(line, sizeof(line), file))
    {
        /* code */
        lines++;
        last = ftell(file);
    }
    fseek(file, last, SEEK_SET);
    if (fgets(line, sizeof(line), file)) {
        if (strcmp(line, "\n") == 0) {
            lines--;
        }
    }
    fclose(file);
    return lines;
}

void initialize()
{
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        processes[i]->compeletion_time = 0;
        processes[i]->is_done = 0;
        processes[i]->remainning_time = processes[i]->Burst_Time;
    }
}

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

void FCFS()
{
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "FCFS");

    write(STDOUT_FILENO, buffer, strlen(buffer));
    int total_time_waiting = 0;
    int curr = 0;
    int time = 0;
    int starting_time = 0;
    int is_idle = 0;
    while (curr < PROCESSES_AMOUNT)
    {
        process *p = processes[curr];
        int burst = 0;
        is_idle = 0;
        if (time < p->Arrival_Time)
        {
            is_idle = 1;
        }
        if (is_idle)
        {
            burst = p->Arrival_Time - time;
        }
        else
        {
            burst = p->Burst_Time;
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            wait(NULL);
            starting_time = time;
            time += burst;
            if (is_idle)
            {
                snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", starting_time, time);
            }
            else
            {
                p->compeletion_time = time;
                curr++;
                snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", starting_time, time, p->Name, p->Description);
            }
            write(STDOUT_FILENO, buffer, strlen(buffer));
        }
    }
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }
    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;
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

void SJF()
{
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
    while (done_processes < PROCESSES_AMOUNT)
    {
        curr = -1;
        int burst = 0;
        int min_burst = INT_MAX;
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
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            wait(NULL);
            starting_time = time;
            time += burst;
            if (is_idle)
            {
                snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", starting_time, time);
            }
            else
            {
                done_processes++;
                process *p1 = processes[curr];
                p1->compeletion_time = time;
                p1->is_done = 1;
                snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", starting_time, time, p1->Name, p1->Description);
            }
            write(STDOUT_FILENO, buffer, strlen(buffer));
        }
    }
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }

    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;
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

void Priority_Scheduling()
{
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
    while (done_processes < PROCESSES_AMOUNT)
    {
        curr = -1;
        int burst = 0;
        int min_priority = INT_MAX;
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
        pid_t pid = fork();
        if (pid == 0)
        {
            alarm(1);
            pause();
        }
        else
        {
            wait(NULL);
            starting_time = time;
            time += burst;
            if (is_idle)
            {
                snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", starting_time, time);
            }
            else
            {
                done_processes++;
                process *p1 = processes[curr];
                p1->compeletion_time = time;
                p1->is_done = 1;
                snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", starting_time, time, p1->Name, p1->Description);
            }
            write(STDOUT_FILENO, buffer, strlen(buffer));
        }
    }
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        process *p = processes[i];
        total_time_waiting += p->compeletion_time - p->Burst_Time - p->Arrival_Time;
    }

    float avg_waiting_time = (float)total_time_waiting / PROCESSES_AMOUNT;
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

void wakeup_handler(int sig)
{
}

void Round_Robin(int timeQuantum)
{
    snprintf(buffer, sizeof(buffer), "══════════════════════════════════════════════\n"
                                     ">> Scheduler Mode : %s\n"
                                     ">> Engine Status  : Initialized\n"
                                     "──────────────────────────────────────────────\n\n",
             "Round Robin");

    write(STDOUT_FILENO, buffer, strlen(buffer));
    pid_t pids[1000];
    for (int i = 0; i < PROCESSES_AMOUNT; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            signal(SIGCONT, wakeup_handler);
            // printf("child waiting %d\n",i);
            pause();
            // printf("child starting %d\n", i);

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
            // kill(SIGSTOP, pid);
            pids[i] = pid;
            // printf("pid : %d i : %d\n", pids[i], i);
        }
    }
    int time = 0;
    int idle_time = 0;
    int curr = -1;

    while (done_processes2 < PROCESSES_AMOUNT)
    {
        // find the next process
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

        // printf("temp is %d\n", temp);
        if (!(p1->Arrival_Time <= time && pids[temp] != 0))
        {
            idle_time++;
            time++;
            sleep(1);
            continue;
        }
        // printf("idle %d\n", idle_time);
        if (idle_time > 0)
        {
            snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", time - idle_time, time);
            write(STDOUT_FILENO, buffer, strlen(buffer));
            idle_time = 0;
        }
        curr = temp;
        // printf("kill\n");
        kill(pids[curr], SIGCONT);
        int burst = timeQuantum < p1->remainning_time ? timeQuantum : p1->remainning_time;
        sleep(burst);
        p1->remainning_time -= burst;
        if (p1->remainning_time == 0)
        {
            pids[curr] = 0;
            done_processes2++;
        }
        else
            kill(pids[curr], SIGSTOP);
        snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", time, time + burst, p1->Name, p1->Description);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        time += burst;
    }
    snprintf(buffer, sizeof(buffer),
             "\n──────────────────────────────────────────────\n"
             ">> Engine Status  : Completed\n"
             ">> Summary        :\n"
             "   └─ Total Turnaround Time : %d time units\n\n"
             ">> End of Report\n"
             "══════════════════════════════════════════════\n\n",
             time);
    write(STDOUT_FILENO, buffer, strlen(buffer));
}

void runCPUScheduler(char *processesCsvFilePath, int timeQuantum)
{
    // printf("check!!!\n");

    PROCESSES_AMOUNT = count_processes(processesCsvFilePath);
    // printf("---------- PROCESSES_AMOUNT %d\n", PROCESSES_AMOUNT);
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

    // for (int i = 0; i < PROCESSES_AMOUNT; i++) {
    //         printf(" %s (%s), Arrival: %d, Burst: %d, Priority: %d, Completion: %d, Remaining: %d, Done: %d\n",

    //             processes[i]->Name,
    //             processes[i]->Description,
    //             processes[i]->Arrival_Time,
    //             processes[i]->Burst_Time,
    //             processes[i]->Priority,
    //             processes[i]->compeletion_time,
    //             processes[i]->remainning_time,
    //             processes[i]->is_done
    //         );

    //     }
}

int main(int argc, char *argv[]) {
    char *processesCsvFilePath = argv[1];
    int timeQuantum = atoi(argv[2]);

    runCPUScheduler(processesCsvFilePath, timeQuantum);

    return 0;
}