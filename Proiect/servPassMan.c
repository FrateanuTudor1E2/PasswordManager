#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <sqlite3.h>
/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

// sectiune BD
sqlite3 *db;
void openDB()
{
    int rc = sqlite3_open("PassManDB.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Nu s-a putut deschide baza de date", sqlite3_errmsg(db));
        exit(1);
    }
}

void closeDB()
{
    sqlite3_close(db);
}

void addUserDB(char username[], char parola[], char raspuns[])
{
    openDB();

    // verificam daca exista un username identic
    char query[100];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM users WHERE MASTusername = '%s'", username);

    sqlite3_stmt *checkStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &checkStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la creerea statement-ului: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "nu s-a putut inregistra");
        closeDB();
        return;
    }

    rc = sqlite3_step(checkStmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "eroare la verificarea username-ului: &s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "nu s-a putut inregistra");
        closeDB();
        return;
    }

    int userCount = sqlite3_column_int(checkStmt, 0);
    sqlite3_finalize(checkStmt);

    if (userCount > 0)
    {
        strcpy(raspuns, "username-ul este deja folosit\n");
        closeDB();
        return;
    }

    // inseram datele in tabela
    char *sql = "INSERT INTO users (MASTusername, MASTparola) VALUES (?,?)";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la verificarea username-ului: &s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "nu s-a putut inregistra");
        closeDB();
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parola, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "executie esuata: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "nu s-a putut inregistra");
        sqlite3_finalize(stmt);
        closeDB();
        return;
    }

    strcpy(raspuns, "registered");
    sqlite3_finalize(stmt);
    closeDB();
}

void checkCredentials(char MASTERusername[], char MASTERparola[], char toKeep[], char raspuns[])
{
    openDB();

    // verificam daca exista userul inregistrat in tabela 'users' + verificam si daca parola introdusa este corecta
    char query[100];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM users WHERE MASTusername = '%s' AND MASTparola = '%s'", MASTERusername, MASTERparola);

    sqlite3_stmt *checkUserPassStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &checkUserPassStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la crearea statement-ului: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la autentificare");
        closeDB();
        return;
    }

    rc = sqlite3_step(checkUserPassStmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "Eroare la verificare user: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la autentificare");
        sqlite3_finalize(checkUserPassStmt);
        closeDB();
        return;
    }

    int userpassCount = sqlite3_column_int(checkUserPassStmt, 0);
    sqlite3_finalize(checkUserPassStmt);

    if (userpassCount == 0)
    {
        strcpy(raspuns, "user sau parola gresite");
        closeDB();
        return;
    }

    strcpy(raspuns, "logged");
    strcpy(toKeep, MASTERusername);
    closeDB();
}

void addPassDB(char toKeep[], char categorie[], char website[], char username[], char parola[], char raspuns[])
{
    openDB();

    // verificam daca exista deja o inregistrare cu acelasi website si username
    char query[200];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM listaparole WHERE MASTusername = ? AND website = ? AND username = ?");

    sqlite3_stmt *checkStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &checkStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la creare statement: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la inregistrare");
        closeDB();
        return;
    }

    sqlite3_bind_text(checkStmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(checkStmt, 2, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(checkStmt, 3, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(checkStmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "eroare la cautare: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la inregistrare");
        sqlite3_finalize(checkStmt);
        closeDB();
        return;
    }

    int recordCount = sqlite3_column_int(checkStmt, 0);
    sqlite3_finalize(checkStmt);

    if (recordCount > 0)
    {
        strcpy(raspuns, "exista deja pentru acest website un username identic, nu se poate inregistra");
        closeDB();
        return;
    }

    // inseram informatiile in tabela
    char *sql = "INSERT INTO listaparole (MASTusername, categorie, website, username, parola) VALUES (?,?,?,?,?)";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la inregistrare: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la inregistrare");
        closeDB();
        return;
    }

    sqlite3_bind_text(stmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, categorie, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, parola, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "executie esuata: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la inregistrare");
        sqlite3_finalize(stmt);
        closeDB();
        return;
    }

    strcpy(raspuns, "Parola adaugata cu succes");
    sqlite3_finalize(stmt);
    closeDB();
}

void updatePassDB(char toKeep[], char categorie[], char website[], char username[], char parola[], char raspuns[])
{
    openDB();

    // asiguram ca website-ul si username-ul nu pot fi modificate
    char query[200];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM listaparole WHERE MASTusername = ? AND website = ? AND username = ?");

    sqlite3_stmt *checkStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &checkStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la crearea statement %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la actualizare");
        closeDB();
        return;
    }

    sqlite3_bind_text(checkStmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(checkStmt, 2, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(checkStmt, 3, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(checkStmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "eroare la verificare: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la actualizare");
        sqlite3_finalize(checkStmt);
        closeDB();
        return;
    }

    int recordCount = sqlite3_column_int(checkStmt, 0);
    sqlite3_finalize(checkStmt);

    if (recordCount == 0)
    {
        strcpy(raspuns, "website-ul si username-ul nu pot fi modificate, folositi addPass");
        closeDB();
        return;
    }

    // actualizam doar campurile categorie si parola
    snprintf(query, sizeof(query), "UPDATE listaparole SET categorie = ?, parola = ? WHERE MASTusername = ? AND website = ? AND username = ?");

    sqlite3_stmt *updateStmt;
    rc = sqlite3_prepare_v2(db, query, -1, &updateStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la crearea instructiunii: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la actualizare");
        closeDB();
        return;
    }

    sqlite3_bind_text(updateStmt, 1, categorie, -1, SQLITE_STATIC);
    sqlite3_bind_text(updateStmt, 2, parola, -1, SQLITE_STATIC);
    sqlite3_bind_text(updateStmt, 3, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(updateStmt, 4, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(updateStmt, 5, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(updateStmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "eroare la actualizare: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la actualizare");
        sqlite3_finalize(updateStmt);
        closeDB();
        return;
    }

    strcpy(raspuns, "campuri actualizate cu succes");
    sqlite3_finalize(updateStmt);
    closeDB();
}

void delPassBD(char toKeep[], char website[], char username[], char raspuns[])
{
    openDB();
    char query[200];
    // stergem randul din tabela
    snprintf(query, sizeof(query), "DELETE FROM listaparole WHERE MASTusername = ? AND website = ? AND username = ?");

    sqlite3_stmt *deleteStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &deleteStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la creare statement %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la stergerea informatiilor");
        closeDB();
        return;
    }

    sqlite3_bind_text(deleteStmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(deleteStmt, 2, website, -1, SQLITE_STATIC);
    sqlite3_bind_text(deleteStmt, 3, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(deleteStmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "eroare la stergerea informatiilor: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la stergerea informatiilor");
        sqlite3_finalize(deleteStmt);
        closeDB();
        return;
    }

    int changes = sqlite3_changes(db);

    if (changes == 0)
    {
        strcpy(raspuns, "informatii inexistente");
    }
    else
    {

        strcpy(raspuns, "informatii sterse cu succes");
    }

    sqlite3_finalize(deleteStmt);
    closeDB();
}

void findByCatBD(char cat[], char raspuns[], char toKeep[])
{
    openDB();

    // cautam dupa categorie
    char query[200];
    snprintf(query, sizeof(query), "SELECT website, username, parola FROM listaparole WHERE MASTusername = ? AND categorie = ?");

    sqlite3_stmt *selectStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &selectStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la creare statement: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la cautare");
        closeDB();
        return;
    }

    sqlite3_bind_text(selectStmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(selectStmt, 2, cat, -1, SQLITE_STATIC);

    strcpy(raspuns, "");
    // concatenam randurile gasite
    while (sqlite3_step(selectStmt) == SQLITE_ROW)
    {
        char website[50], username[50], parola[50];

        snprintf(website, sizeof(website), "%s", sqlite3_column_text(selectStmt, 0));
        snprintf(username, sizeof(username), "%s", sqlite3_column_text(selectStmt, 1));
        snprintf(parola, sizeof(parola), "%s", sqlite3_column_text(selectStmt, 2));

        strcat(raspuns, website);
        strcat(raspuns, " ");
        strcat(raspuns, username);
        strcat(raspuns, " ");
        strcat(raspuns, parola);
        strcat(raspuns, "\n");
    }

    if (strlen(raspuns) == 0)
    {

        strcpy(raspuns, "nu exista inregistrari");
        closeDB();
    }

    sqlite3_finalize(selectStmt);
    closeDB();
}

void findByWebBD(char web[], char raspuns[], char toKeep[])
{
    openDB();

    // cautam dupa website
    char query[200];
    snprintf(query, sizeof(query), "SELECT categorie, username, parola FROM listaparole WHERE MASTusername = ? AND website = ?");

    sqlite3_stmt *selectStmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &selectStmt, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "eroare la creare statement: %s\n", sqlite3_errmsg(db));
        strcpy(raspuns, "eroare la cautare");
        closeDB();
        return;
    }

    sqlite3_bind_text(selectStmt, 1, toKeep, -1, SQLITE_STATIC);
    sqlite3_bind_text(selectStmt, 2, web, -1, SQLITE_STATIC);

    strcpy(raspuns, "");
    // concatenam randurile gasite
    while (sqlite3_step(selectStmt) == SQLITE_ROW)
    {
        char categorie[50], username[50], parola[50];

        snprintf(categorie, sizeof(categorie), "%s", sqlite3_column_text(selectStmt, 0));
        snprintf(username, sizeof(username), "%s", sqlite3_column_text(selectStmt, 1));
        snprintf(parola, sizeof(parola), "%s", sqlite3_column_text(selectStmt, 2));

        strcat(raspuns, categorie);
        strcat(raspuns, " ");
        strcat(raspuns, username);
        strcat(raspuns, " ");
        strcat(raspuns, parola);
        strcat(raspuns, "\n");
    }

    if (strlen(raspuns) == 0)
    {

        strcpy(raspuns, "nu exista inregistrari");
        closeDB();
    }

    sqlite3_finalize(selectStmt);
    closeDB();
}
// end sectiune BD

typedef struct thData
{
    int idThread; // id-ul thread-ului tinut in evidenta de acest program
    int cl;       // descriptorul intors de accept
} thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

void regis(char comanda[], char raspuns[])
{
    char info[500];
    strcpy(info, comanda);
    printf("am intrat in register\n");
    printf("raspuns%s\n", raspuns);
    printf("comanda%s\n", comanda);
    char *regargs;
    char *username;
    char *parola;
    regargs = strtok(info, " ");
    username = strtok(NULL, " ");
    parola = strtok(NULL, " ");
    if (username == NULL || parola == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }
    addUserDB(username, parola, raspuns);
}

void login(char comanda[], char raspuns[], int ok, char toKeep[])
{
    if (ok == 1)
    {
        strcpy(raspuns, "sunteti deja logat");
        return;
    }
    // prelucram 'comanda' pentru a extrage inf necesare
    char info[500];
    strcpy(info, comanda);
    char *logargs;
    char *MASTERusername;
    char *MASTERparola;
    logargs = strtok(info, " ");
    MASTERusername = strtok(NULL, " ");
    MASTERparola = strtok(NULL, " ");
    if (MASTERusername == NULL || MASTERparola == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }

    // verificam in baza de date
    checkCredentials(MASTERusername, MASTERparola, toKeep, raspuns);
}

void addPass(char comanda[], char raspuns[], int ok, char toKeep[])
{
    if (ok == 0)
    {
        strcpy(raspuns, "trebui sa fiti logat");
        return;
    }
    // prelucram 'comanda' pentru a extrage inf necesare
    char info[500];
    strcpy(info, comanda);
    char *addargs;
    char *categorie;
    char *website;
    char *username;
    char *parola;
    addargs = strtok(info, " ");
    categorie = strtok(NULL, " ");
    website = strtok(NULL, " ");
    username = strtok(NULL, " ");
    parola = strtok(NULL, " ");
    if (categorie == NULL || website == NULL || username == NULL || parola == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }
    // aduagam in baza de date
    addPassDB(toKeep, categorie, website, username, parola, raspuns);
}

void findPass(char comanda[], char raspuns[], int ok, char toKeep[])
{
    if (ok == 0)
    {
        strcpy(raspuns, "trebuie sa fiti logat");
    }
    // prelucram 'comanda' pentru a extrage inf necesare
    char info[500];
    strcpy(info, comanda);
    char *findargs;
    char *findmode;
    char *webcat;
    findargs = strtok(info, " ");
    findmode = strtok(NULL, " ");
    webcat = strtok(NULL, " ");
    if (findargs == NULL || findmode == NULL || webcat == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }
    if (strstr(findmode, "-w")) // cautam in baza de date dupa website
    {
        findByWebBD(webcat, raspuns, toKeep);
        printf("findPass raspuns %s\n", raspuns);
        return;
    }
    if (strstr(findmode, "-c")) // cautam in baza de date dupa categorie
    {
        findByCatBD(webcat, raspuns, toKeep);
        return;
    }
    strcpy(raspuns, "argumente invalide");
}

void updatePass(char comanda[], char raspuns[], int ok, char toKepp[])
{
    if (ok == 0)
    {
        strcpy(raspuns, "trebuie sa fiti logat");
        return;
    }
    // prelucram 'comanda' pentru a extrage inf necesare
    char info[500];
    strcpy(info, comanda);
    char *updateargs;
    char *website;
    char *categorie;
    char *username;
    char *parola;
    updateargs = strtok(info, " ");
    categorie = strtok(NULL, " ");
    website = strtok(NULL, " ");
    username = strtok(NULL, " ");
    parola = strtok(NULL, " ");
    if (website == NULL || categorie == NULL || username == NULL || parola == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }
    // actualizam baza de date
    updatePassDB(toKepp, categorie, website, username, parola, raspuns);
}

delPass(char comanda[], char raspuns[], int ok, char toKeep[])
{
    if (ok == 0)
    {
        strcpy(raspuns, "trebuie sa fiti logat");
        return;
    }
    // prelucram 'comanda' pentru a extrage inf necesare
    char info[500];
    strcpy(info, comanda);
    char *delargs;
    char *website;
    char *username;
    delargs = strtok(info, " ");
    website = strtok(NULL, " ");
    username = strtok(NULL, " ");
    if (website == NULL || username == NULL)
    {
        strcpy(raspuns, "comanda incompleta, introduceti toate argumentele");
        return;
    }
    // stergem din baza de date
    delPassBD(toKeep, website, username, raspuns);
}

void logic(char comanda[], char raspuns[], int ok, char toKeep[]) // aici se face rout-ul comenzilor
{
    if (strstr(comanda, "register"))
    {
        regis(comanda, raspuns);
        return;
    }
    if (strstr(comanda, "login"))
    {
        login(comanda, raspuns, ok, toKeep);
        return;
    }
    if (strstr(comanda, "logout"))
    {
        strcpy(raspuns, "out");
        return;
    }
    if (strstr(comanda, "addPass"))
    {
        addPass(comanda, raspuns, ok, toKeep);
        return;
    }

    if (strstr(comanda, "findPass"))
    {
        findPass(comanda, raspuns, ok, toKeep);
        return;
    }
    if (strstr(comanda, "updatePass"))
    {
        updatePass(comanda, raspuns, ok, toKeep);
        return;
    }
    if (strstr(comanda, "delPass"))
    {
        delPass(comanda, raspuns, ok, toKeep);
        return;
    }

    strcpy(raspuns, "comanda invalida");
}

int main()
{
    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;
    int sd; // descriptorul de socket
    pthread_t th[100]; // Identificatorii thread-urilor care se vor crea
    int i = 0;

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    /* utilizarea optiunii SO_REUSEADDR */
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }
    /* servim in mod concurent clientii...folosind thread-uri */
    while (1)
    {
        int client;
        thData *td; // parametru functia executata de thread
        int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        // client= malloc(sizeof(int));
        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        /* s-a realizat conexiunea, se astepta mesajul */

        // int idThread; //id-ul threadului
        // int cl; //descriptorul intors de accept

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);

    } // while
};
static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    raspunde((struct thData *)arg);
    /* am terminat cu acest client, inchidem conexiunea */
    close((intptr_t)arg);
    return (NULL);
};

void raspunde(void *arg)
{
    char comanda[500];
    int q = 0;
    int ok = 0;
    char toKeep[30]; // retine Masterusername-ul autentificat in sesiune
    while (q == 0)
    {
        char cbuf[500];
        char raspuns[10000];
        struct thData tdL;
        tdL = *((struct thData *)arg);
        if (read(tdL.cl, &cbuf, sizeof(cbuf)) <= 0)
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
        }
        strcpy(comanda, cbuf);
        strcpy(cbuf, "");
        printf("[Thread %d]Mesajul a fost receptionat...%s\n", tdL.idThread, comanda);

        /*pregatim mesajul de raspuns */

        logic(comanda, raspuns, ok, toKeep);
        // logica aditionala pt a trata niste cazuri
        if (strstr(comanda, "quit"))
        {
            strcpy(raspuns, comanda);
            q = 1;
        }
        if (strstr(raspuns, "out"))
        {
            ok = 0;
            strcpy(toKeep, "");
        }
        if (strstr(raspuns, "logged"))
        {
            ok = 1;
        }

        printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, raspuns);

        /* returnam mesajul clientului */
        if (write(tdL.cl, &raspuns, sizeof(raspuns)) <= 0)
        {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        }
        else
        {
            printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
            strcpy(raspuns, "");
        }
    }
}