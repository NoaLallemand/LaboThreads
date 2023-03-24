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
	int evt, nbrEchecs = 0;

	ouvrirFenetreGraphique();
	initGrilleJeu();

	struct sigaction signal;
	signal.sa_handler = HandlerSIGQUIT;
	sigemptyset(&signal.sa_mask);
	signal.sa_flags = 0;

	sigset_t masque;
	sigemptyset(&masque);
	sigaddset(&masque, SIGQUIT);
	sigaddset(&masque, SIGALRM);
	sigprocmask(SIG_SETMASK, &masque, NULL); //On masque le signal SIGQUIT partout. Il sera seulement visible par FctThreadDKJr

	sigaction(SIGQUIT, &signal, NULL);

	//On arme SIGALRM pour ThreadEnnemis
	signal.sa_handler = HandlerSIGALRM;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGALRM, &signal, NULL);

	pthread_cond_init(&condDK, NULL);
	pthread_cond_init(&condScore, NULL);

	pthread_mutex_init(&mutexGrilleJeu, NULL);
	pthread_mutex_init(&mutexEvenement, NULL);
	pthread_mutex_init(&mutexDK, NULL);
	pthread_mutex_init(&mutexScore, NULL);

	pthread_key_create(&keySpec, DestructeurVS);

	pthread_create(&threadCle, NULL, FctThreadCle, NULL);
	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);
	pthread_create(&threadDK, NULL, FctThreadDK, NULL);
	pthread_create(&threadScore, NULL, FctThreadScore, NULL);
	pthread_create(&threadEnnemis, NULL, FctThreadEnnemis, NULL);
	
	while(nbrEchecs < 3)
	{
		pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);
		pthread_join(threadDKJr, NULL);
		
		nbrEchecs++;
		afficherEchec(nbrEchecs);
	}
	
	pthread_join(threadEnnemis, NULL);
	pthread_join(threadScore, NULL);
	pthread_join(threadDK, NULL);
	pthread_join(threadEvenements, NULL);
	pthread_join(threadCle, NULL);
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
	struct timespec temps2;

	temps.tv_sec = 1;
	temps.tv_nsec = 400000000;

	temps2.tv_sec = 0;
	temps2.tv_nsec = 500000000;

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
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
							
							positionDKJr--;

							setGrilleJeu(0, positionDKJr, DKJR);
							afficherDKJr(5, (positionDKJr * 2) + 7, 9);
							afficherGrilleJeu();

							if(grilleJeu[0][1].type == VIDE)
							{
								pthread_mutex_unlock(&mutexGrilleJeu);
								nanosleep(&temps2, NULL);
								pthread_mutex_lock(&mutexGrilleJeu);

								setGrilleJeu(0, positionDKJr);
								effacerCarres(5, 12, 3, 2);
								positionDKJr--;
								setGrilleJeu(1, positionDKJr, DKJR);
								afficherDKJr(6, 11, 12);

								afficherGrilleJeu();

								pthread_mutex_unlock(&mutexGrilleJeu);
								nanosleep(&temps2, NULL);
								pthread_mutex_lock(&mutexGrilleJeu);

								setGrilleJeu(1, positionDKJr);
								effacerCarres(6, 10, 2, 3);
								positionDKJr--;
								setGrilleJeu(3, positionDKJr, DKJR);
								afficherDKJr(11, 7, 13);

								afficherGrilleJeu();

								pthread_mutex_unlock(&mutexGrilleJeu);
								nanosleep(&temps2, NULL);
								pthread_mutex_lock(&mutexGrilleJeu);

								setGrilleJeu(3, positionDKJr);
								effacerCarres(11, 7, 2, 2);
								on = false;
							}
							else
							{
								if(grilleJeu[0][1].type == CLE)
								{
									nanosleep(&temps2, NULL);

									setGrilleJeu(0, positionDKJr);
									effacerCarres(5, 12, 3, 2);

									positionDKJr--;
									setGrilleJeu(0, positionDKJr, DKJR);
									afficherDKJr(3, 11, 10);

									pthread_mutex_lock(&mutexDK);
									MAJDK = true;
									pthread_mutex_unlock(&mutexDK);
									pthread_cond_signal(&condDK);

									nanosleep(&temps2, NULL);

									pthread_mutex_lock(&mutexScore);
									score += 10;
									MAJScore = true;
									pthread_mutex_unlock(&mutexScore);
									pthread_cond_signal(&condScore);

									setGrilleJeu(0, positionDKJr);
									effacerCarres(3, 11, 3, 2);

									positionDKJr--;
									setGrilleJeu(1, positionDKJr, DKJR);
									afficherDKJr(6, 10, 11);
									
									afficherGrilleJeu();

									nanosleep(&temps2, NULL);

									setGrilleJeu(1, positionDKJr);
									effacerCarres(6, 10, 2, 3);

									setGrilleJeu(3, 1, DKJR); 
									afficherDKJr(11, 9, 1); 
									etatDKJr = LIBRE_BAS; 
									positionDKJr = 1;
								}
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

void* FctThreadDK(void*)
{
	int nbMorceauxCage;
	struct timespec temps;
	temps.tv_nsec = 700000000;
	temps.tv_sec = 0;

	while(1)
	{
		nbMorceauxCage = 4;
		for(int i=1; i<=4; i++)
		{
			afficherCage(i);
		}

		pthread_mutex_lock(&mutexDK);
		while(nbMorceauxCage > 0)
		{
			pthread_cond_wait(&condDK, &mutexDK);

			switch(nbMorceauxCage)
			{
				case 4:
					effacerCarres(2, 7, 2, 2);
					break;

				case 3:
					effacerCarres(2, 9, 2, 2);
					break;

				case 2:
					effacerCarres(4, 7, 2, 2);
					break;

				case 1:
				{
					pthread_mutex_lock(&mutexScore);
					score += 10;
					pthread_mutex_unlock(&mutexScore);

					effacerCarres(4, 9, 2, 2);
					afficherRireDK();
					nanosleep(&temps, NULL);
					effacerCarres(3, 8, 2, 2);
				}
			}

			nbMorceauxCage--;
			MAJDK = false;
		}
		pthread_mutex_unlock(&mutexDK);
	}

	pthread_exit(0);
}

void* FctThreadScore(void*)
{
	while(1)
	{
		pthread_mutex_lock(&mutexScore);
		
		pthread_cond_wait(&condScore, &mutexScore);

		if(MAJScore == true)
		{
			afficherScore(score);
		}
		MAJScore = false;
		pthread_mutex_unlock(&mutexScore);
	}
	pthread_exit(0);
}

void* FctThreadEnnemis(void*)
{
	sigset_t masque;
	sigemptyset(&masque);
	sigaddset(&masque, SIGQUIT);
	sigprocmask(SIG_SETMASK, &masque, NULL);

	pthread_t threadCorbeaux;
	pthread_t threadCroco;

	struct timespec t;
	struct timespec reste;
	int tmpReste, choixEnnemi;
	srand(time(NULL));

	alarm(15);

	while(1)
	{	
		if(delaiEnnemis > 2500)
		{
			tmpReste = (delaiEnnemis % 1000);
			if(tmpReste == 0)
			{
				t.tv_sec = (delaiEnnemis/1000);
				t.tv_nsec = 0;
			}
			else
			{
				t.tv_sec = (delaiEnnemis/1000);
				t.tv_nsec = (tmpReste*1000000);
			}
		}
		choixEnnemi = rand() % 4;
		while(nanosleep(&t, &reste) == -1)
		{
			t.tv_sec = reste.tv_sec;
			t.tv_nsec = reste.tv_nsec;
		}
		

		if(choixEnnemi == 1 || choixEnnemi == 3)
		{
			pthread_create(&threadCorbeaux, NULL, FctThreadCorbeau, NULL);
		}
		else
		{
			pthread_create(&threadCroco, NULL, FctThreadCroco, NULL);
		}
	}

	pthread_exit(0);
}

void* FctThreadCorbeau(void*)
{
	int *numCol = (int*) malloc(sizeof(int));
	pthread_setspecific(keySpec, numCol);

	int numImg;
	struct timespec t;
	t.tv_nsec = 700000000;
	t.tv_sec = 0;

	for(*numCol = 0; *numCol < 7; (*numCol)++)
	{
		if(*numCol % 2 == 0)
			numImg = 2;
		else
			numImg = 1;

		afficherCorbeau( ((*numCol) * 2) + 8, numImg);
		setGrilleJeu(2, *numCol, CORBEAU, pthread_self());
		
		nanosleep(&t, NULL);

		if(*numCol % 2 == 0)
			effacerCarres(9, ((*numCol) * 2) + 8, 2, 1);
		else
			effacerCarres(10, ((*numCol) * 2) + 8, 1, 1);

	}

	pthread_exit(0);
}

void* FctThreadCroco(void*)
{
	printf("Creation Thread Croco\n");
	pthread_exit(0);
}

void DestructeurVS(void *p)
{

}

void HandlerSIGQUIT(int signal)
{

}

void HandlerSIGALRM(int signal)
{
	if(delaiEnnemis > 2500)
	{
		delaiEnnemis -= 250;
		alarm(15);
		printf("delaisEnnemis = %d\n", delaiEnnemis);
	}
}

