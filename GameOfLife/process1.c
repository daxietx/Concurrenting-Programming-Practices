#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define MAX_N 8192

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

char w[MAX_N][MAX_N];

int w_X, w_Y;
int Nproc;
//char cmd[50];

int init1(int block, int lastblock, int iter, int x, int y, int** data)  // set the first row and column 1, the others 0
{
	int count = 0;
	data[x][y] = 0;
	if(iter == 0) // the first process
	{
		if(x==1) // assign the pure data part
		{
			data[x][y] = 1;
			count++;
		}
		else if((x>0) && (x<(block+1)) && (y==0))
		{
			data[x][y] = 1;
			count++;
		}
		else if(x==block+1 && y==0)
			data[x][y] = 1;
			
	}
	else if(iter == Nproc-1) // the last process
	{
		if(x==0 && y==0)
			data[x][y] = 1;
		else if((x>0) && (x<lastblock+1) && (y==0))
		{
			data[x][y] = 1;
			count++;
		}
	}
	else
	{
		if(x==0 && y==0)
			data[x][y] = 1;
		else if((x>0) && (x<block+1) && (y==0))
		{
			data[x][y] = 1;
			count++;
		}
		else if(x==block+1 && y==0)
			data[x][y] = 1;
	}
	return count;
}

void test_init2()
{
	int i, j;
	for (i=0; i<w_X;i++)
		for (j=0; j<w_Y; j++)
			w[i][j] = 0;
	w[0][3] = 1;
	w[1][3] = 1;
	w[2][1] = 1;
	w[3][0] = w[3][1] = w[3][2] = w[4][1] = w[5][1] = 1;
}

int init2(int datasize, int iter, int** data)
{
	int count = 0;
	if(iter==0) // the first process
	{
		for(int j=0;j<w_Y;j++)
		{
			data[0][j] = 0; //upper ghost
			data[datasize-1][j] = w[(iter+1)*(datasize-2)+1][j]; //lower ghost
		}
			
		for(int i=1;i<datasize-1;i++)
		{
			for(int j=0;j<w_Y;j++)
			{
				data[i][j] = w[iter*(datasize-2)-1+i][j];
			}
		}				
	}
	else if(iter==Nproc-1) // the last process
	{
		for(int j=0;j<w_Y;j++)
		{
			data[0][j] = w[(iter-1)*(datasize-2)+1][j]; //upper ghost
			data[datasize-1][j] = 0; //lower ghost
		}	
		for(int i=1;i<datasize-1;i++)
		{
			for(int j=0;j<w_Y;j++)
			{
				data[i][j] = w[iter*(datasize-2)-1+i][j];
			}
		}		
	}
	else
	{
		for(int j=0;j<w_Y;j++)
		{
			data[0][j] = w[(iter-1)*(datasize-2)+1][j]; //upper ghost
			data[datasize-1][j] = w[(iter+1)*(datasize-2)+1][j]; //lower ghost
		}
			
		for(int i=1;i<datasize-1;i++)
		{
			for(int j=0;j<w_Y;j++)
			{
				data[i][j] = w[iter*(datasize-2)-1+i][j];
			}
		}		
	}
	
	for(int i=1;i<datasize-1;i++)
	{
		for(int j=0;j<w_Y;j++)
		{
			if(data[i][j] == 1) count++;
		}			
	}	
		
	return count;	
}

void print_world(int** fdchild,int** fdparent)
{
	int iter;
	for(iter=0;iter<Nproc;iter++)
	{
		// send child output to the file
		write(fdchild[iter][1],"output",sizeof("output"));		  
	}
	for(iter=0;iter<Nproc;iter++)
	{	
		int size,element;
		read(fdparent[iter][0],&size,sizeof(int));
		for(int i=0;i<size;i++)
		{
			for(int j=0;j<w_Y;j++)
			{
				read(fdparent[iter][0],&element,sizeof(int));
				printf("%d",element);					
			}
			printf("\n");
		}		
	}	
}

void printdata(int iter,int **data,int x,int y)
{
	printf("process%d\n",iter);
	for(int i=0;i<x;i++)
	{
		for(int j=0;j<y;j++)
		{
			printf("%d",data[i][j]);
		}
		printf("\n");
	}
}

int readex(int fd, char* buf,size_t size)
{
	int eachread;
	int real = 0;
	while(real<size)
	{
		do
		{
			eachread = read(fd,buf+real,size-real);
		} while(eachread==-1 && errno==EINTR);
		real += eachread;	
	}
	return real;
}

int writeex(int fd, char* buf,size_t size)
{
	int eachwrite;
	int real = 0;
	while(real<size)
	{
		do
		{
			eachwrite = write(fd,buf+real,size-real);
		}while(eachwrite==-1 && errno==EINTR);
		real += eachwrite;	
	}	
	return real;
}

int neighborcount(int datasize, int x, int y, int** data)
{
  int count = 0;

  if ((x<0) || (x >=datasize)) {
    printf("neighborcount: (%d %d) out of bound (0..%d, 0..%d).\n", x,y,
	   datasize,w_Y);
    exit(0);
  }
  if ((y<0) || (y >=w_Y)) {
    printf("neighborcount: (%d %d) out of bound (0..%d, 0..%d).\n", x,y,
	   datasize,w_Y);
    exit(0);
  }

  if (y==0) // left bound
  {
    if (x == 0) { // the upper left element
      count = data[x][y+1] + data[x+1][y] + data[x+1][y+1];
    } else if (x == datasize-1) { // the lower left element
      count = data[x][y+1] + data[x-1][y] + data[x-1][y+1];
    } else { // the other left bound elements	
      count = data[x-1][y] + data[x+1][y] + data[x-1][y+1] + data[x][y+1] + data[x+1][y+1];
    }
  }

 else if (y == w_Y -1) // right bound
  {
    if (x == 0) { // the upper right element
      count = data[x][y-1] + data[x+1][y-1] + data[x+1][y];
    } else if (x == datasize-1) { // the lower right element
      count = data[x][y-1] + data[x-1][y] + data[x-1][y-1];
    } else {// the other right bound elements
      count = data[x-1][y] + data[x+1][y] + data[x-1][y-1] + data[x][y-1] + data[x+1][y-1];
    }
  }


  else
  { /* x is in the middle */
    if (x == 0) { // the upper bound elements
      count = data[x][y-1] + data[x][y+1] + data[x+1][y-1] + data[x+1][y] + data[x+1][y+1];
    } else if (x == datasize-1) { // the lower bound elements
      count = data[x][y-1] + data[x][y+1] + data[x-1][y-1] + data[x-1][y] + data[x-1][y+1];
    } else {// the middle elements
      count = data[x-1][y-1] + data[x][y-1] + data[x+1][y-1] + data[x-1][y] +data[x+1][y]
              + data[x-1][y+1] + data[x][y+1] + data[x+1][y+1];
    }
  }
  return count;
}

int main(int argc, char *argv[])
{
  int iter = 0;
  int init_count;
  int count = 0;
  int block,lastblock;
  int **data;  // the pointer to the data memory
  int **newdata;
  char buf[MAX_N];
  char ghost[MAX_N];
  int useinit2 = 0;

  if (argc == 1) {
    printf("Usage: ./a.out w_X w_Y\n");
    exit(0);
  } else if (argc == 2)
  {
	  useinit2 = 1;
	  Nproc = atoi(argv[1]);
	  w_X = 6;
	  w_Y = 4;
	  test_init2();
  }  
  else /* more than three parameters */
  {
	  w_X = atoi(argv[1]); //get the row number of the world
	  w_Y = atoi(argv[2]); //get the column number of the world
	  Nproc = atoi(argv[3]); //get the number of processes
  }

//---------------------------------------------------------------------
  if(w_X < Nproc)
  {
	  //printf("The assigned number of process is too large.\n");
	  Nproc = w_X;
  }
  //else // set the handling amount of each process
  {
	  block = w_X / Nproc;
	  lastblock = w_X - (Nproc-1)*block;
	  //printf("block: %d lastblock: %d\n",block,lastblock);
  }
  
  int fd[2]; //pipe 
  int** fdparent = malloc(Nproc*sizeof(fd));  // child to parent 
  int** fdchild = malloc(Nproc*sizeof(fd)); // parent to child
  int** fdupper = malloc(Nproc*sizeof(fd)); // pipe to previous process
  int** fdlower = malloc(Nproc*sizeof(fd)); // pipe to post process
  
  //--------------create pipes-----------------------------------
  for(iter=0;iter<Nproc;iter++)
  {	  
	  fdparent[iter] = malloc(2*sizeof(int));
	  fdchild[iter] = malloc(2*sizeof(int));
	  fdupper[iter] = malloc(2*sizeof(int));
	  fdlower[iter] = malloc(2*sizeof(int));
	  
	  if(pipe(fdparent[iter])<0)
	  {
		  printf("Fail to create fdparent %d.\n",iter);
		  return 1;
	  }
	  if(pipe(fdchild[iter])<0)
	  {
		  printf("Fail to create fdchild %d.\n",iter);
		  return 1;
	  }
	  if(pipe(fdupper[iter])<0)
	  {
		  printf("Fail to create fdupper %d.\n",iter);
		  return 1;
	  }
	  if(pipe(fdlower[iter])<0)
	  {
		  printf("Fail to create fdlower %d.\n",iter);
		  return 1;
	  }
  }
  
  //--------------create processes-----------------------------------
  for(iter=0;iter<Nproc;iter++)
  {	
	  int pid,datasize;
	  pid = fork();
	  if(pid<0)
	  {
		  printf("Fail to fork.\n");
		  return 1;
	  }
	  else if(pid==0) // the child process
	  {

		  if(iter != Nproc-1) //not the last process
			  datasize = block+2;	//block data (include ghost)
		  else
			  datasize = lastblock+2;

		  data = (int**)malloc((datasize)*sizeof(int*)); // allocate local data space 
		  newdata = (int**)malloc((datasize)*sizeof(int*));
		  for(int i=0;i<datasize;i++)
		  {
			  data[i] = (int*)malloc(w_Y*sizeof(int)); // allocate local data space
			  newdata[i] = (int*)malloc(w_Y*sizeof(int));
			  if(useinit2==0)
			  {
				  for(int j=0;j<w_Y;j++)
				  {
					  //-------------initiate the world usig init1--------------------------					  
						count += init1(block,lastblock,iter,i,j,data);	  
				  }		
			  }			  
		  }	
		  //-------------initiate the world usig init2--------------------------
		  if(useinit2==1)
		  {
			  count = init2(datasize,iter,data);		  
		  }
		  
		  write(fdparent[iter][1],"inited",sizeof("inited"));
		  write(fdparent[iter][1],&count,sizeof(int)); // send each count to parent
		  read(fdchild[iter][0],buf,sizeof("done")); // receive from parent: counts init population done	
		  if (DEBUG_LEVEL > 10)
		  {
			  read(fdchild[iter][0],buf,sizeof(buf));
			  int size = datasize-2;
			  write(fdparent[iter][1],&size,sizeof(int)); // inform parent: the datasize of this child
			  for(int i=1;i<datasize-1;i++) // only pass the valid data part
			  {
				  for(int j=0;j<w_Y;j++)
				  {
					  write(fdparent[iter][1],&data[i][j],sizeof(int));
				  }
			  }			  
		  }
		//--------------------initiate over-------------------------------------------------------  
		 
		  while(read(fdchild[iter][0],buf,sizeof(buf)))
		  {
			  if(strncmp(buf,"cal",3)==0)  //get the cmd: do calculation
			  {
				  for(int i=0;i<datasize;i++)
				  {
					  for(int j=0;j<w_Y;j++)
					  {
						  //printf("datasize:%d i:%d j:%d .\n",datasize,i,j);
						  int c = neighborcount(datasize,i,j,data);
					      
						  if(c<=1) 
							  newdata[i][j] = 0;
						  else if (c>=4) 
							  newdata[i][j] = 0;
						  else if(c==3) 
						  {
							  newdata[i][j] = 1;
							  //printf("c==3 newdata:%d\n",newdata[i][j]);
						  }							  
						  else 
						  {
							   newdata[i][j] = data[i][j];
						  }
							 
					  }
				  }
				  writeex(fdparent[iter][1],"done",sizeof("done")); // inform parent: calculation done
			  }
			  else if(strncmp(buf,"count",5)==0) //get the cmd: count the population
			  {
				  count = 0;
				  for(int i=0;i<datasize;i++)
				  {
					  for(int j=0;j<w_Y;j++)
					  {
						  data[i][j] = newdata[i][j];
						  if(i!=0 && i!=datasize-1)
						  {
							  if(data[i][j] == 1)
							  count++;							  
						  }						  
					  }
				  }
				  write(fdparent[iter][1],&count,sizeof(int)); // inform parent: count number
			  }  			  
			  else if(strncmp(buf,"upper",5)==0) //get the cmd: update the upper ghost
			  {
				  if(iter==0)     // the upper ghost of the first process is always 0
				  {
					  for(int j=0;j<w_Y;j++)
					  {
						  data[0][j] = 0;
						  ghost[j] = data[datasize-2][j];
					  }
					  writeex(fdupper[iter][1],ghost,w_Y*sizeof(int));
				  }
				  else // the other processes update the upper ghost from previous process's last valid data row
				  {
					  readex(fdupper[iter-1][0],buf,w_Y*sizeof(int));
					  for(int j=0;j<w_Y;j++)
					  {
						  data[0][j] = buf[j];
						  ghost[j] = data[datasize-2][j];
					  }					
					  if(iter!=Nproc-1)  // the last process only read, doesn't write
						writeex(fdupper[iter][1],ghost,w_Y*sizeof(int));				  
				  }
				  writeex(fdparent[iter][1],"done",sizeof("done")); // inform parent: update upper ghost done
			  }
			  else if(strncmp(buf,"lower",5)==0) //get the cmd:update the lower ghost
			  {
				  if(iter==Nproc-1) // the lower ghost of the last process is always 0
				  {
					  for(int j=0;j<w_Y;j++)
					  {
						  data[datasize-1][j] = 0;
						  ghost[j] = data[1][j];
					  }
					  writeex(fdlower[iter-1][1],ghost,w_Y*sizeof(int));
				  }
				  else
				  {
					  readex(fdlower[iter][0],buf,w_Y*sizeof(int));
					  for(int j=0;j<w_Y;j++)
					  {
						  data[datasize-1][j] = buf[j];
						  ghost[j] = data[0][j];
					  }
					  if(iter!=0)
						  writeex(fdlower[iter-1][1],ghost,w_Y*sizeof(int));
				  }
				  writeex(fdparent[iter][1],"done",sizeof("done")); // inform parent: update lower ghost done
			  }
			  else if(strncmp(buf,"output",6)==0) //get the cmd: print whole world into file
			  {
				  //printf("%s",buf);
				  int size = datasize-2;
				  write(fdparent[iter][1],&size,sizeof(int)); // inform parent: the datasize of this child
				  for(int i=1;i<datasize-1;i++) // only pass the valid data part
				  {
					  for(int j=0;j<w_Y;j++)
					  {
						  write(fdparent[iter][1],&data[i][j],sizeof(int));
					  }
				  }
			  }
			  else if(strncmp(buf,"stop",4)==0) //get out of the while loop
				break;
		  }	
		  for(int i=0;i<Nproc;i++)
		  {
			  close(fdparent[i][0]);
			  close(fdparent[i][1]);
			  close(fdchild[i][0]);
			  close(fdchild[i][1]);
			  close(fdupper[i][0]);
			  close(fdupper[i][1]);
			  close(fdlower[i][0]);
			  close(fdlower[i][1]);
			  free(fdparent[i]);
			  free(fdchild[i]);
			  free(fdupper[i]);
			  free(fdlower[i]);
		  }
		  free(fdparent);
		  free(fdchild);
		  free(fdupper);
		  free(fdlower);

		  exit(0);
	  }
  }

  // parent work
  //------------------handling initialization-------------------
  for(iter=0;iter<Nproc;iter++)
  {
	  read(fdparent[iter][0],buf,sizeof("inited"));
  }
  
  init_count= 0;
  for(iter=0;iter<Nproc;iter++)
  {
	  int a;
	  read(fdparent[iter][0], &a, sizeof(int));
	  init_count += a;
  }
  for(iter=0;iter<Nproc;iter++) //handle init world done
  {
	  write(fdchild[iter][1],"done",sizeof("done"));
  }
  
  printf("initial world, population count: %d\n", init_count);
  if (DEBUG_LEVEL > 10) print_world(fdchild,fdparent);	
  
  count = init_count;
  //-----------------------start update loop---------------------------------
  for (int it = 0; (it < 200) && (count < 50*init_count) && 
	 (count > init_count / 50); it ++)
  {
	  for(iter=0;iter<Nproc;iter++)
	  {
		  // send childs calculate the current state
		   writeex(fdchild[iter][1],"cal",sizeof("cal"));		   
	  }
	 
	  for(iter=0;iter<Nproc;iter++)
	  {
		  // get the feedback from child that calculate done
		  readex(fdparent[iter][0],buf,sizeof("done"));
	  }
	  
	  for(iter=0;iter<Nproc;iter++)
	  {
		  //send child count the population
		  writeex(fdchild[iter][1],"count",sizeof("count"));	  
	  }
	  
	  count = 0;
	  for(iter=0;iter<Nproc;iter++)
	  {
		  //get the feedback from child of the count number
		  int a;	  
		  read(fdparent[iter][0],&a,sizeof(int));
		  //printf("iter:%d %d\n",iter,a);
		  count += a; 
	  }	  
	  printf("iter = %d, population count = %d\n", it, count);
	  	  
	  for(iter=0;iter<Nproc;iter++)
	  {
		  //send child update upper ghost
		  writeex(fdchild[iter][1],"upper",sizeof("upper"));	  
	  }
	  
	  for(iter=0;iter<Nproc;iter++)
	  {
		  //get the feedback from child that upper ghost update done
		  readex(fdparent[iter][0],buf,sizeof("done"));
	  }	  

	  for(iter=0;iter<Nproc;iter++)
	  {
		  //send child update lower ghost
		  writeex(fdchild[iter][1],"lower",sizeof("lower"));	  
	  }
	  
	  for(iter=0;iter<Nproc;iter++)
	  {
		  //get the feedback from child that lower ghost update done
		  readex(fdparent[iter][0],buf,sizeof("done"));
	  }	 
	  
	  if (DEBUG_LEVEL > 10) print_world(fdchild,fdparent);	   	 
  }
  //--------------------------output the results------------------------------  
  {
	for(iter=0;iter<Nproc;iter++)
	{
		// send child output to the file
		write(fdchild[iter][1],"output",sizeof("output"));
	}
    FILE *fd;
    if ((fd = fopen("final_world000.txt", "w")) != NULL)
	{
	  for(iter=0;iter<Nproc;iter++)
	  {	
		int size,element;
		read(fdparent[iter][0],&size,sizeof(int));
		for(int i=0;i<size;i++)
		{
			for(int j=0;j<w_Y;j++)
			{
				read(fdparent[iter][0],&element,sizeof(int));
				fprintf(fd, "%d", element);
			}
			fprintf(fd, "\n");
		}		
      }
    } 
	else 
	{
      printf("Can't open file final_world000.txt\n");
      exit(1);
    }
  }
  //--------------------------stop the processes-------------------------------
  for(iter=0;iter<Nproc;iter++)
  {
	  write(fdchild[iter][1],"stop",sizeof("stop"));
	  wait(NULL);
  }
  
  for(int i=0;i<Nproc;i++)
  {
	  close(fdparent[i][0]);
	  close(fdparent[i][1]);
	  close(fdchild[i][0]);
	  close(fdchild[i][1]);
	  close(fdupper[i][0]);
	  close(fdupper[i][1]);
	  close(fdlower[i][0]);
	  close(fdlower[i][1]);
	  free(fdparent[i]);
	  free(fdchild[i]);
	  free(fdupper[i]);
	  free(fdlower[i]);
  }
  free(fdparent);
  free(fdchild);
  free(fdupper);
  free(fdlower);  
  
  return 0;
}
