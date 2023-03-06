#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <signal.h>

#include <unistd.h>

void* threadsSlave(void*);
void HandlerSIGINT(int);

pthread_t Handlethread1;
pthread_t Handlethread2;
pthread_t Handlethread3;
pthread_t Handlethread4;

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

	printf("Identite du thread principal (main) : %d\n", pthread_self());

	printf("Creation des threads secondaires\n");
	ret = pthread_create(&Handlethread1, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread2, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread3, NULL, (void*(*)(void*))threadsSlave, NULL);
	ret = pthread_create(&Handlethread4, NULL, (void*(*)(void*))threadsSlave, NULL);

	pause();

	ret = pthread_join(Handlethread1, NULL);
	ret = pthread_join(Handlethread2, NULL);
	ret = pthread_join(Handlethread3, NULL);
	ret = pthread_join(Handlethread4, NULL);

	return 0;
}

void* threadsSlave(void*)
{
	printf("Debut du thread\nIdentite du thread: %d\n", pthread_self());
	printf("(thread %d) Attente de la reception d'un signal...\n", pthread_self());
	pause();

	pthread_exit(NULL);
}

void HandlerSIGINT(int signal)
{
	printf("Reception du signal SIGINT par le thread %d\n", pthread_self());
	pthread_exit(NULL);
}