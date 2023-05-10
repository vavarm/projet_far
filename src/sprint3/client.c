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
void *sendThread(void *dS)
{
    int ds = (int)dS;
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    while (1)
    {
        fgets(msg, MAX_LENGTH, stdin);
        if (msg[strlen(msg) - 1] == '\n')
        {
            msg[strlen(msg) - 1] = '\0';
        }
        if (strncmp(msg, "/listfiles", sizeof(char) * 10) == 0)
        {
            system("ls -1 ./files_Client");
            continue;
        }
        ////
        // first: check if the command is /sendfile <filename>
        // second : check if the file exists
        // third : get the size of the file
        // fourth : send '/sendfile <name> <size>' to the server
        // fith : send the file to the server with a loop
        ////
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
            // check if the file exists
            char path[100] = "./files_Client/";
            strcat(path, filename);
            printf("path: %s\n", path);
            FILE *file = fopen(path, "r");
            if (file == NULL)
            {
                printf("❗ ERROR : file not found\n");
                continue;
            }
            printf("file found\n");
            printf("filename: %s\n", filename);
            // get the size of the file
            fseek(file, 0L, SEEK_END); // seek to the end of the file
            int size = ftell(file);    // get the position, which is the size of the file
            rewind(file);              // seek to the beginning of the file
            printf("size: %d\n", size);
            printf("filename: %s\n", filename);
            // send '/sendfile <name> <size>' to the server
            sprintf(msg, "/sendfile %s %d", filename, size);
            printf("msg: %s\n", msg);
            if (send(ds, msg, strlen(msg) + 1, 0) == -1)
            {
                printf("❗ ERROR : send \n");
                exit(0);
            }
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