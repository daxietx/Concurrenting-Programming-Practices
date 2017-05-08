This project implements parallel programming of the Game of Life in three ways.

1. Using multi-threads
	This part is implemented with pthread library. 
	User can assign the number of threads and the scale of the world in command line.
	Multiple threads are created to deal with the population calculation, and then join. After that,
multiple threads are created to deal with copy, and then join.
	The command of executing multi-threads program can be:
	./pthread [number_of_rows] [number_of_columns] [number_of_threads]
	./pthread [number_of_threads]
	
2. Using OpenMP
	This part is implemented with OpenMP library.
	User can assign the number of threads and the scale of the world in command line.
	In the code, the shared, private and reduction variables should be decided when dealing with the 
calculation and copy loops.
	The number of threads should be set using the command below:
	setenv OMP_NUM_THREADS [number_of_threads]
	The command of executing OpenMP program can be(after setting thread amount):
	./openmp [number_of_rows] [number_of_columns]
	./openmp [number_of_threads]

3. Using multi-process
	Several processes are forked and each of them handles a part of the world. For each process, there 
are two ghosts, which are used as the hint for next round's update. The child processes need to communicate 
with parent about the population amount and world data. And between child processes, they need to 
communicate about updating ghosts. Therefore, four kinds of pipes are created to handle the communication 
between processes:
	fdparent - pass data from child to parent
	fdchild - pass data from parent to child
	fdupper - used to update top ghost
	fdlower - used to update bottom ghost
	After initialize the world, in each iteration round parent process send command to all child processes(using 
fdparent) and each child process execute the received command.
	The command of executing OpenMP program can be(after setting thread amount):
	./process [number_of_rows] [number_of_columns] [number_of_threads]
	./process [number_of_threads]
