#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_CLIENTS 2
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20
#define PATH "./files_Server"
#define CHUNK_SIZE 512


int dsFiles;
volatile sig_atomic_t keepRunning = true;

// structure of files
typedef struct
{
    char filename[MAX_LENGTH];
    int size;
    int index_sender;
} file_info;

// structure of users
typedef struct
{
    int dSC;
    int dSF;
    char pseudo[PSEUDO_LENGTH];
} clientConnecte;

// array of clients
clientConnecte clients[MAX_CLIENTS];

// mutex that protect the array of clients
pthread_mutex_t mutex_clients;

// semaphore that manages the number of clients connected
sem_t semaphore;

// TODO: add a CleanThreads thread that will clean a thread when it's disconnected by disconnectClient() by getting a semaphore and then calling pthread_join() to get the message of pthread_exit() and then release the semaphore

// TODO: add a dissconnect status and use it in pthread_exit()
void disconnectClient(int index_client)
{
    printf("👤 %s disconnected, with dSC = %d\n", clients[index_client].pseudo, clients[index_client].dSC);
    // put the default values in the array
    clients[index_client].dSC = -1;
    clients[index_client].dSF = -1;
    clients[index_client].pseudo[0] = '\0';
    // release the semaphore to accept new clients
    sem_post(&semaphore);
    // kill the thread
    pthread_exit(NULL);
}

int getDSCByPseudo(char *pseudo)
{
    int dSC = -1;
    pthread_mutex_lock(&mutex_clients);
    /* début section critique */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (strcmp(clients[i].pseudo, pseudo) == 0)
        {
            dSC = clients[i].dSC;
            break;
        }
    }
    /* fin section critique */
    pthread_mutex_unlock(&mutex_clients);
    return dSC;
}

char *getPseudoByDSC(int dSC)
{
    char *pseudo = malloc(sizeof(char) * (PSEUDO_LENGTH + 1));
    pseudo[0] = '\0';
    pthread_mutex_lock(&mutex_clients);
    /* début section critique */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].dSC == dSC)
        {
            strcpy(pseudo, clients[i].pseudo);
            break;
        }
    }
    /* fin section critique */
    pthread_mutex_unlock(&mutex_clients);
    return pseudo;
}

void broadcastMessage(int index_sender, char *msg)
{
    pthread_mutex_lock(&mutex_clients);
    /* début section critique */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (index_sender != i && clients[i].dSC != -1)
        {
            if (send(clients[i].dSC, msg, strlen(msg) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                disconnectClient(index_sender);
            }
        }
    }
    /* fin section critique */
    pthread_mutex_unlock(&mutex_clients);
}

void *receiveFileAsync(void* file_args){
    file_info *file = (file_info *) file_args;

    printf("file size: %d\n", file->size);

    struct sockaddr_in adFiles;
    socklen_t lgFiles = sizeof(struct sockaddr_in);

    FILE *fp;
    char* path = malloc(sizeof(char) * (MAX_LENGTH + 1));
    int size_received = 0;
    char *block = malloc(CHUNK_SIZE);
    sprintf(path, "%s/%s", PATH, file->filename);
    fp = fopen(path, "a");
    int received = 0;
    while(size_received < file->size){
        if (  (received = recv(clients[file->index_sender].dSF, block, CHUNK_SIZE, 0) )<= 0)
        {
            printf("❗ ERROR : recv \n");
            disconnectClient(file->index_sender);
            pthread_exit(NULL);
        }
        size_received += received;
                
        fwrite(block, 1, received, fp);
        printf("received: %d\n", received); 
        printf("size_received: %d\n", size_received);             
    }
    fclose(fp);
    printf("file received\n");
}

void *sendFileAsync(void *file_args)
{
    file_info *file = (file_info *)file_args;

    struct sockaddr_in adFiles;
    socklen_t lgFiles = sizeof(struct sockaddr_in);

    FILE *fp;
    char *path = malloc(sizeof(char) * (MAX_LENGTH + 1));
    int size_sent = 0;
    char *block = malloc(CHUNK_SIZE);
    sprintf(path, "%s/%s", PATH, file->filename);
    fp = fopen(path, "r");
    // verify if the file exists
    if (fp == NULL)
    {
        printf("❗ ERROR : fopen\n");
        send(clients[file->index_sender].dSF, -1, sizeof(int), 0);
        pthread_exit(NULL);
    }
    printf("file found\n");
    // get the size of the file
    fseek(fp, 0L, SEEK_END);
    file->size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    printf("file size: %d\n", file->size);
    // send the size of the file
    if (send(clients[file->index_sender].dSF, &(file->size), sizeof(int), 0) <= 0)
    {
        printf("❗ ERROR : send \n");
        disconnectClient(file->index_sender);
        pthread_exit(NULL);
    }

    int sent = 0;
    while (size_sent < file->size)
    {
        if ((sent = fread(block, 1, CHUNK_SIZE, fp)) <= 0)
        {
            printf("❗ ERROR : fread \n");
            disconnectClient(file->index_sender);
            pthread_exit(NULL);
        }
        size_sent += sent;
        if (send(clients[file->index_sender].dSF, block, sent, 0) <= 0)
        {
            printf("❗ ERROR : send \n");
            disconnectClient(file->index_sender);
            pthread_exit(NULL);
        }
        printf("sent: %d\n", sent);
        printf("size_sent: %d\n", size_sent);
    }
    fclose(fp);
    printf("file sent\n");
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
        else if (strncmp(msg, "/rm", sizeof(char) * 3) == 0){
            system("rm -r ./files_Server");
            system("mkdir ./files_Server");
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
            // get the dSC of the user
            int dSC_receiver = getDSCByPseudo(user_pseudo);
            printf("dSC_receiver: %d\n", dSC_receiver);
            if (dSC_receiver == -1)
            {
                if (send(clients[index_client].dSC, "❗ ERROR : user not found", strlen("❗ ERROR : user not found") + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
            }
            else
            {
                if (send(dSC_receiver, private_message, strlen(private_message) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
            }
            return 0;
        }else if (strncmp(msg, "/dir", sizeof(char) * 4) == 0){
                struct dirent *dir;
                DIR *d = opendir("./files_Server"); 
                if (d)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                        char* str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                        strcpy(str, dir->d_name);
                        strcat(str, "\n");
                        if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                        {
                            printf("❗ ERROR : send /dir\n");
                            return -1;
                        }
                        sleep(0.1);
                    }
                    closedir(d);
                }
                return 0;
        } else if (strncmp(msg, "/sendfile", sizeof(char)* 9) == 0){
            
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
            char * filename = str_token;
            str_token = strtok(NULL, " ");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            int size = atoi(str_token);

            //THREAD
            //create
            pthread_t thread;
            //set args
            file_info *args = malloc(sizeof(file_info)) ;
            strcpy(args->filename,filename);
            args->size = size;
            args->index_sender = index_client;

            //create thread
            pthread_create(&thread, NULL, receiveFileAsync, (void *)args);
        }else if (strncmp(msg, "/receivefile", sizeof(char)* 12) == 0){
            printf("receivefile command: %s\n", msg);
            char *str_token = strtok(msg, " ");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            str_token = strtok(NULL, "\0");
            if (str_token == NULL)
            {
                printf("❗ ERROR : malloc \n");
                return 0;
            }
            char * filename = str_token;
            
            //THREAD
            //create
            pthread_t thread;
            //set args
            file_info *args = malloc(sizeof(file_info)) ;
            strcpy(args->filename,filename);
            args->index_sender = index_client;

            //create thread
            pthread_create(&thread, NULL, sendFileAsync, (void *)args);

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
    while (keepRunning)
    {
        int error = 0;
        if (recv((clients[index_client]).dSC, pseudo, sizeof(char) * (PSEUDO_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv pseudo \n");
            disconnectClient(index_client);
        }
        printf("|---- pseudo -> %s\n", pseudo);
        printf("|---- taille pseudo -> %ld\n", strlen(pseudo));
        pthread_mutex_lock(&mutex_clients);
        /* début section critique */
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (strcmp(pseudo, (clients[i].pseudo)) == 0 || strcmp(pseudo, "server") == 0 || strcmp(pseudo, "Server") == 0 || strcmp(pseudo, "SERVER") == 0 || strcmp(pseudo, "all") == 0 || strcmp(pseudo, "All") == 0 || strcmp(pseudo, "ALL") == 0 || strcmp(pseudo, "broadcast") == 0 || strcmp(pseudo, "Broadcast") == 0 || strcmp(pseudo, "BROADCAST") == 0 || strcmp(pseudo, "me") == 0 || strcmp(pseudo, "ME") == 0 || strlen(pseudo) == 0)
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

    while (keepRunning)
    {
        //printf("index_client: %d, dSC: %d\n", index_client, clients[index_client].dSC);
        if (recv(clients[index_client].dSC, msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            disconnectClient(index_client);
        }
        int command_status = CommandsManager(msg, index_client);
        if (command_status == 1) // then it's a message to broadcast
        {
            broadcastMessage(index_client, msg);
        }
        else if (command_status == -1)
        {
            disconnectClient(index_client);
        }
    }
    // TODO: shutdown thread by returning a value (do it also in some if statements)
    pthread_exit(NULL);
}
void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        keepRunning = false;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (send(clients[i].dSC, "/quit", strlen("/quit") + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                clients[i].dSC = -1;
                clients[i].pseudo[0] = '\0';
                printf("|--- Client déconnecté\n");
                break;
            }
        }
        // TODO: unbind the socket
        printf("👋🏻 Server closed\n");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("❗ ERROR Argument: nombre d'argument invalide (port) (port fichier)\n");
        return -1;
    }

    printf("Début programme\n");

    if (mkdir(PATH, 0755) == -1)
    {
        printf("❗ ERROR : mkdir -- dossier serveur non créé\n");
        printf(" --- peut-être déjà créé\n");
    }

    // Socket messages
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

    // Socket files
    dsFiles = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket dédié aux fichiers Créé\n");

    struct sockaddr_in adFiles;
    adFiles.sin_family = AF_INET;
    adFiles.sin_addr.s_addr = INADDR_ANY;
    adFiles.sin_port = htons(atoi(argv[2]));
    if (bind(dsFiles, (struct sockaddr *)&adFiles, sizeof(adFiles)) == -1)
    {
        printf("❗ ERROR : bind (le port est peut être déjà utilisé)\n");
        exit(0);
    }
    printf("Socket dédié aux fichiers Nommé\n");

    int lFiles = listen(dsFiles, 7);
    if (lFiles == -1)
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
        if(dSC == -1){
            printf("❗ ERROR : accept\n");
            exit(0);
        }
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
                // listen to the file socket
                int dSF = accept(dsFiles, (struct sockaddr *)&aC, &lg);
                if(dSF == -1){
                    printf("❗ ERROR : accept\n");
                    exit(0);
                }
                clients[ind].dSF = dSF;
                printf("dSF = %d\n", clients[ind].dSF);
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