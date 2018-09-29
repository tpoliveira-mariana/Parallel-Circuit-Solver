#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "Process.h"
#include "list.h"
#include "../lib/timer.h"

void manageProcesses(char * filename);
void newProcess(char * filename);
Node updateStatus(int state, int pid, Node h);

int MAXCHILDREN, currentProcesses = 0, count = 0;
Node liveProcesses, deadProcesses;

int main(int argc, char * argv[]) {
    char * input  = calloc(10000, sizeof(char)); //TODO: Make adaptive function to fit any size of input 
    char * command, *filename;

    TIMER_T stopTime;       //=======================

    liveProcesses = createNode(NULL); //list of processes running
    deadProcesses = createNode(NULL); //list of processes that have finished
    if (argc > 1) {
        sscanf(argv[1], "%d", &MAXCHILDREN);
    } else 
        MAXCHILDREN = 0;

    while(1) {
        fgets(input, 10000, stdin);
        command = strtok(input, " \n\0");
        if (!strcmp(command, "run")) {
            filename = strtok(NULL, " \n");
            manageProcesses(filename);  //=====================
        } else if (!strcmp(command, "exit"))
            break;
        else {
            printf("Invalid command\n");
        }
    }

    //after exit, we must finish all tasks and free memory
    int pid, state;
    while(next(liveProcesses) != NULL) {
        pid = wait(&state);

        TIMER_READ(stopTime);       //=========================
        Node new = updateStatus(state, pid, liveProcesses);
        new->item->finish = stopTime;
        
        insert(deadProcesses, new);
        removeByPID(pid, liveProcesses);
    }

    printAll(deadProcesses);
    printf("END.\n");

    freeAll(liveProcesses);
    freeAll(deadProcesses);
    free(input);
}

void manageProcesses(char * filename) {
    int pid, state;
    TIMER_T stopTime;   //=============================
    //printf("%d\n", currentProcesses); //debug only

    if (!MAXCHILDREN || MAXCHILDREN && currentProcesses < MAXCHILDREN) { //can start right away
        newProcess(filename);   //================

    } else if (currentProcesses >= MAXCHILDREN) { //need to wait for a process to finish
        pid = wait(&state);

        TIMER_READ(stopTime);
        Node new = updateStatus(state, pid, liveProcesses);
        new->item->finish = stopTime;

        currentProcesses--;
        insert(deadProcesses, new);        
        removeByPID(pid, liveProcesses);
        newProcess(filename);
    }
}

void newProcess(char * filename) {
    int pid, status;
    TIMER_T startTime;

    TIMER_READ(startTime);

    pid = fork();
    if (pid < 0) {
        abort();
    }
    else if (pid == 0) { //child process executes a new seq-solver
        //printf("Creating process with file %s\n", filename);
        
        execl("../CircuitRouter-SeqSolver/CircuitRouter-SeqSolver", filename, NULL);
        

    } else {
 
        Process * p = createProcess(pid); //creates new process and adds it to the list
        p->start = startTime;   
        insert(liveProcesses, createNode(p));
        currentProcesses++;
        count++;
    }
}


Node updateStatus(int state, int pid, Node h) {
    int status = -1;

    if (WIFEXITED(state))
        status = 0;

    Node new = createNode(getByPID(pid, liveProcesses));
    new->item->status = status;

    return new;
}