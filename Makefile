phextio: phext.o main.o
	gcc main.o phext.o -o phextio
main.o: main.c
	gcc -c -g main.c
libphext.o: phext.h phext.c
	gcc -c -g phext.c -o libphext.o
test: phextio phesql
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
	./phesql sql.phext select users 'user_id,username'
	./phesql sql.phext select users user_id 101
	./phesql sql.phext select users username bob
	./phesql sql.phext select users password epsilon
perf: phextio
	/bin/bash ./test-performance.sh
phesql: phext.o libphext.o phesql.c
	gcc phesql.c -c && gcc phesql.o libphext.o -o phesql
clean:
	rm -f *.o; rm -f phextio; rm -f phesql.o*; rm -f phesql
