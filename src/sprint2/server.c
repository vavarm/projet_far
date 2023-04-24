#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20

typedef struct
{
    int dSC;
    char pseudo[PSEUDO_LENGTH];
} clientConnecte;

int ind = 0;
clientConnecte clients[MAX_CLIENTS];

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
        if (strncmp(msg, "/quit", sizeof(char) * 5) == 0)
        {
            return -1;
        }
        else if (strncmp(msg, "/list", sizeof(char) * 5) == 0)
        {
            // return all the users to the client
            char *list = malloc(sizeof(char) * (MAX_LENGTH * (PSEUDO_LENGTH + 2) + 1));
            strcat(list, clients[index_client].pseudo);
            for (int i = 0; i < ind; i++)
            {
                if (clients[i].dSC != -1 && strcmp(clients[i].pseudo, clients[index_client].pseudo) != 0)
                {
                    strcat(list, ", ");
                    strcat(list, clients[i].pseudo);
                }
            }
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
            for (int i = 0; i < ind; i++)
            {
                if (strcmp(clients[i].pseudo, user_pseudo) == 0)
                {
                    index_user = i;
                    break;
                }
            }
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

void *client(void *ind_client)
{
    int index_client = (int)ind_client; // cast dSc into int
    char *msg = malloc(sizeof(char) * (MAX_LENGTH + 1));
    if (recv((clients[index_client]).dSC, (clients[index_client]).pseudo, sizeof(char) * (PSEUDO_LENGTH + 1), 0) <= 0)
    {
        printf("❗ ERROR : recv pseudo \n");
        exit(0); // TODO : remplace par une remise a zero du client + break
    }
    printf("|---- pseudo -> %s\n", (clients[index_client]).pseudo);

    while (1)
    {
        if (recv(clients[index_client].dSC, msg, sizeof(char) * (MAX_LENGTH + 1), 0) <= 0)
        {
            printf("❗ ERROR : recv \n");
            exit(0); // TODO :remplacer par une remise a zero du client + break
        }

        int command_status = CommandsManager(msg, index_client);

        if (command_status == 1)
        {
            // broadcast message
            for (int i = 0; i < ind; i++)
            {
                if (index_client != i)
                {
                    // printf("%d\n", clients[i]);
                    if (send(clients[i].dSC, msg, strlen(msg) + 1, 0) <= 0)
                    {
                        printf("❗ ERROR : send \n");
                        exit(0); // TODO :remplacer par une remise a zero du client + break
                    }
                }
            }
        }
        else if (command_status == -1)
        {
            clients[index_client].dSC = -1;
            printf("|--- Client déconnecté\n");
            break;
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

        // TODO: on accept create a thread (clientBroadcast)
        pthread_t thread;
        pthread_create(&thread, NULL, client, (void *)ind);
        printf("|--- Client Connecté\n");
        ind++;
    }
}