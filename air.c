#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXLEN 512 

struct details { 
    long mtype;
    int mtext[1024]; // plane details from ATC
};
// Define semaphore and mutex
sem_t mutex;
//pthread_mutex_t mutex;

void* departureThread(void* allargs) {
    // Extract arguments
    int *dt = ((int **)allargs)[0];
    int *rw = ((int **)allargs)[1];
    int num_r = *((int **)allargs)[2];
    int airport_num = *((int **)allargs)[3];
    int msgid = *((int **)allargs)[4];

    // Extract plane details
    int plane_id = dt[2];
    int plane_weight = dt[3];
    int plane_type = dt[4];
    int num_passengers = dt[5];

    //printf("Departure: Boarding plane %d with weight %d kgs from Airport %d\n", plane_id, plane_weight, airport_num);

    // Lock mutex to ensure thread safety
   // sem_wait(&mutex);

    int weight = plane_weight;
    int rw_assigned = -1;

    // Find the best-fit runway for departure
    int min_difference = INT_MAX;
    for(int i = 0; i < num_r; i++){
        int difference = rw[i] - weight;
        if(difference >= 0 && difference < min_difference){
            min_difference = difference;
            rw_assigned = i+1;
        }
    }

    // If no suitable runway found, use the backup runway
    if(rw_assigned == -1){
        rw_assigned = num_r+1;
    }

    // Simulate boarding/loading process
    sleep(5);

    printf("Plane %d has completed boarding and taken off from Runway No. %d of Airport No. %d\n", plane_id, rw_assigned, airport_num);

    // Send message to air traffic controller
    struct details msg;
    msg.mtype = 21+airport_num;
    msg.mtext[0] = rw_assigned;
    msg.mtext[3]=plane_weight;
    msg.mtext[1]=airport_num; 
    msg.mtext[2]=plane_id;// Send the selected runway number
    msgsnd(msgid, (void*)&msg, sizeof(msg.mtext), 0);
    printf("Sent Departure details to ATC\n");
    // Unlock mutex
    sem_post(&mutex);

    pthread_exit(NULL);
}

void* arrivalThread(void* allargs) {
    // Extract arguments
    int *dt = ((int **)allargs)[0];
    int *rw = ((int **)allargs)[1];
    int num_r = *((int **)allargs)[2];
    int airport_num = *((int **)allargs)[3];
    int msgid = *((int **)allargs)[4];

    // Extract plane details
    int plane_id = dt[2];
    int plane_weight = dt[3];
    int plane_type = dt[4];
    int num_passengers = dt[5];
    printf("Flying\n");
    sleep(30);
    printf("Arrival: Landing plane %d with weight %d kgs at Airport %d\n", plane_id, plane_weight, airport_num);

    // Lock mutex to ensure thread safety
    //sem_wait(&mutex);

    int weight = plane_weight;
    int rw_assigned = -1;

    // Find the best-fit runway for arrival
    int min_difference = INT_MAX;
    for(int i = 0; i < num_r; i++){
        int difference = rw[i] - weight;
        if(difference >= 0 && difference < min_difference){
            min_difference = difference;
            rw_assigned = i+1;
        }
    }

    // If no suitable runway found, use the backup runway
    if(rw_assigned == -1){
        rw_assigned = num_r+1;
    }

    // Simulate landing and deboarding/unloading process
    printf("Landing\n");
    sleep(2); // Simulate landing
    printf("Deboarding\n");
    sleep(3); // Simulate deboarding/unloading

    printf("Plane %d has completed deboarding and has landed on Runway No. %d of Airport No. %d\n", plane_id, rw_assigned, airport_num);

    // Send message to air traffic controller
    struct details msg;
    msg.mtype = 32+airport_num;
    msg.mtext[0] = rw_assigned;
    msg.mtext[3]=plane_weight;
    msg.mtext[1]=airport_num; // Send the selected runway number
    msgsnd(msgid, (void*)&msg, sizeof(msg), 0);
     printf("Sent Arrival details to ATC\n");
    // Unlock mutex
    sem_post(&mutex);
    pthread_exit(NULL);
}

int main() {
    struct details pd;
    int airport_num, num_runways;

    // Initialize semaphore
    sem_init(&mutex, 0, 1); // Initialize mutex to 1

    // Initialize mutex
    //pthread_mutex_init(&mutex, NULL);

    printf("Enter Airport Number: ");
    scanf("%d", &airport_num);

    printf("Enter number of Runways: ");
    scanf("%d", &num_runways);

    num_runways++; // for the backup runway
    int cap[2][num_runways]; // creating 2d array to store loadCapacity of each runway

    printf("Enter loadCapacity of runways: ");
    for (int j=0; j<num_runways-1; j++) {
        scanf("%d", &cap[1][j]);
    };
    for(int i=0; i<num_runways; i++){
        cap[0][i] = i+1;
    };
    cap[1][num_runways-1] = 15000;

    int msgid;
    msgid = msgget(12345, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("ggmsgget");
        exit(1);
    }
    if (msgrcv(msgid, (void*)&pd, sizeof(pd), 10+airport_num, 0) == -1){
    	   printf("error in receiving message\n");
        exit(1);
    }
    while(pd.mtext[6]==0){
        int action = pd.mtext[0]; //finding whether to schedule departure or arrival
        //printf("bc %d\n",action);
        if(airport_num == pd.mtext[1]) { // Check if received airport number matches this airport's number
            //printf("Received message from ATC: Action %d, Plane ID %d, Weight %d, Type %d, Passengers %d\n", pd.mtext[0], pd.mtext[2], pd.mtext[3], pd.mtext[4], pd.mtext[5]);
            void *allargs[] = {(void*)pd.mtext, (void*)cap[1], (void*)&num_runways, (void*)&airport_num, (void*)&msgid}; //array of pointers to send to runner functions

            pthread_t thread;
            if(action == 4) {
                sem_wait(&mutex); // Wait until semaphore is available
                pthread_create(&thread, NULL, departureThread, allargs);
            }
            else if(action == 5) {
                sem_wait(&mutex); // Wait until semaphore is available
                pthread_create(&thread, NULL, arrivalThread, allargs);
            }

            pthread_join(thread, NULL); // Wait for the thread to finish
            sem_post(&mutex); // Release semaphore
            if (msgrcv(msgid, (void*)&pd, sizeof(pd), 10+airport_num, 0) == -1){
    	        printf("\n");
                exit(1);
            }
            else{
                //printf("Status of airplane : %d\n", pd.mtext[6]);
            }
        }
        // else{
        //     if (msgrcv(msgid, (void*)&pd, sizeof(pd), 10, 0) == -1){
    	//         printf("Error in receiving message\n");
        //         exit(1);
        //     }
        // }
    }
    // Destroy semaphore and mutex
    sem_destroy(&mutex);
    //pthread_mutex_destroy(&mutex);

    return 0;
}
