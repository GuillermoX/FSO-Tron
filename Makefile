all : winsuport.o tron0 tron1 

winsuport2.o : winsuport2.c winsuport2.h
	gcc -Wall -c winsuport2.c -o winsuport2.o

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

semafor.o : semafor.c semafor.h
	gcc -Wall -c semafor.c -o semafor.o

memoria.o : memoria.c memoria.h
	gcc -Wall -c memoria.c -o memoria.o

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

clean: 
	rm winsuport.o tron0 tron1
