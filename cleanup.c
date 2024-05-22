#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

#define QUEUE_KEY 12345 // Unique key for message queue
#define MAXLEN 1024

struct Message {
    long mtype;
    int terminate;
};

int main() {
    int msgid;
    struct Message msg;
    char input[MAXLEN];

    // Create a message queue
    msgid = msgget(QUEUE_KEY, 0666|IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    msg.mtype=999;

    int terminate = 0;
    while (terminate != 1) {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No)->\n");

         // Read input as a string
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // Remove newline character if present
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "Y") == 0 || strcmp(input, "y") == 0) {
            terminate = 1;
            msg.terminate=1;
            if(msgsnd(msgid,(void *)&msg,sizeof(msg),0)==-1){
                perror("msgsnd");
                return 1;
            }
            else{
                printf("Sending termination request to Air Traffic Control System...\n");
            }
            break;
        }
        else if (strcmp(input, "N") == 0 || strcmp(input, "n") == 0) {
            msg.terminate=0;
            if(msgsnd(msgid,(void *)&msg,sizeof(msg),0)==-1){
                perror("msgsnd");
                return 1;
            }else{
                // printf("Cleanup process will continue running.\n");
                sleep(3);
            }
        }
        else {
            printf("Invalid Input! Please enter Y or N.\n");
        }
    }
    // Clean up IPC constructs

    printf("Cleanup process terminated.\n");

    return 0;
}

