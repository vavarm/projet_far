/**
 * @file server.c
 * @author Kylian Thezenas - Valentin Racaud--Minuzzi - Leo d'Amerval
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

// array of channels
channel channels[MAX_CHANNELS];

// mutex that protect the array of channels
pthread_mutex_t mutex_channels;

// semaphore that manages the number of channels
sem_t semaphore_channels;

// TODO: add a CleanThreads thread that will clean a thread when it's disconnected by disconnectClient() by getting a semaphore and then calling pthread_join() to get the message of pthread_exit() and then release the semaphore

/**
 * @brief function that handles the disconnection of a client
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
 * @brief thread function that save the channels in a file (channels.txt) (format: channel_index;channel_name)
*/
void *saveChannels(void *arg)
{
    // open the file
    FILE *file = fopen("channels.txt", "w");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    // write the channels in the file
    pthread_mutex_lock(&mutex_channels);
    /* critical section */
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (channels[i].name[0] != '\0')
        {
            fprintf(file, "%d;%s\n", channels[i].index, channels[i].name);
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_channels);
    // close the file
    fclose(file);
    pthread_exit(NULL);
}

/**
 * @brief thread function that parse the channels.txt file and put the channels in the array of channels
*/
void *parseChannels(void *arg)
{
    // open the file
    FILE *file = fopen("channels.txt", "r");
    if (file == NULL)
    {
        perror("Error opening channels.txt file");
        printf("Creating the file channels.txt\n");
        file = fopen("channels.txt", "w");
        if (file == NULL)
        {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
        fclose(file);
        file = fopen("channels.txt", "r");
    }
    // read the file
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1)
    {
        // get the index and the name of the channel
        char *index = strtok(line, ";");
        if(index == NULL) break;
        char *name = strtok(NULL, "\n");
        if(name == NULL) break;
        // add the channel in the array of channels
        pthread_mutex_lock(&mutex_channels);
        /* critical section */
        channels[atoi(index)].index = atoi(index);
        strcpy(channels[atoi(index)].name, name);
        /* end critical section */
        pthread_mutex_unlock(&mutex_channels);
    }
    // close the file
    fclose(file);
    pthread_exit(NULL);
}

/**
 * @brief function that create a new channel (if it doesn't exist) and save the array of channels in the file
 * @param name the name of the channel
 * @return index of the channel if it was created, -1 if it already exists
 */
int createChannel(char *name)
{
    int index = -1;
    // check if the channel already exists
    bool exists = false;
    pthread_mutex_lock(&mutex_channels);
    /* critical section */
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (strcmp(channels[i].name, name) == 0)
        {
            exists = true;
            break;
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_channels);
    // if the channel doesn't exist
    if (!exists)
    {
        // block the semaphore if the number of channels is equal to MAX_CHANNELS
        sem_wait(&semaphore_channels);
        // create the channel
        pthread_mutex_lock(&mutex_channels);
        /* critical section */
        for (int i = 0; i < MAX_CHANNELS; i++)
        {
            if (channels[i].name[0] == '\0')
            {
                index = i;
                channels[i].index = i;
                strcpy(channels[i].name, name);
                break;
            }
        }
        /* end critical section */
        pthread_mutex_unlock(&mutex_channels);
        // save the channels in the file
        pthread_t thread_saveChannels;
        pthread_create(&thread_saveChannels, NULL, saveChannels, NULL);
    }
    return index;
}

/**
 * @brief thread function that delete a channel (if it exists) and save the array of channels in the file
 * @param name the name of the channel
 * @return index of the channel if the channel has been deleted, -1 if the channel doesn't exist
 * @note the channel with index 0 can't be deleted
*/
int deleteChannel(char *name)
{
    int index = -1;
    // check if the channel exists
    bool exists = false;
    pthread_mutex_lock(&mutex_channels);
    /* critical section */
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (strcmp(channels[i].name, name) == 0)
        {
            exists = true;
            break;
        }
    }
    /* end critical section */
    pthread_mutex_unlock(&mutex_channels);
    // if the channel exists
    if (exists)
    {
        printf("Deleting channel %s\n", name);
        // delete the channel
        pthread_mutex_lock(&mutex_channels);
        /* critical section */
        for (int i = 0; i < MAX_CHANNELS; i++)
        {
            if (strcmp(channels[i].name, name) == 0)
            {
                index = i;
                if (index == 0){
                    pthread_mutex_unlock(&mutex_channels);
                    return 0;
                }
                else{
                    //move all the clients of the channel to the default channel
                    pthread_mutex_lock(&mutex_clients);
                    /* critical section */
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].channel == index)
                        {
                            clients[j].channel = 0;
                            if(send(clients[j].dSC, "You have been moved to the default channel", sizeof(char) * (MAX_LENGTH + 1), 0) == -1){
                                perror("Error sending message");
                                disconnectClient(clients[j].dSC);
                            }
                        }
                    }
                    /* end critical section */
                    pthread_mutex_unlock(&mutex_clients);
                    channels[i].name[0] = '\0';
                    break;
                }
            }
        }
        /* end critical section */
        pthread_mutex_unlock(&mutex_channels);
        // liberate a place in the array of channels
        sem_post(&semaphore_channels);
        // save the channels in the file
        pthread_t thread_saveChannels;
        pthread_create(&thread_saveChannels, NULL, saveChannels, NULL);
    }
    return index;
}

/**
 * @brief thread that returns the dSC of the client that has the pseudo given in parameter
 * 
 * @param pseudo
 * @return int dSC
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
 * @brief function that returns the pseudo of the client that has the dSC given in parameter
 * 
 * @param dSC
 * @return char* pseudo
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
 * @brief function that send a message to all clients connected on the same channel (except the sender)
 * 
 * @param index_sender the index of the client that sent the message
 * @param msg the message to send
 * @param channel the channel where the message will be sent
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
 * @brief function that send a message to all clients connected (except the sender)
 * 
 * @param index_sender the index of the client that sent the message
 * @param msg the message to send
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
 * @param file_args file_info struct that contains the file name, the size and the index of the sender
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
 * @param file_args file_info struct that contains the file name, the size and the index of the sender
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
 * @brief function that manages commands sent by clients
 * @param msg the message sent by the client
 * @param index_client the index of the client that sent the message
 * @return int
 * Postcondition : return 1 if no command, 0 if success, -1 if error
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
            // check if the channel can exists
            int channel = atoi(str_token);
            if (channel < 0 || channel >= MAX_CHANNELS)
            {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel %d does not exist, out of range", channel);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
                return 0;
            }
            // check if the channel exists (has a name)
            if (strcmp(channels[channel].name, "\0") == 0)
            {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel %d does not exist", channel);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
                return 0;
            }
            // change the channel
            clients[index_client].channel = channel;
            char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
            sprintf(str, "You are now in channel '%s' number %d", channels[channel].name, channel);
            if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/getchannels", sizeof(char) * 12) == 0) {
            // send the list of channels to the client (the index and the name)
            char *list = malloc(sizeof(char) * (MAX_LENGTH + 1));
            /* critical section */
            pthread_mutex_lock(&mutex_channels);
            sprintf(list, "Channels: ");
            for(int i = 0; i < MAX_CHANNELS; i++) {
                if (strcmp(channels[i].name, "\0") != 0) {
                    printf("channel %d: %s\n", i, channels[i].name);
                    char *previous = malloc(sizeof(char) * (MAX_LENGTH + 1));
                    strcpy(previous, list);
                    sprintf(list, "%s %d:%s ", previous, i, channels[i].name);
                }
            }
            pthread_mutex_unlock(&mutex_channels);
            /* end critical section */
            printf("list: %s\n", list);
            if (send(clients[index_client].dSC, list, strlen(list) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/getchannel", sizeof(char) * 11) == 0) {
            int channel = clients[index_client].channel;
            char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
            sprintf(str, "You are in channel '%s' with number %d", channels[channel].name, channel);
            if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
            {
                printf("❗ ERROR : send \n");
                return -1;
            }
            return 0;
        }
        else if (strncmp(msg, "/createchannel", sizeof(char) * 14) == 0){
            // get the name of the channel
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
            // call the function to create the channel
            int channel = createChannel(str_token);
            if (channel == -1) {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel '%s' already exists", str_token);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
                return 0;
            }
            else {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel '%s' created with success with number %d", str_token, channel);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
                return 0;
            }
        }
        else if (strncmp(msg, "/deletechannel", sizeof(char) * 14) == 0){
            // get the name of the channel
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
            // call the function to delete the channel
            int channel = deleteChannel(str_token);
            if (channel == -1) {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel '%s' does not exist", str_token);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
            }
            else if (channel == 0){
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "You can't delete the channel 'general'");
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
            }
            else {
                char *str = malloc(sizeof(char) * (MAX_LENGTH + 1));
                sprintf(str, "Channel '%s' deleted with success with number %d", str_token, channel);
                if (send(clients[index_client].dSC, str, strlen(str) + 1, 0) <= 0)
                {
                    printf("❗ ERROR : send \n");
                    return -1;
                }
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
 * @param ind the index of the client
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
 * @param int
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

    // init semaphore of MAX_CLIENTS
    sem_init(&semaphore, 0, MAX_CLIENTS);

    // init semaphore of MAX_CHANNELS
    sem_init(&semaphore_channels, 0, MAX_CHANNELS);

    // init channels to default values
    for (int i = 1; i < MAX_CHANNELS; i++)
    {
        strcpy(channels[i].name, "\0");
        channels[i].index = i;
    }

    // parse the file of channels
    pthread_t thread_parse_channels;
    pthread_create(&thread_parse_channels, NULL, parseChannels, NULL);
    pthread_join(thread_parse_channels, NULL);

    // if the channel general doesn't exist, create it
    if (strlen(channels[0].name) == 0)
    {
        strcpy(channels[0].name, "general");
        channels[0].index = 0;
    }

    // save the channels in the file
    pthread_t thread_save_channels;
    pthread_create(&thread_save_channels, NULL, saveChannels, NULL);
    pthread_join(thread_save_channels, NULL);

    // print the channels
    printf("Liste des channels :\n");
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if(channels[i].name[0] != '\0')
        printf("Channel %d : %s\n", channels[i].index, channels[i].name);
    }

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