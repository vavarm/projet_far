#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20

typedef struct {
    int dSC;
    char pseudo[PSEUDO_LENGTH];
}clientConnecte;

int ind = 0;
clientConnecte clients[MAX_CLIENTS];

/*
void *clientBroadcast(void *ind_client)
{
    int index_client = (int)ind_client; // cast dSc into int
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    while (1)
    {
        if (recv(clients[index_client], msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            exit(0);
        }

        for (int i = 0; i < ind; i++)
        {
            if (index_client != i)
            {
                // printf("%d\n", clients[i]);
                if (send(clients[i], msg, strlen(msg) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    exit(0);
                }
            }
        }
        if (strcmp(msg, "fin") == 0)
        {
            printf("🛑 --- FIN DE CONNEXION --- 🛑\n");
            exit(0);
        }
    }
}
*/


void *client (void *ind_client){
    int index_client = (int)ind_client; // cast dSc into int
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    char *pseudo = malloc(sizeof(char) * (PSEUDO_LENGTH + 1));
    while(1){
        int error = 0;
        if (recv((clients[index_client]).dSC, pseudo, sizeof(char) * (PSEUDO_LENGTH + 1), 0) <= 0){
            printf("❗ ERROR : recv pseudo \n");
            clients[index_client].dSC = -1;
            break;
        }
        for (int i = 0; i < ind; i++){
            if (strcmp(pseudo, (clients[i].pseudo)) == 0){
                printf("❗ ERROR : pseudo déjà utilisé \n");
                error = 1;
                if (send((clients[index_client]).dSC, "ko", strlen("ko") + 1, 0) <= 0){
                    printf("❗ ERROR : send ko \n");
                    break;
                }
            }
        }
        if (error ==0){ 
            strcpy((clients[index_client]).pseudo, pseudo);
            if (send((clients[index_client]).dSC, "ok", strlen("ok") + 1, 0) <= 0){
                printf("❗ ERROR : send ok \n");
                clients[index_client].dSC = -1;
                break;
            }
            break;
        }
    }
    printf("|---- pseudo -> %s\n", (clients[index_client]).pseudo);
    

    while (1){
        if (recv(clients[index_client].dSC, msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            clients[index_client].dSC = -1;
            printf("|--- Client déconnecté\n");
            break;
        }

        if (strcmp(msg, "fin") == 0)
        {
            //printf("\n\t🛑 --- FIN DE CONNEXION --- 🛑\n"); 
            clients[index_client].dSC = -1;
            printf("|--- Client déconnecté\n");
            break;
        }

        for (int i = 0; i < ind; i++)
        {
            if (index_client != i && clients[i].dSC != -1)
            {
                if (send(clients[i].dSC, msg, strlen(msg) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    clients[index_client].dSC = -1;
                    printf("|--- Client déconnecté\n");
                    break;
                }
                // DEBUG : affichage du message envoyé
                /*else{
                    printf("|--- Message envoyé à \n");
                }*/
                
            }
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
    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1)
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
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);
        int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
        clients[ind].dSC = dSC;

        pthread_t thread;
        pthread_create(&thread, NULL, client, (void *)ind);
        printf("|--- Client Connecté\n");
        ind++;
    }
}