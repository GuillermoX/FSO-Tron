/*****************************************************************************/
/*									     */
/*				     tron0.c				     */
/*									     */
/*  Programa inicial d'exemple per a les practiques 2 de FSO   	             */
/*     Es tracta del joc del tron: sobre un camp de joc rectangular, es      */
/*     mouen uns objectes que anomenarem 'trons' (amb o tancada). En aquesta */
/*     primera versio del joc, nomes hi ha un tron que controla l'usuari, i  */
/*     que representarem amb un '0', i un tron que controla l'ordinador, el  */
/*     qual es representara amb un '1'. Els trons son una especie de 'motos' */
/*     que quan es mouen deixen rastre (el caracter corresponent). L'usuari  */
/*     pot canviar la direccio de moviment del seu tron amb les tecles:      */
/*     'w' (adalt), 's' (abaix), 'd' (dreta) i 'a' (esquerra). El tron que   */
/*     controla l'ordinador es moura aleatoriament, canviant de direccio     */
/*     aleatoriament segons un parametre del programa (veure Arguments).     */
/*     El joc consisteix en que un tron intentara 'tancar' a l'altre tron.   */
/*     El primer tron que xoca contra un obstacle (sigui rastre seu o de     */
/*     l'altre tron), esborrara tot el seu rastre i perdra la partida.       */
/*									     */
/*  Arguments del programa:						     */
/*     per controlar la variabilitat del canvi de direccio, s'ha de propor-  */
/*     cionar com a primer argument un numero del 0 al 3, el qual indicara   */
/*     si els canvis s'han de produir molt frequentment (3 es el maxim) o    */
/*     poc frequentment, on 0 indica que nomes canviara per esquivar les     */
/*     parets.								     */
/*									     */
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment del menjacocos i dels fantasmes (en ms);           */
/*     el valor per defecte d'aquest parametre es 100 (1 decima de segon).   */
/*									     */
/*  Compilar i executar:					  	     */
/*     El programa invoca les funcions definides a "winsuport.c", les        */
/*     quals proporcionen una interficie senzilla per crear una finestra     */
/*     de text on es poden escriure caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				     */
/*									     */
/*	   $ gcc -c winsuport.c -o winsuport.o			     	     */
/*	   $ gcc tron0.c winsuport2.o -o tron0 -lcurses			     */
/*	   $ ./tron0 variabilitat [retard]				     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*	3  ==>  no hi ha prou memoria per crear les estructures dinamiques   */
/*									     */
/*****************************************************************************/

#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../winsuport2.h"		/* incloure definicions de funcions propies */
#include "../semafor.h"		/* incloure les funcions dels semafors */
#include "../memoria.h"

#define MAX_OPO 9		/* màxim nombre d'oponents */

				/* definir estructures d'informacio */
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

/* variables globals */

tron usu;   	   		/* informacio de l'usuari */
tron opo[MAX_OPO];		/* informacio dels oponents */
int num_opo = 0;		/* nombre d'oponents a crear */

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int retard;		/* valor del retard de moviment, en mil.lisegons */

pos *p_usu;			/* taula de posicions que van recorrent */
pos *p_opo[MAX_OPO];			/* els jugadors */
int n_usu = 0;			/* numero d'entrades en les taules de pos del usuari */
int n_opo[MAX_OPO];		/* numero d'entrades de la taula de pos de cada oponent */


/* memoria compartida */
void *p_map;			/* punter al mapa */
shared_t *p_shared;		/* punter a memoria compartida */


/* funcio per esborrar totes les posicions anteriors, sigui de l'usuari o */
/* de l'oponent */
void esborrar_posicions(pos p_pos[], int n_pos, int usuari)
{
  int i;
  for (i=n_pos-1; i>=0; i--)		/* de l'ultima cap a la primera */
  {	
    
    /* vvv secció crítica d'escriptura de pantalla vvv*/
    waitS(p_shared->id_sem_pant);
    win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);	/* esborra una pos. */
    if(usuari) win_update();	/* Si es l'usuari qui l'executa actualitza pantalla */
    signalS(p_shared->id_sem_pant);
    /* ^^^ secció crítica d'escriptura de pantalla ^^^*/
    win_retard(10);		/* un petit retard per simular el joc real */
  }
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
void inicialitza_joc(void)
{
  char strin[45];

  /* inicialitzar variables de finalització */
  p_shared->fi1 = 0;
  p_shared->fi2 = num_opo;

  usu.f = (n_fil-1)/2;
  usu.c = (n_col)/4;		/* fixa posicio i direccio inicial usuari */
  usu.d = 3;
  win_escricar(usu.f,usu.c,'0',INVERS);	/* escriu la primer posicio usuari */
  p_usu[n_usu].f = usu.f;		/* memoritza posicio inicial */
  p_usu[n_usu].c = usu.c;
  n_usu++;

  for(int i = 0; i < num_opo; i++)
  {
	  	n_opo[i] = 0;	
		
		float fila_div = ((float)(n_fil-1)/(float)(num_opo+1));
 		opo[i].f = fila_div + fila_div*i;
 		opo[i].c = (n_col*3)/4;		/* fixa posicio i direccio inicial oponent */
 		opo[i].d = 1;
 		win_escricar(opo[i].f,opo[i].c,i+1+'0',INVERS);	/* escriu la primer posicio oponent */
 		p_opo[i][n_opo[i]].f = opo[i].f;		/* memoritza posicio inicial */
 		p_opo[i][n_opo[i]].c = opo[i].c;
		n_opo[i] ++;
  }

  sprintf(strin,"Tecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  win_escristr(strin);
}


/* funcio per moure l'usuari una posicio, en funcio de la direccio de   */
/* moviment actual; retorna -1 si s'ha premut RETURN, 1 si ha xocat     */
/* contra alguna cosa, i 0 altrament */
void mou_usuari(void)
{
  char cars;
  tron seg;
  int tecla, retorn;
  int compt = 0;		/* comptador per executar el bucle de moviment cada temps retard */
  int modRetard = retard/10;	/* valor per realitzar el modul a compt i reiniciarlo */
  do
  {
	if(compt == 0)
	{	
		retorn = 0;
		tecla = win_gettec();
		if (tecla != 0)
		 switch (tecla)	/* modificar direccio usuari segons tecla */
		 {
		  case TEC_AMUNT:	usu.d = 0; break;
		  case TEC_ESQUER:	usu.d = 1; break;
		  case TEC_AVALL:	usu.d = 2; break;
		  case TEC_DRETA:	usu.d = 3; break;
		  case TEC_RETURN:	p_shared->fi1 = -1; break;
		 }
		seg.f = usu.f + df[usu.d];	/* calcular seguent posicio */
		seg.c = usu.c + dc[usu.d];
		cars = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
		if (cars == ' ')			/* si seguent posicio lliure */
		{
		  usu.f = seg.f; usu.c = seg.c;		/* actualitza posicio */
		
		  /* vvv secció crítica d'escriptura de pantalla vvv*/
		  waitS(p_shared->id_sem_pant);
		  win_escricar(usu.f,usu.c,'0',INVERS);	/* dibuixa bloc usuari */
		  signalS(p_shared->id_sem_pant);
		  /* ^^^ --------------------------------------- ^^^*/
		  p_usu[n_usu].f = usu.f;		/* memoritza posicio actual */
		  p_usu[n_usu].c = usu.c;
		  n_usu++;
		}
		else
		{ 
		  /* vvv Secció crítica variables globals vvv */
		  waitS(p_shared->id_sem_var);
		  p_shared->fi1 = 1;
		  signalS(p_shared->id_sem_var);
		  /* ^^^ ------------------------------- ^^^*/
		}
		
	}
	compt = (compt + 1) % modRetard;
	
	win_update();		/* s'actualiza la pantalla */
	win_retard(10);		/* s'actualitza cada 10 milisegons, independentment del retard de moviment del jugador */
  } while((p_shared->fi1 == 0) && (p_shared->fi2 != 0) && !retorn);

  esborrar_posicions(p_usu, n_usu, 1);

}

/* programa principal				    */
int main(int n_args, const char *ll_args[])
{

  int id_shared_mem;		/* variables de finalització */
  /*S'inicialitza la memoria compartida entre processos*/	
  id_shared_mem = ini_mem(sizeof(shared_t)); 
  p_shared = map_mem(id_shared_mem);
  
  int retwin;

  srand(getpid());		/* inicialitza numeros aleatoris */
  setbuf(stdout, NULL); 

  if (!(((n_args >= 2) && (n_args <= 4)) || (n_args == 6)))
  {	fprintf(stderr,"Comanda: ./tron0 variabilitat [retard] [nombre d'enemics]\n");
  	fprintf(stderr,"         on \'variabilitat\' indica la frequencia de canvi de direccio\n");
  	fprintf(stderr,"         de l'oponent: de 0 a 3 (0- gens variable, 3- molt variable),\n");
  	fprintf(stderr,"         \'retard\' es el numero de mil.lisegons que s'espera entre dos\n");
  	fprintf(stderr,"         moviments de cada jugador (minim 10, maxim 1000, 100 per defecte).\n");
  	fprintf(stderr,"         \'nombre d'enemics\' es el numero d'enemics (per defecte 1).\n");
  	exit(1);
  }

  num_opo = atoi(ll_args[1]);
  if (num_opo < 1) num_opo = 1;
  if (num_opo > MAX_OPO) num_opo = MAX_OPO;

  int varia;
  if (n_args >= 3)
  {
 	varia = atoi(ll_args[2]);	/* obtenir parametre de variabilitat */
 	if (varia < 0) varia = 0;	/* verificar limits */
 	if (varia > 3) varia = 3;
  }
  else varia = 0;
  /* guardar info variabilitat a mem compartida */
  p_shared->varia = varia;
	
  int log_file = 0;			/* booleà per saber si hi ha un fitxer Log */
  if (n_args >= 4) log_file = 1;
	
  int retard_min, retard_max;
  if (n_args >= 6)		/* si s'ha especificat parametre de retard */
  {	retard_min = atoi(ll_args[4]);	/* convertir-lo a enter */
  	if (retard_min < 10) retard = 10;	/* verificar limits */
  	if (retard_min > 1000) retard = 1000;

  	retard_max = atoi(ll_args[5]);	/* convertir-lo a enter */
  	if (retard_max <= retard_min) retard_max = retard_min + retard_min/4;	/* verificar limits */

	retard = 100;
  }
  else{
	retard = 100;		/* altrament, fixar retard per defecte */
  	retard_min = retard;	/* es fixen els retards mínim i màxim pels oponents*/
  	retard_max = retard + retard/3;
  }
  /* guardar la info dels retard a memoria compartida */
  p_shared->retard_min = retard_min;
  p_shared->retard_max = retard_max;

  printf("Joc del Tron\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  printf("prem una tecla per continuar:\n");
  getchar();
	

  int n_fil, n_col;		/* dimensions del camp de joc */
  n_fil = 0; n_col = 0;		/* demanarem dimensions de taulell maximes */
  retwin = win_ini(&n_fil,&n_col,'+',INVERS);	/* intenta crear taulell */
  /* guardar num files y columnes a memoria compartida */
  p_shared->n_fil = n_fil;
  p_shared->n_col = n_col;

  if (retwin < 0)	/* si no pot crear l'entorn de joc amb les curses */
  { fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin)
    {	case -1: fprintf(stderr,"camp de joc ja creat!\n"); break;
	case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n"); break;
	case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n"); break;
	case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n"); break;
     }
     exit(2);
  }

  /* si s'ha creat el taulell reservem memoria compartida pel mapa i guardem l'ID a la memoria compartida */
  p_shared->id_map_mem = ini_mem(retwin);

  p_map = map_mem(p_shared->id_map_mem);
  win_set(p_map, n_fil, n_col);
  

  p_usu = calloc(n_fil*n_col/2, sizeof(pos));	/* demana memoria dinamica */
  for(int i = 0; i < num_opo; i++)
  {
 	p_opo[i] = calloc(n_fil*n_col/2, sizeof(pos));	/* per a les posicions ant. */
 	if (!p_usu || !p_opo[i])	/* si no hi ha prou memoria per als vectors de pos. */
 	{ win_fi();				/* tanca les curses */
 	  if (p_usu) free(p_usu);
 	  if (p_opo[i]) free(p_opo[i]);	   /* allibera el que hagi pogut obtenir */
 	  fprintf(stderr,"Error en alocatacion de memoria dinamica.\n");
 	  exit(3);
 	}
  }
			/* Fins aqui tot ha anat be! */

  inicialitza_joc();

	
  FILE *fd;  
  if(log_file)
  {

 	fd = fopen(ll_args[3], "w");	/* obrir un canal amb l'arxiu on escriuran els fills */
 	if(fd == NULL)
 	{
 	        perror("Unable to open file");
 	        log_file = 0;
 	}
	else
	{	
 		setbuf(fd, NULL);	/* deshabilitem el buffer de l'escriptura a l'arxiu */
	}
  }
	
  p_shared->id_sem_pant = ini_sem(1);		/*inicialitzem el semafor de la pantalla */
  p_shared->id_sem_fit = ini_sem(1);
  p_shared->id_sem_var = ini_sem(1);
	
  pid_t pid[MAX_OPO];			/* pid de cada process oponent */

  for(int i = 0; i < num_opo; i++)	/*Per cada tron oponent*/
  {	
	//printf("idMem: %d, index: %d, fila: %d, col: %d \n", id_shared_mem, num_opo, p_opo[i][0].c, p_opo[i][0].f);	
	/*TODO: create childs with execlp();
  	pid[i] = fork(); 		//Es crea un process nou

 	if(pid[i] == 0)			//Si es el process fill (el oponent)
 	{
 	      mou_oponent(i);		
	      if(log_file)
	      {
	     	// el process escriu informació al fitxer abans de finalitzar
		// vvv secció crítica accés al fitxer vvv 
		waitS(id_sem_fit);
	     	fprintf(fd, "Fill id: %d, index: %d\n", getpid(), i+1);	
	     	fprintf(fd, "Longitud: %d\n", n_opo[i]);
	     	fprintf(fd, "Causa de finalització:\n");	
		if(fi->fi1 == 0) fprintf(fd, "Mort propia\n\n");
		else fprintf(fd, "Mort jugador humà\n\n");
		signalS(id_sem_fit);
		// ^^^ secció crítica accés al fitxer ^^^
	      }
	      exit(1);			//Quan l'oponent mor s'acaba el process fill oponent
 	}
	*/
  } 
  
  
  mou_usuari();				/*Per l'usuari s'executa un bucle de moviment*/
  for(int i = 0; i < num_opo; i++)	/*Quan mor l'usuari es comprova si han mort tots els enemics*/
  {	
	int retorn;
  	//waitpid(pid[i], &retorn, 0);
  }

  win_fi();
  
  

  if (p_shared->fi1 == -1) printf("S'ha aturat el joc amb tecla RETURN!\n\n");
  else { if (p_shared->fi1 == 0) printf("Ha guanyat l'usuari!\n\n");
  	else printf("Ha guanyat l'ordinador!\n\n"); }	
 

  free(p_usu);

  for(int i = 0; i < num_opo; i++)
  {
	free(p_opo[i]);	
  }

  return(0);
}
