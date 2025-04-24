
#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../winsuport2.h"		/* incloure definicions de funcions propies */
#include "../semafor.h"		/* incloure les funcions dels semafors */
#include "../memoria.h"
#include "../missatge.h"

#define MAX_OPO 9		/* màxim nombre d'oponents */

typedef struct {		/* per un tron (usuari o oponent) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
} tron;

typedef struct {		/* per una entrada de la taula de posicio */
	int f;
	int c;
}pos;

typedef struct {		/* variables de finalització compartides */
	int n_fil, n_col;

	int fi1;
	int fi2;
	
	int varia;		/* valor de variabilitat dels oponents [0..9] */
	int retard_min, retard_max;	/*rang minim-maxim del retard de moviment d'un oponent*/

	/* semafors */
	int id_sem_pant;		/* ID del semafor d'accés a pantalla */
	int id_sem_fit; 		/* ID del semafor d'accés al fitxer */
	int id_sem_var;			/* ID del semafor d'accés a les variables de finalitzacio */

	/* bústies */
	int id_bust[MAX_OPO];

	int id_map_mem;			/* ID de la zona de memoria compartida del mapa */

	int fd;
} shared_t;

typedef struct
{
	int dir;
	int i;
}action_param_t;



int id_sem_pant;
void *p_map;

shared_t *p_shared;
int id_bust;

pos *p_opo;
int n_opo;

int i_opo;

char mort;


void * event_action(void * par)
{
	action_param_t *param = (action_param_t*)par;
	int i = param->i;
	char segchar;
	if(param->dir == 0)
	{
		if(i != n_opo)	//si no es la primera posició (cap)
		{
			for(int j = i+1; j < n_opo && (mort == 0); j++)
			{		
				waitS(id_sem_pant);
				segchar = win_quincar(p_opo[j].f, p_opo[j].c);
				if(segchar != 'X') win_escricar(p_opo[j].f, p_opo[j].c, i_opo+'A', INVERS);
				signalS(id_sem_pant);
				win_retard(30);
			}
		}
	}
	else
	{
		if(i != 0)	//si no es la última posició (cua)
		{
			for(int j = i-1; j >= 0 && (mort == 0); j--)
			{		
				waitS(id_sem_pant);	
				segchar = win_quincar(p_opo[j].f, p_opo[j].c);
				if(segchar != 'X') win_escricar(p_opo[j].f, p_opo[j].c, i_opo+'a', INVERS);
				signalS(id_sem_pant);
				win_retard(30);
			}
		}
	}	
	
	
	free(param);

	return((void*)(intptr_t)0);

}

void * event_listener(void * i)
{
	pos position;
	do
	{
		receiveM(id_bust, (void*) &position);
		
		int trobat = 0;
		int i = 0;
		while(i < n_opo && trobat == 0)
		{
			trobat = (p_opo[i].c == position.c) && (p_opo[i].f == position.f);
			i++;
		}
		i--;

		if(trobat)
		{
			pthread_t tid1;
			action_param_t *param1 = malloc(sizeof(action_param_t));
			param1->dir = 0;		//direcció cap
			param1->i = i;

			pthread_t tid2;
			action_param_t *param2  = malloc(sizeof(action_param_t));
			param2->dir = 1;		//direcció cua
			param2->i = i;

			pthread_create(&tid1, NULL, event_action, (void*)param1);
			pthread_create(&tid2, NULL, event_action, (void*)param2);

			


		}
	}while(1);

	return ((void*)(intptr_t)0);

}


/* funcio per esborrar totes les posicions anteriors, sigui de l'usuari o */
/* de l'oponent */
void esborrar_posicions(pos p_pos[], int n_pos)
{
  int i;
  for (i=n_pos-1; i>=0; i--)		/* de l'ultima cap a la primera */
  {	
    
    /* vvv secció crítica d'escriptura de pantalla vvv*/
    waitS(id_sem_pant); 
    if(win_quincar(p_pos[i].f, p_pos[i].c) == 'X')
    {
	
    	win_escricar(p_pos[i].f,p_pos[i].c,'0',INVERS);	/* esborra una pos. */
    }
    else
    {

    	win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);	/* esborra una pos. */
    }

    signalS(id_sem_pant);
    /* ^^^ secció crítica d'escriptura de pantalla ^^^*/
    win_retard(10);		/* un petit retard per simular el joc real */
  }
}


/* funcio per moure un oponent una posicio; retorna 1 si l'oponent xoca */
/* contra alguna cosa, 0 altrament					*/
int main(int argc, char **argv)
{
  /* obtenir punter a memoria compartida */
  int id_shared_mem = atoi(argv[1]);
  p_shared = map_mem(id_shared_mem);
	
  /* inicialitzar accés a mapa */
  p_map = map_mem(p_shared->id_map_mem);
  win_set(p_map, p_shared->n_fil, p_shared->n_col);
  
  /* obtenir i_opo */
  i_opo = atoi(argv[2]);
	
  /* inicialitzar la posició del oponent */
  tron opo;
  p_opo = calloc((p_shared->n_fil*p_shared->n_col)/2, sizeof(pos));
  p_opo[0].f = atoi(argv[3]);
  p_opo[0].c = atoi(argv[4]);
  opo.c = p_opo[0].c;
  opo.f = p_opo[0].f;
  opo.d = 1;
  n_opo = 1;
	
  /* obtenir els valors de retard i variabilitat */
  int retard_min = p_shared->retard_min;
  int retard_max = p_shared->retard_max;
  int varia = p_shared->varia;

  /* obtenir els ids dels semafors */
  id_sem_pant = p_shared->id_sem_pant;
  int id_sem_fit = p_shared->id_sem_fit;
  int id_sem_var = p_shared->id_sem_var;

  /*obtenir el id de la bústia*/
  id_bust = p_shared->id_bust[i_opo];

  char cars;
  tron seg;
  int k, vk, nd, vd[3];
  int canvi = 0;
  int retorn = 0;


  int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
  int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

  pthread_t tid;
  pthread_create(&tid, NULL, event_listener, NULL);

  mort = 0; 

  do
  { 	
 	seg.f = opo.f + df[opo.d];	/* calcular seguent posicio */
 	seg.c = opo.c + dc[opo.d];
 	cars = win_quincar(seg.f,seg.c);	/* calcula caracter seguent posicio */
 	if (cars != ' ')			/* si seguent posicio ocupada */
 	   canvi = 1;		/* anotar que s'ha de produir un canvi de direccio */
 	else
 	  if (varia > 0)	/* si hi ha variabilitat */
 	  { k = rand() % 10;		/* prova un numero aleatori del 0 al 9 */
 	    if (k < varia) canvi = 1;	/* possible canvi de direccio */
 	  }
 	
 	if (canvi)		/* si s'ha de canviar de direccio */
 	{
	  canvi = 0;
 	  nd = 0;
 	  for (k=-1; k<=1; k++)	/* provar direccio actual i dir. veines */
 	  {
 	      vk = (opo.d + k) % 4;		/* nova direccio */
 	      if (vk < 0) vk += 4;		/* corregeix negatius */
 	      seg.f = opo.f + df[vk];		/* calcular posicio en la nova dir.*/
 	      seg.c = opo.c + dc[vk];
 	      cars = win_quincar(seg.f,seg.c);/* calcula caracter seguent posicio */
 	      if (cars == ' ')
 	      { vd[nd] = vk;			/* memoritza com a direccio possible */
 	        nd++;				/* anota una direccio possible mes */
 	      }
 	  }
 	  if (nd == 0)			/* si no pot continuar, */
 		retorn = 1;		/* xoc: ha perdut l'oponent! */
 	  else
 	  { if (nd == 1)			/* si nomes pot en una direccio */
 		opo.d = vd[0];			/* li assigna aquesta */
 	    else				/* altrament */
 	  	opo.d = vd[rand() % nd];	/* segueix una dir. aleatoria */
 	  }
 	}
 	if (retorn == 0)		/* si no ha col.lisionat amb res */
 	{
 	  opo.f = opo.f + df[opo.d];			/* actualitza posicio */
 	  opo.c = opo.c + dc[opo.d];
	  /* vvv secció crítica d'escriptura de pantalla vvv*/
	  waitS(id_sem_pant);
 	  win_escricar(opo.f,opo.c,i_opo+1+'0',INVERS);	/* dibuixa bloc oponent */
	  signalS(id_sem_pant);
	  /* ^^^ secció crítica d'escriptura de pantalla ^^^ */
 	  p_opo[n_opo].f = opo.f;			/* memoritza posicio actual */
 	  p_opo[n_opo].c = opo.c;
 	  n_opo++;
 	}
 	else
	{
		/* vvv Secció crítica variables globals vvv */
		waitS(id_sem_var);
		p_shared->fi2 = p_shared->fi2-1;
		signalS(id_sem_var);
		/* ^^^----------------------------------^^^ */
	}

	
	int retard_aleat = retard_min + (rand() % (retard_max-retard_min)); 		
	win_retard(retard_aleat);
  }while((p_shared->fi1 == 0) && (p_shared->fi2 != 0) && (retorn == 0));

  mort = 1;
  esborrar_posicions(p_opo, n_opo);

  if(p_shared->fd != -1)
  {
    FILE *fd = fdopen(p_shared->fd, "w");
    // el process escriu informació al fitxer abans de finalitzar
    // vvv secció crítica accés al fitxer vvv 
    if(p_shared->fd != 0)
    {
   	waitS(id_sem_fit);
   	fprintf(fd, "Fill id: %d, i_opo: %d\n", getpid(), i_opo+1);	
   	fprintf(fd, "Longitud: %d\n", n_opo);
   	fprintf(fd, "Causa de finalització: ");	
   	if(p_shared->fi1 == 0) fprintf(fd, "Mort propia\n\n");
   	else fprintf(fd, "Mort jugador humà\n\n");
   	signalS(id_sem_fit);
    }
    // ^^^ secció crítica accés al fitxer ^^^
  }
	
  return 0;
}
