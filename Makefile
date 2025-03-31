all : winsuport.o tron0 tron1 

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

semafor.o : semafor.c semafor.h
	gcc -Wall -c semafor.c -o semafor.o

tron0 : tron0.c winsuport.o winsuport.h
	gcc -Wall tron0.c winsuport.o -o tron0 -lcurses

tron1 : tron1.c winsuport.o winsuport.h
	gcc -Wall tron1.c winsuport.o -o tron1 -lcurses

tron2 : tron2.c winsuport.o winsuport.h
	gcc -Wall tron2.c winsuport.o semafor.o -o tron2 -lcurses

clean: 
	rm winsuport.o tron0 tron1
