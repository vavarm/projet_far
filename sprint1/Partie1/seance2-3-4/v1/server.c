#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LENGTH 100

int clients[2];

void* client1_to_client2(void *arg){
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    while(1){
        if(recv(clients[0], msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0){
            printf("❗ ERROR : recv \n");
            exit(0);
        }
        if(send(clients[1], msg, strlen(msg)+1, 0) <= 0){
            printf("❗ ERROR : send \n");
            exit(0);
        }
        if(strcmp(msg, "fin") == 0){
            printf("🛑 --- FIN DE CONNEXION --- 🛑\n");
            exit(0);
        }
    }
}

void* client2_to_client1(void *arg){
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    while(1){
        if(recv(clients[1], msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0){
            printf("❗ ERROR : recv \n");
            exit(0);
        }
        if(send(clients[0], msg, strlen(msg)+1, 0) <= 0){
            printf("❗ ERROR : send \n");
            exit(0);
        }
        if(strcmp(msg, "fin") == 0){
            printf("🛑 --- FIN DE CONNEXION --- 🛑\n");
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("❗ ERROR Argument: nombre d'argument invalide (port)\n");
        return -1;
    }

    printf("Début programme\n");
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));
    int b = bind(dS, (struct sockaddr *)&ad, sizeof(ad));
    if (b == -1)
    {
        printf("❗ ERROR : bind (le port est peut être déjà utilisé)\n");
        exit(0);
    }
    printf("Socket Nommé\n");

    int l = listen(dS, 7);
    if (l == -1)
    {
        printf("❗ ERROR : listen\n");
        exit(0);
    }

    printf("En attente de connexion\n");
    printf("... \n");

    while (1)
    {
        if (clients[0] == NULL)
        {
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            clients[0] = dSC;
            printf("|--- Client 1 Connecté\n");
        }
        if (clients[1] == NULL) 
        {
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            clients[1] = dSC;
            printf("|--- Client 2 Connecté\n");
        }


        if (clients[0] != NULL && clients[1] != NULL)
        {
            pthread_t thread1;
            pthread_t thread2;
            pthread_create(&thread1, NULL, client1_to_client2, NULL);
            pthread_create(&thread2, NULL, client2_to_client1, NULL);
        }

    }
}