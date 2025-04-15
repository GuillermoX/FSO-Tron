
#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../winsuport2.h"		/* incloure definicions de funcions propies */
#include "../semafor.h"		/* incloure les funcions dels semafors */
#include "../memoria.h"

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

	int id_map_mem;			/* ID de la zona de memoria compartida del mapa */
} shared_t;

/* funcio per moure un oponent una posicio; retorna 1 si l'oponent xoca */
/* contra alguna cosa, 0 altrament					*/
int main(int argc, char **argv)
{
  /* obtenir punter a memoria compartida */
  int id_shared_mem = atoi(argv[1]);
  shared_t *p_shared = map_mem(id_shared_mem);
  
  /* obtenir index */
  int index = atoi(argv[2]);


  pos *p_tron;

  char cars;
  tron seg;
  int k, vk, nd, vd[3];
  int canvi = 0;
  int retorn = 0;
  final_t *fi = map_mem(id_final_mem);
  
do
  { 
 	seg.f = opo[index].f + df[opo[index].d];	/* calcular seguent posicio */
 	seg.c = opo[index].c + dc[opo[index].d];
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
 	      vk = (opo[index].d + k) % 4;		/* nova direccio */
 	      if (vk < 0) vk += 4;		/* corregeix negatius */
 	      seg.f = opo[index].f + df[vk];		/* calcular posicio en la nova dir.*/
 	      seg.c = opo[index].c + dc[vk];
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
 		opo[index].d = vd[0];			/* li assigna aquesta */
 	    else				/* altrament */
 	  	opo[index].d = vd[rand() % nd];	/* segueix una dir. aleatoria */
 	  }
 	}
 	if (retorn == 0)		/* si no ha col.lisionat amb res */
 	{
 	  opo[index].f = opo[index].f + df[opo[index].d];			/* actualitza posicio */
 	  opo[index].c = opo[index].c + dc[opo[index].d];
	  /* vvv secció crítica d'escriptura de pantalla vvv*/
	  waitS(id_sem_pant);
 	  win_escricar(opo[index].f,opo[index].c,index+1+'0',INVERS);	/* dibuixa bloc oponent */
	  signalS(id_sem_pant);
	  /* ^^^ secció crítica d'escriptura de pantalla ^^^ */
 	  p_opo[index][n_opo[index]].f = opo[index].f;			/* memoritza posicio actual */
 	  p_opo[index][n_opo[index]].c = opo[index].c;
 	  n_opo[index]++;
 	}
 	else
	{
		/* vvv Secció crítica variables globals vvv */
		waitS(id_sem_var);
		fi->fi2 = fi->fi2-1;
		signalS(id_sem_var);
		/* ^^^----------------------------------^^^ */
	}
	
	int retard_aleat = retard_min + (rand() % (retard_max-retard_min)); 		
	win_retard(retard_aleat);
  }while((fi->fi1 == 0) && (fi->fi2 != 0) && (retorn == 0));


  esborrar_posicions(p_opo[index], n_opo[index], 0);

}
