#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE        		0
#define DKJR       		1
#define CROCO       		2
#define CORBEAU     		3
#define CLE 			4

#define AUCUN_EVENEMENT    	0

#define LIBRE_BAS		1
#define LIANE_BAS		2
#define DOUBLE_LIANE_BAS	3
#define LIBRE_HAUT		4
#define LIANE_HAUT		5

void* FctThreadEvenements(void *);
void* FctThreadCle(void *);
void* FctThreadDK(void *);
void* FctThreadDKJr(void *);
void* FctThreadScore(void *);
void* FctThreadEnnemis(void *);
void* FctThreadCorbeau(void *);
void* FctThreadCroco(void *);

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

void DestructeurVS(void *p);

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;

pthread_key_t keySpec;

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
  bool haut;
  int position;
} S_CROCO;

// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	int evt;

	ouvrirFenetreGraphique();
	initGrilleJeu();

	struct sigaction signal;
	signal.sa_handler = HandlerSIGQUIT;
	sigemptyset(&signal.sa_mask);
	signal.sa_flags = 0;

	sigset_t masque;
	sigemptyset(&masque);
	sigaddset(&masque, SIGQUIT);
	sigprocmask(SIG_SETMASK, &masque, NULL); //On masque le signal SIGQUIT partout. Il sera seulement visible par FctThreadDKJr

	sigaction(SIGQUIT, &signal, NULL);

	pthread_mutex_init(&mutexGrilleJeu, NULL);
	pthread_mutex_init(&mutexEvenement, NULL);

	pthread_create(&threadCle, NULL, FctThreadCle, NULL);
	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);
	pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);

	pthread_join(threadCle, NULL);
	pthread_join(threadEvenements, NULL);
	pthread_join(threadDKJr, NULL);
}

// -------------------------------------

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{   
   for(int j = 0; j < 4; j++)
   {
       for(int k = 0; k < 8; k++)
          printf("%d  ", grilleJeu[j][k].type);
       printf("\n");
   }

   printf("\n");   
}


// --------------------- Fonctions Threads -----------------------

void* FctThreadCle(void *)
{
	struct timespec temps;
	temps.tv_sec = 0;
	temps.tv_nsec = 700000000;

	int i;
	while(1)
	{
		for(i=1; i <= 4; i++)
		{
			pthread_mutex_lock(&mutexGrilleJeu);
			switch(i)
			{
				case 1:
					setGrilleJeu(0, 1, CLE, threadCle);
					break;

				default: 
					setGrilleJeu(0, 1, VIDE, threadCle);
			}
			effacerCarres(3,12,2,3);
			afficherCle(i);
			pthread_mutex_unlock(&mutexGrilleJeu);
			nanosleep(&temps, NULL);
		}

		i -= 2;
		while(i > 1)
		{
			pthread_mutex_lock(&mutexGrilleJeu);
			switch(i)
			{
				case 1:
					setGrilleJeu(0, 1, CLE, threadCle);
					break;

				default: 
					setGrilleJeu(0, 1, VIDE, threadCle);
			}
			effacerCarres(3, 12, 2, 3);
			afficherCle(i);
			pthread_mutex_unlock(&mutexGrilleJeu);
			nanosleep(&temps, NULL);

			i--;
		}
	}

	pthread_exit(NULL);
}

void* FctThreadEvenements(void *)
{
	int evt;
	struct timespec temps;
	temps.tv_nsec = 10000000;
	temps.tv_sec = 0;


	while (1)
	{
	    evt = lireEvenement();

		pthread_mutex_lock(&mutexEvenement);
	    switch (evt)
	    {
			case SDL_QUIT:
				pthread_mutex_unlock(&mutexEvenement);
				exit(0);

			case SDLK_UP:
				evenement = SDLK_UP;
				break;

			case SDLK_DOWN:
				evenement = SDLK_DOWN;
				break;

			case SDLK_LEFT:
				evenement = SDLK_LEFT;
				break;

			case SDLK_RIGHT:
				evenement = SDLK_RIGHT;
	    }
		pthread_mutex_unlock(&mutexEvenement);

		kill(getpid(), SIGQUIT);
		nanosleep(&temps, NULL);

		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);
	}
}

void* FctThreadDKJr(void* p)
{
 	bool on = true;

	struct timespec temps;
	temps.tv_sec = 1,4;
	temps.tv_nsec = 0;

	printf("Debut ThreadDKJr....tid = %d\n", pthread_self());

 	pthread_mutex_lock(&mutexGrilleJeu);
 
 	setGrilleJeu(3, 1, DKJR); 
 	afficherDKJr(11, 9, 1); 
 	etatDKJr = LIBRE_BAS; 
 	positionDKJr = 1;
 
 	pthread_mutex_unlock(&mutexGrilleJeu);

	sigset_t masque;
	sigemptyset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

 	while(on)
 	{
 		pause();
 		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);
 		switch (etatDKJr)
 		{
 			case LIBRE_BAS:
			switch (evenement)
			{
				case SDLK_LEFT:
					if (positionDKJr > 1)
					{
						setGrilleJeu(3, positionDKJr);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
						positionDKJr--;
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIBRE_BAS --- Event: KEY_LEFT\n");
						afficherGrilleJeu();
					}
					break;

				case SDLK_RIGHT:
					if(positionDKJr < 7)
					{
						setGrilleJeu(3, positionDKJr); //on efface de la grille de jeu le "1" pour dire que DKJr ne s'y trouve plus
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
						positionDKJr++;
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						
						printf("State: LIBRE_BAS --- Event: KEY_RIGHT\n");
						afficherGrilleJeu();
					}
					break;

				case SDLK_UP:
					if( (positionDKJr >= 2 && positionDKJr <= 4) || positionDKJr == 6)
					{
						setGrilleJeu(3, positionDKJr);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR de l'emplacement en bas

						setGrilleJeu(2, positionDKJr, DKJR);
						afficherDKJr(10, (positionDKJr * 2) + 7, 8); //on actualise sa position quand il a sauté
						
						printf("State: LIBRE_BAS --- Event: KEY_UP\n");
						afficherGrilleJeu();

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						setGrilleJeu(2, positionDKJr);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR d'en haut pour ensuite le refaire tomber (effet de gravité)

						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						
						printf("State: LIBRE_BAS --- Event: Gravity after KEY_UP\n");
						afficherGrilleJeu();
					}
					else
					{
						setGrilleJeu(3, positionDKJr);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(2, positionDKJr, DKJR);
						if(positionDKJr == 7)
						{
							etatDKJr = DOUBLE_LIANE_BAS;
							afficherDKJr(10, (positionDKJr * 2) + 7, 5); //On affiche l'image 8 car DKJR se tient aux deux lianes
						}
						else
						{
							etatDKJr = LIANE_BAS;
							afficherDKJr(10, (positionDKJr * 2) + 7, 7);
						}
						
						printf("State: LIBRE_BAS --- Event: KEY_UP LIANE\n");
						afficherGrilleJeu();
					}
					break;
			}
			break;
 			
			case LIANE_BAS:
			switch(evenement)
			{
				case SDLK_DOWN:
				{
					etatDKJr = LIBRE_BAS;
					setGrilleJeu(2, positionDKJr);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					setGrilleJeu(3, positionDKJr, DKJR);
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					printf("State: LIANE_BAS --- Event: KEY_DOWN\n");
					afficherGrilleJeu();
				}
				break;

			}
			break;
			
			case DOUBLE_LIANE_BAS:
			switch(evenement)
			{
				case SDLK_DOWN:
				{
					etatDKJr = LIBRE_BAS;
					setGrilleJeu(2, positionDKJr);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					setGrilleJeu(3, positionDKJr, DKJR);
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					printf("State: DOUBLE_LIANE_BAS --- Event: KEY_DOWN\n");
					afficherGrilleJeu();
				}
				break;

				case SDLK_UP:
				{
					etatDKJr = LIBRE_HAUT;
					setGrilleJeu(2, positionDKJr);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					setGrilleJeu(1, positionDKJr, DKJR);
					afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					printf("State: DOUBLE_LIANE_BAS --- Event: KEY_UP\n");
					afficherGrilleJeu();
				}
				break;
			}
			break;

			case LIBRE_HAUT:
			switch(evenement)
			{
				case SDLK_DOWN:
					if(positionDKJr == 7)
					{
						etatDKJr = DOUBLE_LIANE_BAS;
						setGrilleJeu(1, positionDKJr);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
						
						setGrilleJeu(2, positionDKJr, DKJR);
						afficherDKJr(10, (positionDKJr * 2) + 7, 5);

						printf("State: LIBRE_HAUT --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
					break;

				case SDLK_LEFT:
					if(positionDKJr > 3)
					{
						setGrilleJeu(1, positionDKJr);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
						positionDKJr--;
						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIBRE_HAUT --- Event: KEY_LEFT\n");
						afficherGrilleJeu();
					}
					else
					{
						if(positionDKJr == 3)
						{
							if(grilleJeu[0][1].type == CLE)
							{
								setGrilleJeu(1, positionDKJr);
								effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
								
								positionDKJr--;

								etatDKJr = LIBRE_BAS;
								setGrilleJeu(2, positionDKJr, DKJR);
								afficherDKJr(10, (positionDKJr * 2) + 7, 8);
								
								printf("State: LIBRE_HAUT --- Event: KEY_LEFT\n");
								afficherGrilleJeu();

								pthread_mutex_unlock(&mutexGrilleJeu);
								nanosleep(&temps, NULL);
								pthread_mutex_lock(&mutexGrilleJeu);

								setGrilleJeu(2, positionDKJr);
								effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

								setGrilleJeu(3, positionDKJr, DKJR);
								afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

								printf("State: LIBRE_BAS --- Event: Chute...\n");
								afficherGrilleJeu();
							}
							else
							{

							}
						}
					}
					break;

				case SDLK_RIGHT:
					if(positionDKJr < 7)
					{
						setGrilleJeu(1, positionDKJr);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						if(evenement == SDLK_LEFT)
							positionDKJr--;
						else
							positionDKJr++;

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIBRE_HAUT --- Event: KEY_RIGHT\n");
						afficherGrilleJeu();
					}
					break;

				case SDLK_UP:
					if(positionDKJr == 3 || positionDKJr == 4)
					{
						setGrilleJeu(1, positionDKJr);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(0, positionDKJr, DKJR);
						afficherDKJr(6, (positionDKJr * 2) + 7, 8);

						printf("State: LIBRE_HAUT --- Event: KEY_UP\n");
						afficherGrilleJeu();

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						setGrilleJeu(0, positionDKJr);
						effacerCarres(6, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR d'en haut pour ensuite le refaire tomber (effet de gravité)

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						
						printf("State: LIBRE_HAUT --- Event: Gravity after KEY_UP\n");
						afficherGrilleJeu();
					}
					else
					{
						if(positionDKJr == 6)
						{
							etatDKJr = LIANE_HAUT;
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							setGrilleJeu(0, positionDKJr, DKJR);
							afficherDKJr(6, (positionDKJr * 2) + 7, 7);

							printf("State: LIBRE_HAUT --- Event: KEY_UP LIANE\n");
							afficherGrilleJeu();
						}
					}
					break;
			}
			break;

			case LIANE_HAUT:
			switch(evenement)
			{
				case SDLK_DOWN:
				{
					etatDKJr = LIBRE_HAUT;
					setGrilleJeu(0, positionDKJr);
					effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

					setGrilleJeu(1, positionDKJr, DKJR);
					afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					printf("State: LIANE_HAUT --- Event: KEY_DOWN\n");
					afficherGrilleJeu();
				}
				break;
			}
			break;
				
 		}
 		pthread_mutex_unlock(&mutexGrilleJeu);
 		pthread_mutex_unlock(&mutexEvenement);
 	}
 	pthread_exit(0);
}


void HandlerSIGQUIT(int signal)
{
	//printf("Réception du signal SIGQUIT\n");
}

