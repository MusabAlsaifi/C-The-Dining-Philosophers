#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <math.h>

int NUM_PHIL = 5;
int forksSetId = -1;
int externalInterrupt = false;

int counterSemaphore  = -1;

void philosopherRoutine(int num);
void grab_forks(int leftForkId);
void put_away_forks(int leftForkId);
void interruptHandler(); 

int main() {

	signal(SIGINT, interruptHandler);
	
	// Creating the semaphore which is used to make sure the philosphers eat fairly
	counterSemaphore =  semget(IPC_PRIVATE, 1, 0644 | IPC_CREAT );
	semctl(counterSemaphore, 0 , SETVAL,0);

	// Creating the Semaphore Group 	
	forksSetId = semget(IPC_PRIVATE, NUM_PHIL, 0644 | IPC_CREAT );


	if (forksSetId == -1) {
		printf("PARENT [%ld]: Creation of Semaphore failed\n",(long)getpid());
		exit(1);
	}
	
	//Setting intial value of each fork(semaphore) to 1
	for(int i =0; i < NUM_PHIL;i++) {
		semctl(forksSetId, i , SETVAL,1);
	}
	
	for(int i =0; i < NUM_PHIL; i++) {
		int processId = fork();
		if(processId == 0) {
			//We are in Child Process and the child acts as philosopher
			philosopherRoutine(i);
			exit(0);
		}
		else if (processId == -1) {
			printf("Parent [%ld]: Error in Child Creation\n",(long)getpid());
	               signal(SIGTERM,SIG_IGN); // To ignore SIGTERM signal in the parent
                	kill(0,SIGTERM);
                	return 1;
		}
	}
	
	int count = 0;
	while(1) {
		if(wait(NULL) == -1) 
		{
		    if(errno == ECHILD) printf("Parent [%ld]: Finished. Number of philosophers terminated = %d\n",(long)getpid(),count);
		    else    printf("parent [%ld]: Unexpected Error from wait\n",(long)getpid());
	            return 0;
		}
		else count++;
	}
    semctl(counterSemaphore, 0, IPC_RMID);

}

void philosopherRoutine(int num) {
	int count = 0;
	while(!externalInterrupt) {
		printf("Philosopher [%d] : Thinking...\n",num);
		sleep(1);
		while(count != floor(semctl(counterSemaphore, 0,GETVAL)/NUM_PHIL) ){
		//This loop makes sure that the philosophers eat fairly by comparing the no of meals had by one philosopher with the others. Using the semaphore callled counterSemaphore.
			sleep(1);
		}
		grab_forks(num);
		printf("Philosopher [%d] : Eating...\n",num);
		sleep(1);
		put_away_forks(num);	
		count++;
		printf("Philosopher [%d] : Meals Eaten: %d\n",num,count);
	}
	printf("Philosopher [%d] : Finished. Final Meals Eaten: %d\n",num,count);
	
}

void grab_forks(int leftForkId){

	int rightForkId = (leftForkId + 1)% NUM_PHIL;
	
	struct sembuf sops[2] = {  // Size of array is 2 as we operate on two semaphores at the same time
		{leftForkId,-1,0},	 // The semval is decremented by one i.e set to 0 to indiacte it is taken
		{rightForkId,-1,0}
	};
	
	//The Philospher attempts to grab the forks. If the forks are not free i.e semval of any of them is 0 then it waits for it to increase to 1.
	if(semop(forksSetId, sops, 2) == -1) { 
		printf("Philosopher [%d]: Error in semop while picking up: %s \n",leftForkId,strerror(errno));
	}
	fflush(stdout);
    	printf("Philosopher [%d]: Picked Fork %d and Fork %d\n", leftForkId, leftForkId, rightForkId);
    	
    	struct sembuf sops_d[1] ={
    		{0,1,0}
    	};
    	
    	semop(counterSemaphore,sops_d,1); //Increment global counter semaphore
    	
	//fflush(stdout);
    	//printf("Global counter: %d\n",semctl(counterSemaphore, 0,GETVAL));

}

void put_away_forks(int leftForkId) {

	int rightForkId = (leftForkId + 1)% NUM_PHIL;
	
	struct sembuf sops[2] = {  // Size of array is 2 as we operate on two semaphores at the same time
		{leftForkId,1,0},	 // The semval is incremented by one i.e set to 1 to indiacte it is free
		{rightForkId,1,0}
	};
	
	//The Philospher releases the forks.
	if(semop(forksSetId, sops, 2) == -1) { 
		printf("Philosopher [%d]: Error in semop while putting away\n",leftForkId);
	}
	
    	printf("Philosopher [%d]: Put Away Fork %d and Fork %d\n", leftForkId, leftForkId, rightForkId);
	
}

void interruptHandler() {
	externalInterrupt = true;
	printf("Interrupt Received\n");
}


