#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <signal.h>

#include <unistd.h>

void* threadsSlave(void*);
void* threadMaster(void*);
void HandlerSIGINT(int);
void HandlerSIGUSR1(int);

pthread_t Handlethread1;
pthread_t Handlethread2;
pthread_t Handlethread3;
pthread_t Handlethread4;
pthread_t HandlerthreadMaster;

int main()
{
	int ret;
	struct sigaction A;

	A.sa_handler = HandlerSIGINT;
	sigemptyset(&A.sa_mask);
	A.sa_flags = 0;

	if(sigaction(SIGINT, &A, NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(1);
	}

    A.sa_handler = HandlerSIGUSR1;
    if(sigaction(SIGUSR1, &A, NULL) == -1)
    {
        perror("Erreur de sigaction");
        exit(1);
    }

	printf("Identite du thread principal (main) : %d\n", pthread_self());

	printf("Creation des threads secondaires\n");
	ret = pthread_create(&Handlethread1, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread2, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread3, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread4, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&HandlerthreadMaster, NULL, (void*(*)(void*))threadMaster, NULL);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	pause();

	ret = pthread_join(Handlethread1, NULL);
	ret = pthread_join(Handlethread2, NULL);
	ret = pthread_join(Handlethread3, NULL);
	ret = pthread_join(Handlethread4, NULL);
	ret = pthread_join(HandlerthreadMaster, NULL);

	return 0;
}

void* threadsSlave(void*)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	printf("\nDebut du thread\nIdentite du thread: %d\n", pthread_self());
	printf("(thread %d) Attente de la reception d'un signal...\n", pthread_self());
	pause();

	pthread_exit(NULL);
}

void* threadMaster(void*)
{
	printf("\nDebut du thread Master\nIdentite du thread: %d\n", pthread_self());
	printf("(thread %d) Attente de la réception d'un signal...\n", pthread_self());

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_SETMASK, &mask, NULL);
	
	while(1)
	{
		pause();
	}

	pthread_exit(NULL);
}

void HandlerSIGINT(int signal)
{
	printf("\nReception du signal SIGINT par le thread Master\n");
    kill(getpid(), SIGUSR1);
}

void HandlerSIGUSR1(int signal)
{
    printf("\nReception du signal SIGUSR1 par le thread %d\n", pthread_self);
}