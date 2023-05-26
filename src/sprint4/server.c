/**
 * @file server.c
 * @author Kylian Thezenas - Valentin Raccaud--Minuzzi -Leo d'Amerval
 * @brief file containing the server functions for the project FAR of the 6th semester of the engineering cycle of Polytech Montpellier
 * @date 2023-05-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */

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

#include "header.h"

int dsFiles;
volatile sig_atomic_t keepRunning = true;

// array of clients
clientConnecte clients[MAX_CLIENTS];

// mutex that protect the array of clients
pthread_mutex_t mutex_clients;

// semaphore that manages the number of clients connected
sem_t semaphore;

// TODO: add a CleanThreads thread that will clean a thread when it's disconnected by disconnectClient() by getting a semaphore and then calling pthread_join() to get the message of pthread_exit() and then release the semaphore

/**
 * @brief add a dissconnect status and use it in pthread_exit()
 * 
 * @param index_client 
 */
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

/**
 * @brief thread that will handle the connection of a client
 * 
 * @param pseudo 
 * @return int (dSC)
 */
int getDSCByPseudo(char *pseudo)
{
    int dSC = -1;
    pthread_mutex_lock(&mutex_clients);
    /* critical section */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (strcmp(clients[i].pseudo, pseudo) == 0)
        {
            dSC = clients[i].dSC;
            break;
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_clients);
    return dSC;
}

/**
 * @brief Get the Pseudo By DSC object
 * 
 * @param dSC 
 * @return char* 
 */
char *getPseudoByDSC(int dSC)
{
    char *pseudo = malloc(sizeof(char) * (PSEUDO_LENGTH + 1));
    pseudo[0] = '\0';
    pthread_mutex_lock(&mutex_clients);
    /* critical section */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].dSC == dSC)
        {
            strcpy(pseudo, clients[i].pseudo);
            break;
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_clients);
    return pseudo;
}

/**
 * @brief thread that will handle the connection of a client
 * 
 * @param index_sender 
 * @param msg 
 * @param channel 
 */
void sendMessageInChannel(int index_sender, char *msg, int channel)
{
    char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
    strcpy(str, msg);
    //get timestamp
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestamp[20];
    if(tm.tm_hour < 10 && tm.tm_min < 10)
        sprintf(timestamp, "[0%d:0%d]", tm.tm_hour, tm.tm_min);
    else if(tm.tm_hour < 10)
        sprintf(timestamp, "[0%d:%d]", tm.tm_hour, tm.tm_min);
    else if(tm.tm_min < 10)
        sprintf(timestamp, "[%d:0%d]", tm.tm_hour, tm.tm_min);
    else
        sprintf(timestamp, "[%d:%d]", tm.tm_hour, tm.tm_min);
    sprintf(msg, "%s %s : %s", timestamp, clients[index_sender].pseudo, str);
    pthread_mutex_lock(&mutex_clients);
    /* critical section */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (index_sender != i && clients[i].dSC != -1 && clients[i].channel == channel)
        {
            if (send(clients[i].dSC, msg, strlen(msg) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                disconnectClient(index_sender);
            }
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_clients);
}


/**
 * @brief send message to all clients except the sender
 * 
 * @param index_sender 
 * @param msg 
 */
void broadcastMessage(int index_sender, char *msg)
{
    char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
    strcpy(str, msg);
    //get timestamp
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestamp[20];
    if(tm.tm_hour < 10 && tm.tm_min < 10)
        sprintf(timestamp, "[0%d:0%d]", tm.tm_hour, tm.tm_min);
    else if(tm.tm_hour < 10)
        sprintf(timestamp, "[0%d:%d]", tm.tm_hour, tm.tm_min);
    else if(tm.tm_min < 10)
        sprintf(timestamp, "[%d:0%d]", tm.tm_hour, tm.tm_min);
    else
        sprintf(timestamp, "[%d:%d]", tm.tm_hour, tm.tm_min);
    sprintf(msg, "%s %s : %s", timestamp, clients[index_sender].pseudo, str);
    pthread_mutex_lock(&mutex_clients);
    /* critical section */
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
    /* end critical section */
    pthread_mutex_unlock(&mutex_clients);
}

/**
 * @brief thread to receive file from a client
 * 
 * @param file_args 
 * @return void* 
 */
void *receiveFileAsync(void* file_args){
    file_info *file = (file_info *) file_args;

    printf("file size: %d\n", file->size);

    struct sockaddr_in adFiles;
    socklen_t lgFiles = sizeof(struct sockaddr_in);

    FILE *fp;
    char* path = malloc(sizeof(char) * (MAX_LENGTH + 1));
    int size_received = 0;
    char *block = malloc(CHUNK_SIZE);
    sprintf(path, "%s/%s", PATH_SERVER_FILES, file->filename);
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

/**
 * @brief thread to send file to a client
 * 
 * @param file_args 
 * @return void* 
 */
void *sendFileAsync(void *file_args)
{
    file_info *file = (file_info *)file_args;

    struct sockaddr_in adFiles;
    socklen_t lgFiles = sizeof(struct sockaddr_in);

    FILE *fp;
    char *path = malloc(sizeof(char) * (MAX_LENGTH + 1));
    int size_sent = 0;
    char *block = malloc(CHUNK_SIZE);
    sprintf(path, "%s/%s", PATH_SERVER_FILES, file->filename);
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

/**
 * @brief function that manage commands send by clients
 * Commandes:
    /quit : quit server : return -1
    if success on other command : return 0
    if error : return -1
    if no command : return 1
 * @param msg index_client 
 * @return void* 
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
        else if (strncmp(msg, "/listallusers", sizeof(char) * 13) == 0)
        {
            // return all the users to the client
            char *list = malloc(sizeof(char) * (MAX_LENGTH + 1));
            pthread_mutex_lock(&mutex_clients);
            /* critical section */
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].dSC != -1)
                {
                    strcat(list, clients[i].pseudo);
                    strcat(list, " ");
                }
            }
            /* end critical section */
            pthread_mutex_unlock(&mutex_clients);
            if (send(clients[index_client].dSC, list, strlen(list) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/listusers", sizeof(char)*12) == 0){
            // return all the users in the channel to the client
            char *list = malloc(sizeof(char) * (MAX_LENGTH + 1));
            pthread_mutex_lock(&mutex_clients);

            sprintf(list, "Users in channel %d: ", clients[index_client].channel);
            /* critical section */
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].dSC != -1 && clients[i].channel == clients[index_client].channel)
                {
                    strcat(list, clients[i].pseudo);
                    strcat(list, " ");
                }
            }
            /* end critical section */
            pthread_mutex_unlock(&mutex_clients);
            if (send(clients[index_client].dSC, list, strlen(list) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/changechannel", sizeof(char) * 14) == 0){
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
            int channel = atoi(str_token);
            clients[index_client].channel = channel;
            return 0;
        }
        else if (strncmp(msg, "/getchannel", sizeof(char) * 11) == 0) {
            int channel = clients[index_client].channel;
            char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
            sprintf(str, "%d", channel);
            if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/all", sizeof(char) * 4) == 0){
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
            broadcastMessage(index_client, str_token);
            return 0;
        }
        else if (strncmp(msg, "/mp", sizeof(char) * 3) == 0)
        {
            // get the user to send the message to
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
/**
 * @brief thread function for the client connection
 * collect the pseudo and the dSC of the client
 * 
 * @param ind 
 * @return void* 
 */
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
        /* critical section */
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
        /* end critical section */
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
        if (recv(clients[index_client].dSC, msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            disconnectClient(index_client);
        }
        int command_status = CommandsManager(msg, index_client);
        if (command_status == 1) // then it's a message to broadcast
        {
            sendMessageInChannel(index_client, msg, clients[index_client].channel);
        }
        else if (command_status == -1)
        {
            disconnectClient(index_client);
        }
    }
    pthread_exit(NULL);
}

/**
 * @brief disconnect the server if the signal CTRL+C is received
 * 
 * @param index_client 
 */
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
        printf("👋🏻 Server closed\n");
        exit(0);
    }
}

/**
 * @brief main function
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("❗ ERROR Argument: nombre d'argument invalide (port) (port fichier)\n");
        return -1;
    }

    printf("Début programme\n");

    if (mkdir(PATH_SERVER_FILES, 0755) == -1)
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
        /* critical section */
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
                clients[ind].channel = 0;
                trouve = 0;
            }
            else
            {
                printf("place %d déjà prise\n", ind);
                ind++;
            }
        }
        /* end critical section */
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