#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

typedef struct
{
    char nom[20];
    int nbSecondes;
}DONNEE;

void* fctThread(DONNEE *p);
void handlerSIGINT(int);
void destructeur(void*);

DONNEE data[] = { "MATAGNE", 15,
                  "WILVERS", 10,
                  "WAGNER", 17,
                  "QUETTIER", 8,
                  "", 0 };

DONNEE Param;

pthread_mutex_t mutexParam;
pthread_mutex_t mutexCompteur;
pthread_cond_t condCompteur;
pthread_key_t cle;

int compteur;


int main()
{
    pthread_t tabHandle[5];

    pthread_mutex_init(&mutexParam, NULL);
    pthread_mutex_init(&mutexCompteur, NULL);
    pthread_cond_init(&condCompteur, NULL);

    pthread_key_create(&cle, destructeur);

    //int i;
    for(int i=0; strcmp(data[i].nom, "") != 0; i++)
    {
        pthread_mutex_lock(&mutexParam);

        memcpy(&Param, &data[i], sizeof(DONNEE));
        pthread_create(&tabHandle[i], NULL, (void*(*)(void*))fctThread, &Param);

        pthread_mutex_lock(&mutexCompteur);
        compteur++;
        pthread_mutex_unlock(&mutexCompteur);
    }

    sigset_t masque;
    sigemptyset(&masque);
    sigaddset(&masque, SIGINT);
    sigprocmask(SIG_SETMASK, &masque, NULL);

    //for(int j=0; j < i; j++)
    //{
    //    pthread_join(tabHandle[j], NULL);
    //}

    pthread_mutex_lock(&mutexCompteur);
    while(compteur != 0)
    {
        pthread_cond_wait(&condCompteur, &mutexCompteur); //cette fonction fait : un unlock du mutex puis endort le thread, lorsque le thread est reveillé suite l'appel de pthread_cond_signal, on lock à nouveau le mutex car il faut le unlock après la boucle.
    }
    pthread_mutex_unlock(&mutexCompteur);
    printf("Fin du thread principal!\n");

    return 0;
}

void handlerSIGINT(int signal)
{
    char *pNom = (char *)pthread_getspecific(cle);
    printf("Thread %d.%d s'occupe de : %s\n", getpid(), pthread_self(), pNom);
}

void destructeur(void *p)
{
    free(p);
}

void* fctThread(DONNEE *p)
{
    struct timespec temps, tmpRest;
    struct sigaction A;

    A.sa_handler = handlerSIGINT;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;
    sigaction(SIGINT, &A, NULL);

    char *pNom = (char*) malloc(sizeof(p->nom));
    strcpy(pNom, p->nom);
    pthread_setspecific(cle, (void*)pNom);

    temps.tv_sec = ((DONNEE *)p)->nbSecondes;
    temps.tv_nsec = 0;

    printf("Thread %d.%d lancé!\n", getpid(), pthread_self());
    printf("Nom passé en paramètre :");
    puts(((DONNEE *)p)->nom);

    pthread_mutex_unlock(&mutexParam);

    while(nanosleep(&temps, &tmpRest) == -1)
    {
        printf("Temps restant lors de l'interruption du nanosleep : %d\n", tmpRest.tv_sec);
        temps.tv_sec = tmpRest.tv_sec;
        temps.tv_nsec = tmpRest.tv_nsec;
    }

    printf("Thread %d.%d se termine!\n", getpid(), pthread_self());

    pthread_mutex_lock(&mutexCompteur);
    compteur--;
    pthread_mutex_unlock(&mutexCompteur);
    
    pthread_cond_signal(&condCompteur);
    pthread_exit(NULL);

    return NULL;
}