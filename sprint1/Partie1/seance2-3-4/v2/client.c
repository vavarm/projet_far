#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LENGTH 100

void* sendThread(void* dS){
    int ds = (int) dS;
    char* msg = malloc(sizeof(char)*(MAX_LENGTH+1));
    while(1){
        fgets(msg, MAX_LENGTH, stdin);
        if(msg[strlen(msg)-1] == '\n'){
            msg[strlen(msg)-1] = '\0';
        }
        if(send(ds, msg, strlen(msg)+1, 0) == -1){
            printf("❗ ERROR : send \n");
            exit(0);
        }
        if(strcmp(msg, "fin") == 0){
            printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
            exit(0);
        }
    }
}

void* receiveThread(void* dS){
    int ds = (int) dS;
    char* msg = malloc(sizeof(char)*(MAX_LENGTH+1));
    while(1){
        if(recv(ds, msg, sizeof(char)*(MAX_LENGTH+1), 0) == -1){
            printf("❗ ERROR : recv \n");
            exit(0);
        }
        printf("\33[2K\r");
        printf("\t\t\t ");
        puts(msg);
        if(strcmp(msg, "fin") == 0){
            printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        printf("Arguments: adresse(127.0.0.1) port\n");
        exit(0);
    }

    printf("Début programme\n");
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    if (connect(dS, (struct sockaddr *)&aS, lgA) == -1)
    {
        printf("❗ ERROR : erreur de connexion \n");
        exit(0);
    }
    else
    {
        printf("Socket Connecté\n");
    }
    printf("x-----------------------------------x\n");
    printf("Taille max message: %d caractères\n Vous pouvez commencer a discuter \n taper \"fin\" pour mettre fin a la connexion\n", MAX_LENGTH);
    printf("x-----------------------------------x\n");

    pthread_t threadSend;
    pthread_t threadReceive;
    pthread_create(&threadSend, NULL, sendThread, (void*)dS);
    pthread_create(&threadReceive, NULL, receiveThread, (void*)dS);
    pthread_join(threadReceive, NULL);
}