#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20
#define CHUNK_SIZE 512

char pseudo[PSEUDO_LENGTH];
int dS;

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        char *msg = "/quit";
        if (send(dS, msg, strlen(msg) + 1, 0) == -1)
        {
            printf("❗ ERROR : send Ctrl+C\n");
            exit(0);
        }
        printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
        exit(0);
    }
}
void *sendFileAsync(void *arg)
{
    printf("sendFileAsync\n");
    char *filename = (char *)arg;
    printf("filename: %s\n", filename);
    char path[100] = "./files_Client/";
    sprintf(path, "%s%s", path, filename);
    printf("path: %s\n", path);
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        printf("❗ ERROR : file not found\n");
        pthread_exit(NULL);
    }
    printf("file found\n");
    //get the size of the file
    fseek(file, 0L, SEEK_END); // seek to the end of the file
    int size = ftell(file);    // get the position, which is the size of the file
    rewind(file);              // seek to the beginning of the file
    printf("size: %d\n", size);
    //send the command to the server
    char *command = malloc(sizeof(char) * (MAX_LENGTH + 1));
    sprintf(command, "/sendfile %s %d", filename, size);
    printf("command: %s\n", command);
    if(send(dS, command, strlen(command) + 1, 0) == -1){
        printf("❗ ERROR : send \n");
        exit(0);
    }
    printf("command sent\n");
    //send the file to the server using a loop and fread to read the file by chunks
    char *buffer = malloc(sizeof(char) * (CHUNK_SIZE + 1));
    int nbBytesRead = 0;
    while ((nbBytesRead = fread(buffer, sizeof(char), CHUNK_SIZE, file)) > 0)
    {
        if (send(dS, buffer, nbBytesRead, 0) == -1)
        {
            printf("❗ ERROR : send \n");
            exit(0);
        }
        sleep(0.1);
    }
    fclose(file);
    printf("file sent\n");
}
void *sendThread(void *dS)
{
    int ds = (int)dS;
    
    while (1)
    {
        char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
        fgets(msg, MAX_LENGTH, stdin);
        if (msg[strlen(msg) - 1] == '\n')
        {
            msg[strlen(msg) - 1] = '\0';
        }

        if (strlen(msg) >= MAX_LENGTH+1){
            printf("❗ ERROR : message trop long \n");
            printf("Taille max message: %d caractères\n", MAX_LENGTH);
            continue;
        }

        if (strncmp(msg, "/listfiles", sizeof(char) * 10) == 0)
        {
            system("ls -1 ./files_Client");
            continue;
        }
        if (strncmp(msg, "/sendfile", sizeof(char) * 9) == 0)
        {
            // TODO: call a thread to send the file
            char *command = strtok(msg, " ");
            if (command == NULL)
            { 
                printf("❗ ERROR : sendfile <filename>\n");
                continue;
            }
            printf("command: %s\n", command);
            char *filename = strtok(NULL, "\0");
            if (filename == NULL)
            {
                printf("❗ ERROR : sendfile <filename>\n");
                continue;
            }
            printf("filename: %s\n", filename);
            printf("sending file...\n");
            pthread_t threadSendFile;
            pthread_create(&threadSendFile, NULL, sendFileAsync, (void *)filename);
            continue;
        }
        if (msg[strlen(msg) - 1] == '\n')
        {
            msg[strlen(msg) - 1] = '\0';
        }
        if (send(ds, msg, strlen(msg) + 1, 0) == -1)
        {
            printf("❗ ERROR : send \n");
            exit(0);
        }
        if (strncmp(msg, "/quit", sizeof(char) * 5) == 0)
        {
            printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
            exit(0);
        }
    }
}

void *receiveThread(void *dS)
{
    int ds = (int)dS;
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    while (1)
    {
        if (recv(ds, msg, sizeof(char) * (MAX_LENGTH + 1), 0) == -1)
        {
            printf("❗ ERROR : recv \n");
            exit(0);
        }
        printf("\33[2K\r");
        printf("\t\t\t ");
        puts(msg);
        if (strncmp(msg, "/quit", sizeof(char) * 5) == 0 && strlen(msg) == 5)
        {
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
    dS = socket(PF_INET, SOCK_STREAM, 0);
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

    signal(SIGINT, signalHandler);

    while (1)
    {
        printf("Entrez votre pseudo : ");
        fgets(pseudo, PSEUDO_LENGTH, stdin);
        if (pseudo[strlen(pseudo) - 1] == '\n')
        {
            pseudo[strlen(pseudo) - 1] = '\0';
        }
        if (send(dS, pseudo, strlen(pseudo) + 1, 0) == -1)
        {
            printf("❗ ERROR : send \n");
            exit(0);
        }
        printf("En attente de la réponse du serveur...\n");
        char reponse[2];
        if (recv(dS, reponse, sizeof(char) * (2 + 1), 0) == -1)
        {
            printf("❗ ERROR : recv \n");
            printf("Connexion interrompue avec le serveur\n");
            printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
            exit(0);
        }
        if (strcmp(reponse, "ok") == 0)
        {
            printf("Pseudo accepté\n");
            break;
        }
        else if (strncmp(reponse, "/quit", sizeof(char) * 5) == 0 && strlen(reponse) == 5)
        {
            printf("\t🛑 --- FIN DE CONNEXION --- 🛑\n\n");
            exit(0);
        }
        else
        {
            printf("Pseudo refusé\n");
        }
    }

    printf("x-----------------------------------x\n");
    printf("Taille max message: %d caractères\n Vous pouvez commencer a discuter \n taper \"/quit\" pour mettre fin a la connexion ou \"/cmd\" pour obtenir la liste des commandes\n", MAX_LENGTH);
    printf("x-----------------------------------x\n");

    pthread_t threadSend;
    pthread_t threadReceive;
    pthread_create(&threadSend, NULL, sendThread, (void *)dS);
    pthread_create(&threadReceive, NULL, receiveThread, (void *)dS);
    pthread_join(threadReceive, NULL);
}