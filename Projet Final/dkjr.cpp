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

void tueEnnemisProximite(void);

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
	sigaction(SIGQUIT, &signal, NULL);

	//On arme SIGALRM pour ThreadEnnemis
	signal.sa_handler = HandlerSIGALRM;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGALRM, &signal, NULL);

	signal.sa_handler = HandlerSIGUSR1;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGUSR1, &signal, NULL);

	signal.sa_handler = HandlerSIGINT;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGINT, &signal, NULL);

	signal.sa_handler = HandlerSIGUSR2;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGUSR2, &signal, NULL);

	signal.sa_handler = HandlerSIGHUP;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGHUP, &signal, NULL);

	signal.sa_handler = HandlerSIGCHLD;
	sigemptyset(&signal.sa_mask);
	sigaction(SIGCHLD, &signal, NULL);

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
	
	//On masque les signaux partout. Ils seront seulement recus par les threads concernés!
	sigset_t masque;
	sigfillset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

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

	sigset_t masque;
	sigfillset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

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

	sigset_t masque;
	sigfillset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

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

	sigset_t masque;
	sigfillset(&masque);
	sigdelset(&masque, SIGINT);
	sigdelset(&masque, SIGQUIT);
	sigdelset(&masque, SIGHUP);
	sigdelset(&masque, SIGCHLD);
	sigprocmask(SIG_SETMASK, &masque, NULL);

	printf("Debut ThreadDKJr....tid = %d\n", pthread_self());

 	pthread_mutex_lock(&mutexGrilleJeu);
 
 	setGrilleJeu(3, 1, DKJR); 
 	afficherDKJr(11, 9, 1); 
 	etatDKJr = LIBRE_BAS; 
 	positionDKJr = 1;
 
 	pthread_mutex_unlock(&mutexGrilleJeu);

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
						if(grilleJeu[3][positionDKJr-1].type == CROCO)
						{
							pthread_kill(grilleJeu[3][positionDKJr-1].tid, SIGUSR2);

							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							on = false;
							tueEnnemisProximite();
						}
						else
						{
							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr--;
							setGrilleJeu(3, positionDKJr, DKJR);
							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							printf("State: LIBRE_BAS --- Event: KEY_LEFT\n");
							afficherGrilleJeu();
						}
					}
					break;

				case SDLK_RIGHT:
					if(positionDKJr < 7)
					{
						if(grilleJeu[3][positionDKJr+1].type == CROCO)
						{
							pthread_kill(grilleJeu[3][positionDKJr+1].tid, SIGUSR2);

							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							on = false;
							tueEnnemisProximite();
						}
						else
						{
							setGrilleJeu(3, positionDKJr); //on efface de la grille de jeu le "1" pour dire que DKJr ne s'y trouve plus
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr++;
							setGrilleJeu(3, positionDKJr, DKJR);
							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
							
							printf("State: LIBRE_BAS --- Event: KEY_RIGHT\n");
							afficherGrilleJeu();
						}
					}
					break;

				case SDLK_UP:
					if(grilleJeu[2][positionDKJr].type == CORBEAU)
					{
						pthread_kill(grilleJeu[2][positionDKJr].tid, SIGUSR1);
						
						setGrilleJeu(3, positionDKJr);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						on = false;
						tueEnnemisProximite();
					}
					else
					{
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


							if(grilleJeu[3][positionDKJr].type == CROCO)
							{
								pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
								on = false;
								tueEnnemisProximite();
							}
							else
							{
								setGrilleJeu(3, positionDKJr, DKJR);
								afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
								
								printf("State: LIBRE_BAS --- Event: Gravity after KEY_UP\n");
								afficherGrilleJeu();
							}
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

					if(grilleJeu[3][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
						on = false; //Fin du thread, on perd une vie
						tueEnnemisProximite();
					}
					else
					{
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIANE_BAS --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
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

					if(grilleJeu[3][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
						on = false; //Fin du thread, on perd une vie
						tueEnnemisProximite();
					}
					else
					{
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: DOUBLE_LIANE_BAS --- Event: KEY_DOWN\n");
						afficherGrilleJeu();						
					}
				}
				break;

				case SDLK_UP:
				{
					etatDKJr = LIBRE_HAUT;
					setGrilleJeu(2, positionDKJr);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					setGrilleJeu(1, positionDKJr, DKJR);
					afficherDKJr(7, (positionDKJr * 2) + 7, 6);

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
						if(grilleJeu[1][positionDKJr-1].type == CROCO)
						{
							pthread_kill(grilleJeu[1][positionDKJr-1].tid, SIGUSR2);

							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							on = false; //Fin du thread, on perd une vie
							tueEnnemisProximite();
						}
						else
						{
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr--;
							setGrilleJeu(1, positionDKJr, DKJR);
							afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							printf("State: LIBRE_HAUT --- Event: KEY_LEFT\n");
							afficherGrilleJeu();								
						}
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
								tueEnnemisProximite();
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

									afficherCage(4); //On réaffiche la 4eme partie de la cage car elle est effacée à cause de l'effacement de l'img de DKJr

									positionDKJr--;
									setGrilleJeu(1, positionDKJr, DKJR);
									afficherDKJr(6, 10, 11);
									
									afficherGrilleJeu();

									nanosleep(&temps2, NULL);

									setGrilleJeu(1, positionDKJr);
									effacerCarres(6, 10, 2, 3);

									tueEnnemisProximite();

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
						if(grilleJeu[1][positionDKJr+1].type == CROCO)
						{
							pthread_kill(grilleJeu[1][positionDKJr+1].tid, SIGUSR2);

							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							on = false; //Fin du thread, on perd une vie
							tueEnnemisProximite();
						}
						else
						{
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							positionDKJr++;
							setGrilleJeu(1, positionDKJr, DKJR);

							if(positionDKJr == 7)
								afficherDKJr(7, (positionDKJr * 2) + 7, 6);
							else
								afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							printf("State: LIBRE_HAUT --- Event: KEY_RIGHT\n");
							afficherGrilleJeu();
						}
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

						if(grilleJeu[1][positionDKJr].type == CROCO)
						{
							pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);
							on = false;
							tueEnnemisProximite();
						}
						else
						{
							setGrilleJeu(1, positionDKJr, DKJR);
							afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
							
							printf("State: LIBRE_HAUT --- Event: Gravity after KEY_UP\n");
							afficherGrilleJeu();
						}
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

					if(grilleJeu[1][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);
						on = false; //Fin du thread, on perd une vie
					}
					else
					{
						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIANE_HAUT --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
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

	sigset_t masque;
	sigfillset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

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

					effacerCarres(4, 9, 2, 3);
					afficherDKJr(3, 11, 10); //En effacant la 4eme partie de la cage, on efface aussi un morceau de DKJr donc on le réaffiche ici

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
	sigset_t masque;
	sigfillset(&masque);
	sigprocmask(SIG_SETMASK, &masque, NULL);

	afficherScore(score);

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
	sigfillset(&masque);
	sigdelset(&masque, SIGALRM);
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

	sigset_t masque;
	sigfillset(&masque);
	sigdelset(&masque, SIGUSR1);
	sigprocmask(SIG_SETMASK, &masque, NULL);

	int numImg;
	struct timespec t;
	t.tv_nsec = 700000000;
	t.tv_sec = 0;

	for(*numCol = 0; *numCol < 8; (*numCol)++)
	{
		if(*numCol % 2 == 0)
				numImg = 2;
		else
			numImg = 1;

		afficherCorbeau( ((*numCol) * 2) + 8, numImg);

		pthread_mutex_lock(&mutexGrilleJeu);
		if(grilleJeu[2][*numCol].type == DKJR)
		{
			pthread_mutex_unlock(&mutexGrilleJeu);

			kill(getpid(), SIGINT);
			if(*numCol % 2 == 0)
				effacerCarres(9, ((*numCol) * 2) + 8, 2, 1);
			else
				effacerCarres(10, ((*numCol) * 2) + 8, 1, 1);
			
			pthread_exit(0);
		}
		else
		{
			setGrilleJeu(2, *numCol, CORBEAU, pthread_self());
			
			pthread_mutex_unlock(&mutexGrilleJeu);
			nanosleep(&t, NULL);
			pthread_mutex_lock(&mutexGrilleJeu);

			if(*numCol % 2 == 0)
				effacerCarres(9, ((*numCol) * 2) + 8, 2, 1);
			else
				effacerCarres(10, ((*numCol) * 2) + 8, 1, 1);

			setGrilleJeu(2, *numCol, VIDE);
		}
		pthread_mutex_unlock(&mutexGrilleJeu);
	}
	pthread_exit(0);
}

void* FctThreadCroco(void*)
{
	sigset_t masque;
	sigfillset(&masque);
	sigdelset(&masque, SIGUSR2);
	sigprocmask(SIG_SETMASK, &masque, NULL);

	struct timespec t;
	t.tv_nsec = 700000000;
	t.tv_sec = 0; 

	S_CROCO *pCroco = (S_CROCO*)malloc(sizeof(S_CROCO));
	pthread_setspecific(keySpec, pCroco);

	int numImg;

	for(pCroco->position = 2, pCroco->haut = true; pCroco->position < 8; (pCroco->position)++)
	{
		if(pCroco->position % 2 == 0)
			numImg = 2;
		else
			numImg = 1;

		pthread_mutex_lock(&mutexGrilleJeu);
		if(grilleJeu[1][pCroco->position].type == DKJR)
		{
			pthread_mutex_unlock(&mutexGrilleJeu);
			kill(getpid(), SIGHUP);
			effacerCarres(8, ((pCroco->position-1) * 2) + 7);
			setGrilleJeu(1, pCroco->position-1);
			pthread_exit(0);
		}
		else
		{
			effacerCarres(8, ((pCroco->position-1) * 2) + 7);
			setGrilleJeu(1, pCroco->position-1);

			afficherCroco( (pCroco->position * 2) + 7, numImg);
			setGrilleJeu(1, pCroco->position, CROCO, pthread_self());
			
			pthread_mutex_unlock(&mutexGrilleJeu);
			nanosleep(&t, NULL);
			pthread_mutex_lock(&mutexGrilleJeu);
		}
		pthread_mutex_unlock(&mutexGrilleJeu);
	}

	effacerCarres(8, ((pCroco->position-1) * 2) + 7);
	setGrilleJeu(1, pCroco->position-1);

	numImg = 3;
	(pCroco->position)--;
	afficherCroco((pCroco->position * 2) + 7, numImg);

	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(2, pCroco->position, CROCO, pthread_self());
	pthread_mutex_unlock(&mutexGrilleJeu);

	nanosleep(&t, NULL);

	effacerCarres(9, (pCroco->position * 2) + 9);
	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(2, pCroco->position);
	pthread_mutex_unlock(&mutexGrilleJeu);

	for(pCroco->haut = false; pCroco->position > 0; (pCroco->position)--)
	{
		if(pCroco->position % 2 == 0)
			numImg = 5;
		else
			numImg = 4;
		
		pthread_mutex_lock(&mutexGrilleJeu);
		if(grilleJeu[3][pCroco->position].type == DKJR)
		{
			pthread_mutex_unlock(&mutexGrilleJeu);
			kill(getpid(), SIGCHLD);
			effacerCarres(12, ((pCroco->position+1) * 2) + 8);
			setGrilleJeu(3, pCroco->position+1);
			pthread_exit(0);
		}
		else
		{
			if(pCroco->position < 7) //Pour ne pas exécuter les 2 lignes lors du premier tour de la boucle!
			{
				effacerCarres(12, ((pCroco->position+1) * 2) + 8, 1, 1);
				setGrilleJeu(3, pCroco->position+1);
			}

			afficherCroco( (pCroco->position * 2) + 8, numImg);
			setGrilleJeu(3, pCroco->position, CROCO, pthread_self());

			afficherGrilleJeu();

			pthread_mutex_unlock(&mutexGrilleJeu);
			nanosleep(&t, NULL);
			pthread_mutex_lock(&mutexGrilleJeu);
		}
		pthread_mutex_unlock(&mutexGrilleJeu);
	}
	effacerCarres(12, ((pCroco->position+1) * 2) + 8);
	setGrilleJeu(3, pCroco->position+1);

	pthread_exit(0);
}

void DestructeurVS(void *p) { free(p); }

void tueEnnemisProximite()
{
	for(int i=0; i<3; i++)
	{
		if(grilleJeu[3][i+1].type == CROCO)
		{
			printf("--------------\nCROCO DETECTE AU SPAWN\n-------------\n");
			afficherGrilleJeu();

			pthread_kill(grilleJeu[3][i+1].tid, SIGUSR2);
		}	

		if(grilleJeu[2][i].type == CORBEAU)
		{
			printf("--------------\nCORBEAU DETECTE AU SPAWN\n-------------\n");
			afficherGrilleJeu();
			
			pthread_kill(grilleJeu[2][i].tid, SIGUSR1);
		}		
	}	
}

void HandlerSIGQUIT(int signal) {}

void HandlerSIGALRM(int signal)
{
	if(delaiEnnemis > 2500)
	{
		delaiEnnemis -= 250;
		alarm(15);
		printf("delaisEnnemis = %d\n", delaiEnnemis);
	}
}

void HandlerSIGUSR1(int signal)
{
	int *p = (int*)pthread_getspecific(keySpec);

	setGrilleJeu(2, *p, VIDE);
	if(*p % 2 == 0)
		effacerCarres(9, ((*p) * 2) + 8, 2, 1);
	else
		effacerCarres(10, ((*p) * 2) + 8, 1, 1);

	pthread_exit(0);
}

void HandlerSIGINT(int signal)
{
	if(etatDKJr == LIBRE_BAS)
		pthread_mutex_unlock(&mutexEvenement);

	setGrilleJeu(2, positionDKJr);
	effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
	pthread_exit(0);
}

void HandlerSIGUSR2(int signal)
{
	S_CROCO *p = (S_CROCO*)pthread_getspecific(keySpec);

	pthread_mutex_lock(&mutexGrilleJeu);
	if(p->haut == 0)
	{
		setGrilleJeu(3, p->position);
		effacerCarres(12 ,(p->position * 2) + 8);
	}
	else
	{
		setGrilleJeu(1, p->position);
		effacerCarres(8, (p->position * 2) + 7);
	}
	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);
}

void HandlerSIGHUP(int signal)
{
	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(1, positionDKJr);
	pthread_mutex_unlock(&mutexGrilleJeu);

	effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
	pthread_exit(0);
}

void HandlerSIGCHLD(int signal)
{
	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(3, positionDKJr);
	pthread_mutex_unlock(&mutexGrilleJeu);

	effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
	pthread_exit(0);
}