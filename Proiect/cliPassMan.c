#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;
void logged(int sd)
{
    char lrbuf[10000];
    char lcbuf[500];
    char lraspuns[10000];
    char lcomanda[500];
    while (1) // while folosit cand clientul este logat
    {
        printf("[client logged] Introduceti comanda: ");
        fflush(stdout);
        read(0, lcbuf, sizeof(lcbuf));
        lcbuf[strcspn(lcbuf, "\n")] = '\0';
        strcpy(lcomanda, lcbuf);
        strcpy(lcbuf, "");

        printf("[client logged] Am citit %s\n", lcomanda);

        /* trimiterea mesajului la server */
        if (write(sd, &lcomanda, sizeof(lcomanda)) <= 0)
        {
            perror("[client logged] Eroare la write() spre server.\n");
            return errno;
        }

        /* citirea raspunsului dat de server
           (apel blocant pina cind serverul raspunde) */
        if (read(sd, &lrbuf, sizeof(lrbuf)) < 0)
        {
            perror("[client logged] Eroare la read() de la server.\n");
            return errno;
        }
        else
        {
            strcpy(lraspuns, lrbuf);
            strcpy(lrbuf, "");
        }

        printf("[client logged] Mesajul primit este: \n%s\n", lraspuns);
        if (strstr(lraspuns, "out"))
        {
            break;
        }
        if (strstr(lraspuns, "logged"))
            logged(sd);
        if (strstr(lraspuns, "quit"))
        {
            /* inchidem conexiunea, am terminat */
            close(sd);
            exit(1);
        }
    }
}
int main(int argc, char *argv[])
{
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare
                               // mesajul trimis
    char comanda[500];
    char raspuns[10000];
    char cbuf[500];
    char rbuf[10000];
    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }
    printf(" P A S S W O R D   M A N A G E R \n");
    printf(" -------------------------------- \n");
    printf("___Lista Comenzi___\n");
    printf("quit\nlogin username parola \nlogout\nregister username parola \nfindPass -w website/-c categorie\naddPass categorie website username parola\nupdatePass categorie website username parola\n -- nu se poate face update la username si website, creati entry nou --\ndelPass website username\n");

    while (1)
    {
        /* citirea mesajului */
        printf("[client] Introduceti comanda: ");
        fflush(stdout);
        read(0, cbuf, sizeof(cbuf));
        cbuf[strcspn(cbuf, "\n")] = '\0';
        strcpy(comanda, cbuf);
        strcpy(cbuf, "");
        // scanf("%d",&nr);

        printf("[client] Am citit %s\n", comanda);

        /* trimiterea mesajului la server */
        if (write(sd, &comanda, sizeof(comanda)) <= 0)
        {
            perror("[client] Eroare la write() spre server.\n");
            return errno;
        }

        /* citirea raspunsului dat de server
           (apel blocant pina cind serverul raspunde) */
        if (read(sd, &rbuf, sizeof(rbuf)) < 0)
        {
            perror("[client] Eroare la read() de la server.\n");
            return errno;
        }
        else
        {
            strcpy(raspuns, rbuf);
            strcpy(rbuf, "");
        }
        /* afisam mesajul primit */
        printf("[client] Mesajul primit este: \n%s\n", raspuns);
        if (strstr(raspuns, "logged"))
        {
            logged(sd);
        }
        if (strstr(raspuns, "quit"))
        {
            /* inchidem conexiunea, am terminat */
            close(sd);
            exit(1);
        }
    }
}