phextio: phext.o main.o
	gcc main.o phext.o -o phextio
main.o: main.c
	gcc -c -g main.c
phext.o: phext.h phext.c
	gcc -c -g phext.c
test: phextio
	./phextio 1.1.1/1.1.1/1.1.1
	./phextio 1.1.1/1.1.1/1.1.2
	./phextio 1.1.1/1.1.1/1.1.3
	./phextio 1.1.1/1.1.1/1.2.2
	./phextio 1.1.1/1.1.1/2.2.2
	./phextio 1.1.1/1.1.2/2.2.2
	./phextio 1.1.1/1.2.2/2.2.2
	./phextio 1.1.1/2.2.2/2.2.2
	./phextio 1.1.2/2.2.2/2.2.2
	./phextio 1.2.2/2.2.2/2.2.2
	./phextio 2.1.1/1.1.1/1.1.1	
	./phextio 2.2.2/2.2.2/2.2.2
	./phextio 2.2.2/2.2.2/2.2.5
	./phextio 16.7.5/12.1.3/42.17.16	
	./phextio 16.7.5/12.1.3/42.17.17
	./phextio 13.13.13/13.13.13/13.13.13
clean:
	rm -f *.o; rm -f phextio
