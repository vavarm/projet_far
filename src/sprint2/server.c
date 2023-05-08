#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#define MAX_CLIENTS 2
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20

// structure of users
typedef struct
{
    int dSC;
    char pseudo[PSEUDO_LENGTH];
} clientConnecte;

// array of clients
clientConnecte clients[MAX_CLIENTS];

// mutex that protect the array of clients
pthread_mutex_t mutex_clients;

// semaphore that manages the number of clients connected
sem_t semaphore;

// TODO: add a dissconnect status and use it in pthread_exit()
void disconnectClient(int index_client)
{
    printf("👤 %s disconnected, with dSC = %d\n", clients[index_client].pseudo, clients[index_client].dSC);
    // put the default values in the array
    clients[index_client].dSC = -1;
    clients[index_client].pseudo[0] = '\0';
    // release the semaphore to accept new clients
    sem_post(&semaphore);
    // kill the thread
    pthread_exit(NULL);
}

/*
    Commandes:
    /quit : quitter le serveur : retourne -1
    /list : lister les utilisateurs connectés : retourne 0
    /mp <pseudo> <message> : envoyer un message privé à un utilisateur : retourne 0
    En cas d'erreur, retourne 0
*/
int CommandsManager(char *msg, int index_client)
{
    if (msg[0] == '/')
    {
        if (strncmp(msg, "/cmd", sizeof(char) * 4) == 0)
        {
            FILE *ptr;
            char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
            ptr = fopen("manual.txt", "r");
            if (ptr == NULL)
            {
                printf("❗ ERROR : fopen \n");
                return 0;
            }
            while (fgets(str, MAX_LENGTH + 1, ptr) != NULL)
            {
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
                sleep(0.1);
            }
            return 0;
        }
        else if (strncmp(msg, "/quit", sizeof(char) * 5) == 0)
        {
            return -1;
        }
        else if (strncmp(msg, "/list", sizeof(char) * 5) == 0)
        {
            // return all the users to the client
            char *list = malloc(sizeof(char) * (MAX_LENGTH * (PSEUDO_LENGTH + 2) + 1));
            pthread_mutex_lock(&mutex_clients);
            /* début section critique */
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].dSC != -1)
                {
                    strcat(list, clients[i].pseudo);
                    strcat(list, " ");
                }
            }
            /* fin section critique */
            pthread_mutex_unlock(&mutex_clients);
            if (send(clients[index_client].dSC, list, strlen(list) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/mp", sizeof(char) * 3) == 0)
        {
            // get the user to send the message to
            // TODO: check if the user exists and send an error message if not
            char *message_copy = malloc(sizeof(char) * (MAX_LENGTH + 1));
            char *private_message = malloc(sizeof(char) * (MAX_LENGTH + 1));
            strcpy(message_copy, msg);
            char *user_pseudo = malloc(sizeof(char) * (PSEUDO_LENGTH + 1));
            char *str_token = strtok(msg, " ");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            str_token = strtok(NULL, " ");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            strcpy(user_pseudo, str_token);
            // get the message
            str_token = strtok(NULL, "\0");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            strcpy(private_message, str_token);
            // get the index of the user
            int index_user = -1;
            pthread_mutex_lock(&mutex_clients);
            /* début section critique */
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (strcmp(clients[i].pseudo, user_pseudo) == 0)
                {
                    index_user = i;
                    break;
                }
            }
            /* fin section critique */
            pthread_mutex_unlock(&mutex_clients);
            // send the private message to the user
            if (index_user != -1)
            {
                if (send(clients[index_user].dSC, private_message, strlen(private_message) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
            }
            return 0;
        }
        return 0;
    }
    return 1;
}

void *client(void *ind)
{
    int index_client = (int)ind; // cast dSc into int
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    char *pseudo = malloc(sizeof(char) * (PSEUDO_LENGTH + 1));
    while (1)
    {
        int error = 0;
        if (recv((clients[index_client]).dSC, pseudo, sizeof(char) * (PSEUDO_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv pseudo \n");
            disconnectClient(index_client);
        }
        pthread_mutex_lock(&mutex_clients);
        /* début section critique */
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (strcmp(pseudo, (clients[i].pseudo)) == 0 || strcmp(pseudo, "server") == 0 || strcmp(pseudo, "Server") == 0 || strcmp(pseudo, "SERVER") == 0 || strcmp(pseudo, "all") == 0 || strcmp(pseudo, "All") == 0 || strcmp(pseudo, "ALL") == 0 || strcmp(pseudo, "broadcast") == 0 || strcmp(pseudo, "Broadcast") == 0 || strcmp(pseudo, "BROADCAST") == 0 || strlen(pseudo) == 0)
            {
                printf("❗ ERROR : pseudo déjà utilisé ou non valide\n");
                error = 1;
                if (send((clients[index_client]).dSC, "ko", strlen("ko") + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send ko \n");
                    disconnectClient(index_client);
                }
            }
        }
        /* fin section critique */
        pthread_mutex_unlock(&mutex_clients);
        if (error == 0)
        {
            strcpy((clients[index_client]).pseudo, pseudo);
            if (send((clients[index_client]).dSC, "ok", strlen("ok") + 1, 0) <= 0)
            {
                printf("❗ ERROR : send ok \n");
                disconnectClient(index_client);
            }
            // pseudo accepted: quit the loop
            break;
        }
    }
    printf("👤 %s connected, with dSC = %d\n", clients[index_client].pseudo, clients[index_client].dSC);
    printf("|---- pseudo -> %s\n", (clients[index_client]).pseudo);

    while (1)
    {
        printf("index_client: %d, dSC: %d\n", index_client, clients[index_client].dSC);
        if (recv(clients[index_client].dSC, msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            disconnectClient(index_client);
        }
        int command_status = CommandsManager(msg, index_client);
        if (command_status == 1) // then it's a message to broadcast
        {
            pthread_mutex_lock(&mutex_clients);
            /* début section critique */
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (index_client != i && clients[i].dSC != -1)
                {
                    if (send(clients[i].dSC, msg, strlen(msg) + 1, 0) <= 0)
                    {
                        printf("❗ ERROR : send \n");
                        disconnectClient(index_client);
                    }
                }
            }
            /* fin section critique */
            pthread_mutex_unlock(&mutex_clients);
        }
        else if (command_status == -1)
        {
            disconnectClient(index_client);
        }
    }
    // TODO: shutdown thread by returning a value (do it also in some if statements)
}
void signalHandler(int sig){
    if (sig == SIGINT){
        for (int i = 0; i < MAX_CLIENTS; i++){
            if (send(clients[i].dSC, "/quit", strlen("/quit") + 1, 0) <= 0){
                printf("❗ ERROR : send \n");
                clients[i].dSC = -1;
                clients[i].pseudo[0] = '\0';
                printf("|--- Client déconnecté\n");
                break;
            }
        }
        printf("👋🏻 Server closed\n");
        exit(0);
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

    // init clients to default values
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].dSC = -1;
        clients[i].pseudo[0] = '\0';
    }

    // init mutex
    if (pthread_mutex_init(&mutex_clients, NULL) != 0)
    {
        printf("❗ ERROR : pthread_mutex_init\n");
        exit(0);
    }

    // init semaphore
    sem_init(&semaphore, 0, MAX_CLIENTS);

    printf("En attente de connexion\n");
    printf("... \n");
    
    signal(SIGINT, signalHandler);
    while (1)
    {
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);
        int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
        // wait for a client to disconnect if the number of clients connected is equal to MAX_CLIENTS
        sem_wait(&semaphore);
        int ind = 0;
        int trouve = -1;
        pthread_mutex_lock(&mutex_clients);
        /* début section critique */
        while (ind <= MAX_CLIENTS && trouve == -1)
        {
            if (clients[ind].dSC == -1)
            {
                clients[ind].dSC = dSC;
                printf("dSC = %d\n", clients[ind].dSC);
                trouve = 0;
            }
            else
            {
                printf("place %d déjà prise\n", ind);
                ind++;
            }
        }
        /* fin section critique */
        pthread_mutex_unlock(&mutex_clients);

        printf("Nouvelle conexion provenant de dSC: %d\n", dSC);

        pthread_t thread;
        pthread_create(&thread, NULL, client, (void *)ind);
        printf("|--- Client Connecté\n");
    }
    // destroy mutex
    pthread_mutex_destroy(&mutex_clients);

    return 0;
}