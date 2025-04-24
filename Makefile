all : winsuport.o tron0 tron1 

winsuport2.o : winsuport2.c winsuport2.h
	gcc -Wall -c winsuport2.c -o winsuport2.o

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

semafor.o : semafor.c semafor.h
	gcc -Wall -c semafor.c -o semafor.o

memoria.o : memoria.c memoria.h
	gcc -Wall -c memoria.c -o memoria.o

missatge.o : missatge.c missatge.h
	gcc -Wall -c missatge.c -o missatge.o

tron0 : tron0.c winsuport.o winsuport.h
	gcc -Wall tron0.c winsuport.o -o tron0 -lcurses

tron1 : tron1.c winsuport.o winsuport.h
	gcc -Wall tron1.c winsuport.o -o tron1 -lcurses

tron2 : tron2.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h
	gcc -Wall tron2.c winsuport2.o semafor.o memoria.o -o tron2 -lcurses


tron3/oponent3 : tron3/oponent3.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h
	gcc -Wall tron3/oponent3.c winsuport2.o semafor.o memoria.o -o tron3/oponent3 -lcurses

tron3/tron3 : tron3/oponent3 tron3/tron3.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h
	gcc -Wall tron3/tron3.c winsuport2.o semafor.o memoria.o -o tron3/tron3 -lcurses


tron4/oponent4 : tron4/oponent4.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h
	gcc -Wall tron4/oponent4.c winsuport2.o semafor.o memoria.o -o tron4/oponent4 -lcurses

tron4/tron4 : tron4/oponent4 tron4/tron4.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h
	gcc -Wall tron4/tron4.c winsuport2.o semafor.o memoria.o -o tron4/tron4 -lcurses -lpthread

tron5/oponent5 : tron5/oponent5.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h missatge.o missatge.h
	gcc -Wall tron5/oponent5.c winsuport2.o semafor.o memoria.o missatge.o -o tron5/oponent5 -lcurses

tron5/tron5 : tron5/oponent5 tron5/tron5.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h missatge.o missatge.h
	gcc -Wall tron5/tron5.c winsuport2.o semafor.o memoria.o missatge.o -o tron5/tron5 -lcurses -lpthread

tron6/oponent6 : tron6/oponent6.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h missatge.o missatge.h
	gcc -Wall tron6/oponent6.c winsuport2.o semafor.o memoria.o missatge.o -o tron6/oponent6 -lcurses

tron6/tron6 : tron6/oponent6 tron6/tron6.c winsuport2.o winsuport2.h memoria.o memoria.h semafor.o semafor.h missatge.o missatge.h
	gcc -Wall tron6/tron6.c winsuport2.o semafor.o memoria.o missatge.o -o tron6/tron6 -lcurses -lpthread

clean: 
	rm winsuport.o tron0 tron1
