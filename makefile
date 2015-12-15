all: prog4

prog4: Lab4Main.o Lab4Photo.o Lab4Clock.o
	gcc Lab4Main.o Lab4Photo.o Lab4Clock.o -o prog4

Lab4Main.o: Lab4Main.c
	gcc -c Lab4Main.c

Lab4Photo.o: Lab4Photo.c
	gcc -c Lab4Photo.c

Lab4Clock.o: Lab4Clock.c
	gcc -c Lab4Clock.c

clean:
	rm Lab4Main.o 
	rm Lab4Photo.o 
	rm Lab4Clock.o 
	rm prog4
