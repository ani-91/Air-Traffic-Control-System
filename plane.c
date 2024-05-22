#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include<wait.h>

#define READ_END 0
#define WRITE_END 1
#define MSGQ_KEY 12345

struct plane{
    long mtype;
    int id;
    int type;
    int weight;
    int dep_airport;
    int arr_airport;
    int passengers;
    int takeoff;
    int flag;
};

struct {
    long mtype;
    int flag;
} dep;
struct {
    long mtype;
    int flag;
} arr;

int main(){
    struct plane plane;
	int msg_id;
	key_t key;
    //system("touch AirTrafficController.txt");

    key = ftok("AirTrafficController.txt", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    msg_id = msgget(12345, 0666|IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Enter Plane ID: ");
    scanf("%d",&plane.id);

    printf("Enter Type of Plane (0->Cargo Plane, 1->Passenger Plane): ");
    scanf("%d",&plane.type);

    int num_cargo=0;
    int cargo_weight=0;
    int total_weight=0;
    
    if(plane.type==0){
        printf("Enter Number of Cargo Items: ");
        scanf("%d",&num_cargo);
        printf("Enter Average Weight of Cargo Items: ");
        scanf("%d",&cargo_weight);
        plane.passengers=0;
        total_weight=150+(cargo_weight*num_cargo);
    } 
    else {
        printf("Enter the Number of Occupied Seats: ");
        scanf("%d",&plane.passengers);

        int pipes[plane.passengers][2];

        for(int i=0;i<plane.passengers;i++){
            if(pipe(pipes[i])==-1){
                fprintf(stderr,"Pipe Failed");
                return 1;
            }
        }

        for(int i=0;i<plane.passengers;i++){

            pid_t pid=fork();

            if(pid<0){
                fprintf(stderr, "Fork Failed");
		        return 1;
            }
            else if(pid==0){
                int luggage_weight;
                printf("Passenger %d->\n",i+1);
                printf("\tPlease Enter Weight of Your Luggage: ");
                scanf("%d",&luggage_weight);

                int body_weight;
                printf("\tPlease Enter your Body Weight: ");
                scanf("%d",&body_weight);

                int total_pass_weight=luggage_weight+body_weight;

                close(pipes[i][READ_END]);
                write(pipes[i][WRITE_END],&total_pass_weight,sizeof(total_pass_weight));
                close(pipes[i][WRITE_END]);
                return 0;
            }
            else{
                wait(NULL);
            }
        }

        int w;
        for(int i=0;i<plane.passengers;i++){
            close(pipes[i][WRITE_END]);
            read(pipes[i][READ_END],&w,sizeof(w));
            total_weight+=w;
            close(pipes[i][READ_END]);
        }

        total_weight+=525;
    }

    plane.weight=total_weight;

    printf("Enter the Airport Number for Departure: ");
    scanf("%d",&plane.dep_airport);

    printf("Enter the Airport Number for Arrival: ");
    scanf("%d",&plane.arr_airport);

    plane.mtype=1;
    if(msgsnd(msg_id,(void*)&plane,sizeof(plane),0)==-1){
        printf("Error in sending flight details to ATC.");
        return 1;
    }
    else{
        printf("-----Flight Details sent to ATC-----\n");
    }
    //Waiting for Confirmation from ATC
   if(msgrcv(msg_id,(void *)&dep,sizeof(dep),100,0)==-1){
        printf("Error in receiving message from ATC.\n");
        return 1;
    }
    else{
        if(dep.flag==1){
            printf("Departure details recieved, Taking Off! Happy Journey!\n");
            //printf("%d\n",200+plane.weight);
            sleep(30);
        }
        else{
            printf("Plane.flag= %d\n", plane.flag);
            printf("Take Off Cancelled!\n");
            return 1;
        }
    }
//arrival airport messages
    //printf("%d\n",200+plane.id);
    if(msgrcv(msg_id,(void *)&arr,sizeof(arr),200+plane.weight,0)==-1){
            printf("Error in receiving message from ATC.\n");
            return 1;
        }
        else{
            if(arr.flag==1){
                printf("Landing and the deboarding/unloading process is complete,\n");
            }
            else{
                printf("No arrival details recieved, Landing failed\n");
                return 1;
            }
        }

    printf("Plane %d has succesfully travelled from Airport %d to Airport %d!\n",plane.id,plane.dep_airport,plane.arr_airport);
    
    return 0;
}

