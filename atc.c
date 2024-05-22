#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#define MAX_AIRPORTS 10
#define MAX_MSG_SIZE 256
#define MAXLEN 512
#define AIR_TRAFFIC_CONTROLLER_FILE "AirTrafficController.txt"

struct details{
	long mtype;
	int mtext[1024];
	};
// Define message structure
struct plane {
    long mtype;
    int id;
    int type;
    int weight;
    int dep_airport;
    int arrv_airport;
    int passengers;
    int flag;
};
struct dep{
    long mtype;
    int flag;
} dep;
struct arr {
    long mtype;
    int flag;
} ;
struct Message {
    long mtype;
    int terminate;
};

// Function to log plane journey
void log_journey(int plane_id, int departure_airport, int arrival_airport) {
    FILE *file = fopen(AIR_TRAFFIC_CONTROLLER_FILE, "a");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane_id, departure_airport, arrival_airport);
    fclose(file);
}

int main() {
    int num_airports;
    key_t key;
    int msgid;
    struct plane plane;
    struct Message msg;
    struct details pd[200];
    struct dep dep[100];
    struct arr arr[100];
    int planes_in_airports[MAX_AIRPORTS] = {0}; 
    int active=0;
    int arri[100];

    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);
    
    system("touch AirTrafficController.txt");

    key = ftok("AirTrafficController.txt", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    msgid = msgget(12345, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    msg.terminate=0;
    int i=-1;
    while (1) {
        if(msgrcv(msgid,(void *)&msg,sizeof(msg),999,IPC_NOWAIT) == -1){
            if (errno == ENOMSG) {
                //nothing from cleanup yet
            } else {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        
        }else{
            //printf("Cleanup sent : %d\n", msg.terminate);
            if(msg.terminate==1 && active==0){
                printf("All Airplanes landed, Terminating ATC\n");
                if(msg.terminate==1 && active==0){
                //printf("All Airplanes landed, Closing all airports and Terminating ATC\n");
                for(int j=0;j<=i;j++){ //closing all running airports
                  
                    pd[j].mtext[6]=1;
                    //printf("Closing Departure Airport number : %d\n",pd[j].mtext[1]);
                    pd[j].mtype = 10+pd[j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[j], sizeof(pd[j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                    pd[100+j].mtext[6]=1;
                
                    //printf("Closing Arrival Airport number : %d\n",pd[100+j].mtext[1]);
                    pd[100+j].mtype = 10+pd[100+j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[100+j], sizeof(pd[100+j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                }
                if(msgctl(msgid,IPC_RMID,NULL)==-1)
                {
                    perror("Error in deleting message queue");
                    return 1;
                }
                return 0;
        }
                if(msgctl(msgid,IPC_RMID,NULL)==-1)
                {
                    perror("Error in deleting message queue");
                    return 1;
                }
                return 0;
            }
        }
        if(msg.terminate==0){
        // Receive message from plane
        if (msgrcv(msgid, (void *)&plane, sizeof(plane), 1, IPC_NOWAIT) == -1) {
            // If no message of type 1 is available, continue loop
            if (errno == ENOMSG) {
                //printf("Continue from plane reciever\n");
                
            } else {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        else{
        i++;  
        active++;     
        //printf("Active airplames= %d\n",active);
        //printf("Received message from plane : %d\n", plane.dep_airport);
        pd[i].mtext[0]= 4;
        pd[i].mtext[1]= plane.dep_airport;
        pd[i].mtext[2]= plane.id;
        pd[i].mtext[3]= plane.weight;
        pd[i].mtext[4]= plane.type;
        pd[i].mtext[5]= plane.passengers;
        pd[i].mtext[6]=0;
        arri[i]=plane.arrv_airport;
        pd[i].mtype = 10+pd[i].mtext[1];
        //Send message to the specified airport
        if (msgsnd(msgid, (void *)&pd[i], sizeof(pd[i]), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        } else {
            printf("Message sent to Departure Airport : %d\n", pd[i].mtext[1]);
        }
        //printf("here %d\n", pd[i].mtext[1]);
       if (msgrcv(msgid, (void *)&pd[i], sizeof(pd[i]), 21+pd[i].mtext[1], 0) == -1) {
                perror("msgrcv");
                exit(EXIT_FAILURE);
        }
        else{
        //printf("Departure Runway number recieved : Runway %d\n",pd[i].mtext[0]);
        log_journey(pd[i].mtext[2], pd[i].mtext[1], arri[i] );
        dep[i].mtype=100;
        dep[i].flag=1;
        msgsnd(msgid, (void *)&dep[i], sizeof(dep[i]), 0);
        printf("Departure details sent to Plane\n");
	    }
        pd[100+i].mtext[0]= 5;
        pd[100+i].mtext[1]= plane.arrv_airport;
        pd[100+i].mtext[2]= plane.id;
        pd[100+i].mtext[3]= plane.weight;
        pd[100+i].mtext[4]= plane.type;
        pd[100+i].mtext[5]= plane.passengers;
        pd[100+i].mtext[6]=0;
        pd[100+i].mtype = 10+pd[100+i].mtext[1];
        //Send message to the specified airport
        if (msgsnd(msgid, (void *)&pd[100+i], sizeof(pd[100+i]), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        } else {
            printf("Message sent to arrival airport %d\n", pd[100+i].mtext[1]);
        }}}
        for(int j=100;j<=100+i;j++){
        if (msgrcv(msgid, (void *)&pd[j], sizeof(pd[j]), 32+pd[j].mtext[1], IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                //printf("arrival reciever continuing\n");
                //break;
        } else {
            perror("msgrcv");
            exit(1);
        }
        }
        else{
        //printf("Arrival Runway number recieved : Runway %d\n",pd[j].mtext[0]);
        arr[j].mtype=(200+pd[j].mtext[3]);
        //printf("%ld\n", arr[j].mtype);
        arr[j].flag=1;
        msgsnd(msgid, (void *)&arr[j], sizeof(arr[j]), 0);
        printf("Arrival details sent to Plane\n");
        active--;
        //printf("Active airplanes= %d, i= %d\n",active,i);
        if(msg.terminate==1 && active==0){
                printf("All Airplanes landed, Closing all airports and Terminating ATC\n");
                for(int j=0;j<=i;j++){ //closing all running airports
                    pd[j].mtext[6]=1;
                    //printf("Closing Departure Airport number : %d\n",pd[j].mtext[1]);
                    pd[j].mtype = 10+pd[j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[j], sizeof(pd[j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                    pd[100+j].mtext[6]=1;
                   // printf("Closing Arrival Airport number : %d\n",pd[100+j].mtext[1]);
                    pd[100+j].mtype = 10+pd[100+j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[100+j], sizeof(pd[100+j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                }
                if(msgctl(msgid,IPC_RMID,NULL)==-1)
                {
                    perror("Error in deleting message queue");
                    return 1;
                }
                return 0;
        }
	}}
    if(msg.terminate==1 && active==0){
                printf("All Airplanes landed, Closing all airports and Terminating ATC\n");
                for(int j=0;j<=i;j++){ //closing all running airports
                    
                    pd[j].mtext[6]=1;
                    //printf("Closing Departure Airport number : %d\n",pd[j].mtext[1]);
                    pd[j].mtype = 10+pd[j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[j], sizeof(pd[j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                    pd[100+j].mtext[6]=1;
                 
                   // printf("Closing Arrival Airport number : %d\n",pd[100+j].mtext[1]);
                    pd[100+j].mtype = 10+pd[100+j].mtext[1];
                    if (msgsnd(msgid, (void *)&pd[100+j], sizeof(pd[100+j]), 0) == -1) {
                        perror("msgsnd");
                        exit(EXIT_FAILURE);
                    }
                }
                if(msgctl(msgid,IPC_RMID,NULL)==-1)
                {
                    perror("Error in deleting message queue");
                    return 1;
                }
                return 0;
        }
   
	//break;
    }
    if(msgctl(msgid,IPC_RMID,NULL)==-1)
                {
                    perror("Error in deleting message queue");
                    return 1;
                }
    return 0;
}

