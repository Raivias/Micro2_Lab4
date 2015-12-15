all: prog4

prog4: Lab4Thread.o Lab4Photo.o Lab4Clock.o
	gcc Lab4Thread.o Lab4Photo.o Lab4Clock.o -o prog4

Lab4Main.o: Lab4Thread.c
	gcc -c Lab4Thread.c

Lab4Photo.o: Lab4Photo.c
	gcc -c Lab4Photo.c

Lab4Clock.o: Lab4Clock.c
	gcc -c Lab4Clock.c

clean:
	rm *.o 
	rm prog4
