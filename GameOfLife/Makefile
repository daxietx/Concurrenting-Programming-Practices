#setenv OMP_NUM_THREADS 8
CFLAG=gcc -Wall -std=c99 -pedantic
sequent: proj3_seq.c
	$(CFLAG) proj3_seq.c -o sequent
pthread: proj3_Pthread.c
	$(CFLAG) -lpthread proj3_Pthread.c -o pthread
openmp: proj3_OpenMP.c
	$(CFLAG) -fopenmp proj3_OpenMP.c -o openmp		
process: process1.c
	$(CFLAG) process1.c -o process
generate: sequent
	echo generate correct output files
	./sequent 10
	mv final_world000.txt 1p.txt
	./sequent 1000 1000
	mv final_world000.txt 2p.txt 
demo: pthread openmp process
	echo test pthread
	eval time ./pthread 1000 1000 7 > /dev/null
	diff 2p.txt final_world000.txt
	eval time ./pthread 79 > /dev/null
	diff 1p.txt final_world000.txt
	
	echo test openmp
	eval time ./openmp 4 > /dev/null
	diff 1p.txt final_world000.txt
	eval time ./openmp 1000 1000 > /dev/null
	diff 2p.txt final_world000.txt
	
	echo test process
	eval time ./process 1000 1000 3 > /dev/null
	diff 2p.txt final_world000.txt
	eval time ./process 4 > /dev/null
	diff 1p.txt final_world000.txt
	
clean:
	rm -f pthread
	rm -f openmp
	rm -f process
	rm -f sequent
	rm -f final_world000.txt
	