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
	$(MAKE) test060
perf: phextio
	/bin/bash ./test-performance.sh
phesql: phext.o libphext.o phesql.c
	gcc phesql.c -c && gcc phesql.o libphext.o -o phesql
phext_seal.o: phext.h phext_seal.h phext_seal.c
	gcc -c -g phext_seal.c -o phext_seal.o
test_phext: phext.o test_phext.c
	gcc -g phext.o test_phext.c -o test_phext
test_seal: phext.o phext_seal.o test_seal.c
	gcc -g phext.o phext_seal.o test_seal.c -o test_seal -llzma
test_fixtures: phext.o test_fixtures.c
	gcc -g phext.o test_fixtures.c -o test_fixtures
seal_tool: phext.o phext_seal.o seal_tool.c
	gcc -g phext.o phext_seal.o seal_tool.c -o seal_tool -llzma
test060: test_phext test_seal test_fixtures
	./test_phext
	./test_seal
	./test_fixtures tests/fixtures/chains
clean:
	rm -f *.o; rm -f phextio; rm -f phesql.o*; rm -f phesql; rm -f test_phext test_seal test_fixtures seal_tool
