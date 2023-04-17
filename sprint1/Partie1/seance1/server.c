#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 100

int clients[2];

int main(int argc, char *argv[])
{
    /*
        On setup la socket TCP
        while(1){
            si client 1 est NULL alors: On attend la connexion du client 1
            si client 2 est NULL alors: On attend la connexion du client 2
            while(1){
                on:
                    reception du message du client 1
                    si: message = "fin"
                    then: on deco le client 1, on le retire du tableau, on quitte la boucle while (break)
                then:
                    On écrit le message au client 2
                on:
                    reception du message du client 2
                    si: message = "fin"
                    then: on deco le client 2, on le retire du tableau, on quitte la boucle while (break)
                then:
                    on écrit le message au client 1
            }
        }
    */

    if (argc != 2)
    {
        printf("ERROR Argument: nombre d'argument invalide (port)\n");
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
        printf("ERROR : bind, le port est peut être déjà utilisé\n");
        exit(0);
    }
    printf("Socket Nommé\n");

    int l = listen(dS, 7);
    if (l == -1)
    {
        printf("ERROR : listen\n");
        exit(0);
    }

    while (1)
    {
        printf("en attente de connexion\n");
        if (clients[0] == NULL)
        {
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            clients[0] = dSC;
            printf("Client 1 Connecté\n");
        }
        if (clients[1] == NULL)
        {
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            clients[1] = dSC;
            printf("Client 2 Connecté\n");
        }

        printf("x-----------------------------------x\n");

        char *msg = malloc(sizeof(char) * MAX_LENGTH + 1);

        while (1)
        {
            // reception du message du client 1 et envoie au client 2
            if (recv(clients[0], msg, sizeof(char) * (MAX_LENGTH + 1), 0) == 0)
            {
                printf("ERROR : recv \n");
                send(clients[1], "fin", sizeof(char) * (MAX_LENGTH + 1), 0);
                clients[0] = NULL;
                clients[1] = NULL;
                break;
            }
            
            printf("| Message reçu : \t \t \t \t%s\n", msg);
            if (send(clients[1], msg, sizeof(char) * (MAX_LENGTH + 1), 0) == 0)
            {
                printf("ERROR : send \n");
                send(clients[0], "fin", sizeof(char) * (MAX_LENGTH + 1), 0);
                clients[0] = NULL;
                clients[1] = NULL;
                printf("client 2 déconnecté \n");
                break;
            }
            printf("| Message envoye : \t \t \t \t%s\n", msg);
            printf("X-----------------------------------X\n");
            // si le message est "fin", on reset le tableau client
            if (strcmp(msg, "fin") == 0)
            {
                clients[0] = NULL;
                clients[1] = NULL;
                break;
            }
            // reception du message du client 2 et envoie au client 1
            if (recv(clients[1], msg, sizeof(char) * (MAX_LENGTH + 1), 0) == 0)
            {
                printf("ERROR : recv \n");
                send(clients[0], "fin", sizeof(char) * (MAX_LENGTH + 1), 0);
                clients[0] = NULL;
                clients[1] = NULL;
                break;
            }
            
            printf("\n");
            printf("| Message reçu : \t \t \t \t%s\n", msg);
            if (send(clients[0], msg, sizeof(char) * (MAX_LENGTH + 1), 0) == 0)
            {
                printf("ERROR : send \n");
                send(clients[1], "fin", sizeof(char) * (MAX_LENGTH + 1), 0);
                clients[0] = NULL;
                clients[1] = NULL;
                printf("client 1 déconnecté \n");
                break;
            }
            printf("| Message envoye : \t \t \t \t%s\n", msg);
            printf("X-----------------------------------X\n");

            // si le message est "fin", on reset le tableau client
            if (strncmp(msg, "fin", 3) == 0)
            {
                clients[0] = NULL;
                clients[1] = NULL;
                break;
            }
        }
    }
}
/*
    recv(dSC, msg, 50, 0);

    printf("| Message reçu : \t \t \t \t%s\n", msg);
    printf("|----%d octet recu \n", nb_octet);
    printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);

    if (strncmp(msg, "shutdown", 8) == 0){
        x = 11;
        s = send(dSC, &x, sizeof(int), 0) ;
        if (s == -1){
            printf("ERROR : send\n");
            exit(0);
        }
        shutdown(dSC,2) ;
        shutdown(dS,2) ;
        printf("Fin du programme \n");
        exit(0);
    }
    s = send(dSC, &x, sizeof(int), 0) ;
    if (s == -1){
        printf("ERROR : send\n");
        exit(0);
        }
    printf("X-----------------------------------X\n");

    while (strncmp(msg, "shutdown", 8) != 0){
        nb_octet = recv(dSC, msg, 50, 0) ;

        if (strncmp(msg, "shutdown", 8) == 0){
        break;
        }
        nb_octet_recu += nb_octet;
        printf("| Message reçu : \t \t \t \t %s\n", msg) ;
        printf("|----%d octet recu \n", nb_octet);
        printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);

        int s = send(dSC, &x, sizeof(int), 0) ;
        if (s == -1){
        printf("ERROR : send\n");
        exit(0);
        }
        printf("X-----------------------------------X\n");
    }

    printf("Message reçu : %s\n", msg) ;
    x = 11 ;
    s = send(dSC, &x, sizeof(int), 0) ;
    if (s == -1){
        printf("ERROR : send\n");
        exit(0);
        }
}
  shutdown(dSC,2) ;
  shutdown(dS,2) ;
  printf("\t \t 🛑 Fin du programme 🛑\n");
  */
