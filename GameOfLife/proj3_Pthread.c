/* 
 * Created 06/20/2015 by Xin Yuan for the COP5570 class
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_N 8192

#ifndef DEBUG_LEVEL 
#define DEBUG_LEVEL 0
#endif

char w[MAX_N][MAX_N];
char neww[MAX_N][MAX_N];

int w_X, w_Y;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int Nthread;
pthread_t tid[MAX_N];
int myid[MAX_N];

  
void init1(int X, int Y)  // set the first row and column 1, the others 0
{ 
  int i, j;
  w_X = X,  w_Y = Y;
  for (i=0; i<w_X;i++)
    for (j=0; j<w_Y; j++) 
      w[j][i] = 0;

  for (i=0; i<w_X; i++) w[0][i] = 1;
  for (i=0; i<w_Y; i++) w[i][0] = 1;
}

void test_init2()
{ 
  int i, j;
  w_X = 4;
  w_Y = 6;

  for (i=0; i<w_X;i++)
    for (j=0; j<w_Y; j++) 
      w[j][i] = 0;
  w[0][3] = 1;
  w[1][3] = 1;
  w[2][1] = 1;
  w[3][0] = w[3][1] = w[3][2] = w[4][1] = w[5][1] = 1;
}

void print_world()
{
  int i, j;

  for (i=0; i<w_Y; i++) {
    for (j=0; j<w_X; j++) {
      printf("%d", (int)w[i][j]);
    }
    printf("\n");
  }
}

int neighborcount(int x, int y)
{
  int count = 0;

  if ((x<0) || (x >=w_X)) {
    printf("neighborcount: (%d %d) out of bound (0..%d, 0..%d).\n", x,y, 
	   w_X, w_Y);
    exit(0);
  }
  if ((y<0) || (y >=w_Y)) {
    printf("neighborcount: (%d %d) out of bound (0..%d, 0..%d).\n", x,y, 
	   w_X, w_Y);
    exit(0);
  }

  if (x==0) // left bound
  {
    if (y == 0) { // the upper left element
      count = w[y][x+1] + w[y+1][x] + w[y+1][x+1];
    } else if (y == w_Y-1) { // the lower left element
      count = w[y][x+1] + w[y-1][x] + w[y-1][x+1];
    } else { // the other left bound elements
      count = w[y-1][x] + w[y+1][x] + w[y-1][x+1] + w[y][x+1] + w[y+1][x+1];
    }
  } 
 
 else if (x == w_X -1) // right bound
  {
    if (y == 0) { // the upper right element
      count = w[y][x-1] + w[y+1][x-1] + w[y+1][x];
    } else if (y == w_Y-1) { // the lower right element
      count = w[y][x-1] + w[y-1][x] + w[y-1][x-1];
    } else {// the other right bound elements 
      count = w[y-1][x] + w[y+1][x] + w[y-1][x-1] + w[y][x-1] + w[y+1][x-1];
    }
  } 
  
  
  else 
  { /* x is in the middle */
    if (y == 0) { // the upper bound elements 
      count = w[y][x-1] + w[y][x+1] + w[y+1][x-1] + w[y+1][x] + w[y+1][x+1];
    } else if (y == w_Y-1) { // the lower bound elements
      count = w[y][x-1] + w[y][x+1] + w[y-1][x-1] + w[y-1][x] + w[y-1][x+1];
    } else {// the middle elements
      count = w[y-1][x-1] + w[y][x-1] + w[y+1][x-1] + w[y-1][x] + w[y+1][x] 
              + w[y-1][x+1] + w[y][x+1] + w[y+1][x+1];
    }
  }

  return count;
}

void *threadwork(void *arg)
{
	int rank;
	int i,j;
	int c;
	
	rank = *(int *)arg;
	int block = w_X / Nthread;
	if(block>0)
	{
		for(i=0;i<w_Y;i++)
		{
			if(rank!=Nthread-1)
			{
				for(j=rank*block;j<block*(rank+1);j++)
				{
					c = neighborcount(j, i);  /* count neighbors */
					if (c <= 1) neww[i][j] = 0;      /* die of loneliness */
					else if (c >=4) neww[i][j] = 0;  /* die of overpopulation */
					else if (c == 3)  neww[i][j] = 1;             /* becomes alive */ 
					else neww[i][j] = w[i][j];   /* c == 2, no change */
				}
			}
			else //the last thread part, which should handle all left columns
			{
				//printf("goes here.");
				for(j=rank*block;j<w_X;j++)
				{
					c = neighborcount(j, i);  /* count neighbors */
					if (c <= 1) neww[i][j] = 0;      /* die of loneliness */
					else if (c >=4) neww[i][j] = 0;  /* die of overpopulation */
					else if (c == 3)  neww[i][j] = 1;             /* becomes alive */ 
					else neww[i][j] = w[i][j];   /* c == 2, no change */					
				}				
			}
		}
	}
	pthread_exit(NULL);
}

void *threadcopy(void *arg)
{
	int rank;
	int i,j;
	int *count = malloc (1*sizeof(int));
	*count = 0;
	rank = *(int *)arg;
	int block = w_X / Nthread;
	if(block>0)
	{
		for(i=0;i<w_Y;i++)
		{
			if(rank!=Nthread-1)
			{
				for(j=rank*block;j<block*(rank+1);j++)
				{
					w[i][j] = neww[i][j];
					if(w[i][j] == 1)
					{
						//pthread_mutex_lock(&mutex);
						(*count)++;
						//pthread_mutex_unlock(&mutex);
					}
				}
			}
			else //the last thread part, which should handle all left columns
			{
				for(j=rank*block;j<w_X;j++)
				{
					w[i][j] = neww[i][j];
					if(w[i][j] == 1)
					{
						//pthread_mutex_lock(&mutex);
						(*count)++;
						//pthread_mutex_unlock(&mutex);						
					}
				}				
			}
		}
	}	
	return (void*)count;
}

int main(int argc, char *argv[])
{
  int x, y;
  int iter = 0;
  int c;
  int init_count;
  int count = 0;
  
  int i;

  if (argc == 1) 
  {
    printf("Usage: ./a.out w_X w_Y\n");
    exit(0);
  } else if (argc == 2) 
  {
	test_init2();
	Nthread = atoi(argv[1]);
  }   
  else /* more than three parameters */
  {
	  init1(atoi(argv[1]), atoi(argv[2]));
	  Nthread = atoi(argv[3]);
  }
  
  if(Nthread>w_X) Nthread = w_X;
    
  c = 0;
  for (x=0; x<w_X; x++) {
    for (y=0; y<w_Y; y++) {
      if (w[y][x] == 1) c++;
    }
  }

  init_count = c;
  count = init_count;

  printf("initial world, population count: %d\n", c);
  if (DEBUG_LEVEL > 10) print_world();

  for (iter = 0; (iter < 200) && (count < 50*init_count) && 
	 (count > init_count / 50); iter ++) 
  {
	  for(i=0;i<Nthread;i++)
	  {
		  myid[i] = i;
		  pthread_create(&tid[i],NULL,&threadwork,&myid[i]);	  	 
	  }
	  
	  for(i=0;i<Nthread;i++)
	  {
		  pthread_join(tid[i],NULL);		   
	  }

    /* copy the world, and count the current lives */
	count = 0;
	for(i=0;i<Nthread;i++)
	{
		myid[i] = i;
		pthread_create(&tid[i],NULL,&threadcopy,&myid[i]);
	}
	for(i=0;i<Nthread;i++)
	{
		int* status = NULL;
		pthread_join(tid[i],(void**)&status);
		count = count + *status;
	} 
	
    printf("iter = %d, population count = %d\n", iter, count);      
    if (DEBUG_LEVEL > 10) print_world();
  }

  {
    FILE *fd;
    if ((fd = fopen("final_world000.txt", "w")) != NULL) 
	{
      for (y=0; y<w_Y; y++) 
	  {
		for (x=0; x<w_X; x++) 
		{
          fprintf(fd, "%d", (int)w[y][x]);
		}
		fprintf(fd, "\n");
      }
    } else {
      printf("Can't open file final_world000.txt\n");
      exit(1);
    }
  }
  return 0;
}
