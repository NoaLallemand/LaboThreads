#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

pthread_t handlerThread1;
pthread_t handlerThread2;
pthread_t handlerThread3;
pthread_t handlerThread4;

typedef struct{

	char NomFichier[35];
	char motRecherche[30];
	int nbrTabulations;
}structThread;

void * fctThread(structThread *paramThread);

int main()
{
	int ret, *retThread1 = NULL, *retThread2 = NULL, *retThread3 = NULL, *retThread4 = NULL;
	structThread thread1, thread2, thread3, thread4;

	strcpy(thread1.NomFichier, "texte1");
	strcpy(thread1.motRecherche, "thread");
	thread1.nbrTabulations = 0;

	strcpy(thread2.NomFichier, "texte2");
	strcpy(thread2.motRecherche, "Messi");
	thread2.nbrTabulations = 1;

	strcpy(thread3.NomFichier, "texte3");
	strcpy(thread3.motRecherche, "Standard");
	thread3.nbrTabulations = 2;

	strcpy(thread4.NomFichier, "texte4");
	strcpy(thread4.motRecherche, "programmation");
	thread4.nbrTabulations = 3;

	printf("Creation du premier thread\n");
	ret = pthread_create(&handlerThread1, NULL, (void *(*)(void*))fctThread, &thread1);
	
	printf("Creation du deuxieme thread\n");
	ret = pthread_create(&handlerThread2, NULL, (void *(*)(void*))fctThread, &thread2);
	
	printf("Creation du troisieme thread\n");
	ret = pthread_create(&handlerThread3, NULL, (void *(*)(void*))fctThread, &thread3);
	
	printf("Creation du quatrieme thread\n");
	ret = pthread_create(&handlerThread4, NULL, (void *(*)(void*))fctThread, &thread4);	

	ret = pthread_join(handlerThread1, (void **)&retThread1);
	ret = pthread_join(handlerThread2, (void **)&retThread2);
	ret = pthread_join(handlerThread3, (void **)&retThread3);
	ret = pthread_join(handlerThread4, (void **)&retThread4);

	printf("occurrences du mot (thread1) : %d\n", *retThread1);
	printf("occurrences du mot (thread2) : %d\n", *retThread2);
	printf("occurrences du mot (thread3) : %d\n", *retThread3);
	printf("occurrences du mot (thread4) : %d\n", *retThread4);

	free(retThread1);
	free(retThread2);
	free(retThread3);
	free(retThread4);

	printf("Fin du thread principal\n");
	return 0;
}

void * fctThread(structThread *paramThread)
{
	int *compteur = (int*)malloc(sizeof(int));
	*compteur = 0;
	int fd, nbrOctets, res;

	printf("th> Debut du thread\n");

	fd = open(paramThread->NomFichier, O_RDONLY);
	nbrOctets = lseek(fd, 0, SEEK_END);
	printf("th> Taille du fichier : %d octets\n", nbrOctets);
	close(fd);

	char mot[30], lecture[30];
	strcpy(mot, paramThread->motRecherche);
	res = strlen(mot);

	for(int i = 0; res == strlen(mot); i++)
	{
		fd = open(paramThread->NomFichier, O_RDONLY);

		switch(paramThread->nbrTabulations)
		{
			case 0:
				printf("*\n");
				break;

			case 1:
				printf("\t*\n");
				break;

			case 2:
				printf("\t\t*\n");
				break;

			case 3:
				printf("\t\t\t*\n");
				break;

			default:
				break;
		}
		
		lseek(fd, i*sizeof(char), SEEK_CUR);
		res = read(fd, &lecture, strlen(mot));
		close(fd);

		if(strcmp(mot, lecture) == 0)
			(*compteur)++;
	}
	pthread_exit(compteur);
}