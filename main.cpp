#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>


#define ROW 10
#define COLUMN 50 
#define LENGTH 17

int gamestatus=1; // 0: quit 1: playing 2: win 3: jump to the river 4: collision with boundary
pthread_mutex_t mutex;
pthread_barrier_t barrier;
struct Node{
	int x , y; 
	Node( int _x , int _y ) : x( _x ) , y( _y ) {}; 
	Node(){} ; 
} frog ; 

char map[ROW+1][COLUMN]; 

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0. 
int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}


void printMap(){
	printf("\033[H\033[2J");// move the cursor to the left top corner of terminal
	for (int i = 0; i <= ROW; i++){
		for (int j = 0; j < COLUMN; j++)
			printf("%c", map[i][j]);
		printf("\n");
	}
}


void *logs_move( void *t ){
	int id = (long)t;
	int row = id+1;

	/*  In the beginning put the log on the left end  */

	for (int i = 0; i < LENGTH; i++)
		map[row][i] = '=';
	
	for (int k= LENGTH; k < COLUMN; k++)
		map[row][k] = ' ';
	
	pthread_barrier_wait(&barrier);	// wait for all thread prepared
	
	int temp[COLUMN];
	int v=(row % 2 == 1)?1:(COLUMN-1);	// +1 %COLUMNS means moves rightwards, +(COLUMN-1)%COLUMN means moves leftwards	
	int j;	
	while(gamestatus==1){
		
		
		pthread_mutex_lock(&mutex);

	 	/*  Move this log  */
		for (j = 0; j < COLUMN; j++){
			temp[(j+v)%COLUMN] = map[row][j];
		}
		for (j = 0; j < COLUMN; j++){
			map[row][j] = temp[j];	
		}
		if(frog.y==row)// if the frog was on this log, move the frog too
			frog.x = (frog.x+v)%COLUMN;

		if (frog.y == row){	// frog on the log
			if (map[row][0] == '='||map[row][COLUMN-1] == '=') 	// log hit the boundary	
				gamestatus = 4;	
		}		
		// print current map to screen
		printMap();
		/*  Check keyboard hits, to change frog's position or quit the game. */
		if( kbhit() ){
			char dir = getchar();
			
			/*  Quit the game  */
			if (dir == 'q' || dir == 'Q') {
				gamestatus = 0;	
			}
			/*  Go up  */
			if (dir == 'w' || dir == 'W') {
				if (map[frog.y-1][frog.x] == '|')	// jump to bank
				{
					gamestatus = 2; 
					map[frog.y-1][frog.x] = '0';
					map[frog.y][frog.x] = '=';
					printMap();
				}
				else if (map[frog.y-1][frog.x] == '='){	// jump to another log
					map[frog.y][frog.x] = (frog.y==ROW)?'|':'=';
					frog.y--;
					map[frog.y][frog.x] = '0';
				}
				else 
					gamestatus = 3; //jump into the river
			}
			/*  Go down  */
			if (dir == 's' || dir == 'S') {
				if (map[frog.y+1][frog.x] == '='|| map[frog.y+1][frog.x] == '|'){
					map[frog.y][frog.x] = '=';
					frog.y++;
					map[frog.y][frog.x] = '0';
				}
				else 
					gamestatus = 3; //jump into the river
			}
			/*  Go left  */
			if (dir == 'a' || dir == 'A') {
				if (map[frog.y][frog.x-1] == '=' || map[frog.y][frog.x-1] == '|'){
					map[frog.y][frog.x] = (frog.y==ROW)?'|':'=';
					frog.x--;
					map[frog.y][frog.x] = '0';
				}
				else 
					gamestatus = 3; //jump into the river
			}
			/*  Go right  */
			if (dir == 'd' || dir == 'D') {
				if (map[frog.y][frog.x+1] == '=' || map[frog.y][frog.x+1] == '|'){
					map[frog.y][frog.x] = (frog.y==ROW)?'|':'=';
					frog.x++;
					map[frog.y][frog.x] = '0';
				}
				else 
					gamestatus = 3;//jump into the river
			}
			
		}
		
		pthread_mutex_unlock(&mutex);
		usleep(200000);
		if (row == 2||row == 7) 
			usleep(400000);
		else if (row == 4|| row == 5) 
			usleep(300000);
	}
	pthread_exit(NULL);
}

int main( int argc, char *argv[] ){

	printf("\033[H\033[2J");// clear the screen

	// Initialize the river map and frog's starting position
	int i , j ; 
	for( i = 1; i < ROW; ++i ){	
		for( j = 0; j < COLUMN - 1; ++j )	
			map[i][j] = ' ' ;  
	}	

	for( j = 0; j < COLUMN - 1; ++j )	
		map[ROW][j] = map[0][j] = '|' ;

	for( j = 0; j < COLUMN - 1; ++j )	
		map[0][j] = map[0][j] = '|' ;

	frog = Node(  (COLUMN-1) / 2 , ROW) ; 
	map[frog.y][frog.x] = '0' ; 
	
	//Print the map into screen
	printMap();


	/*  Create pthreads for wood move and frog control.  */
	pthread_mutex_init(&mutex, NULL);
	pthread_attr_t attr;
	pthread_t threads[ROW-1];
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_barrier_init(&barrier,NULL,ROW-1);
    int rc, rc1;

    for(long i =0; i<ROW-1; i++){
        rc = pthread_create(&threads[i], &attr, logs_move, (void*)i);
        if(rc){
            printf("ERROR: return code from pthread_create() is %d", rc);
            exit(1);
        }
    }
	for(long i =0; i<ROW-1; i++){
		// wait for all threads to terminate
        rc1 = pthread_join(threads[i], NULL);
        if(rc1){
            printf("ERROR: return code from pthread_join() is %d", rc1);
            exit(1);
        }
    }
	/*  Display the output for user: win, lose or quit.  */ 
	switch(gamestatus){
				case 0:
				printf("\n--YOU QUIT THE GAME--\n");
				break;
				case 2: 
				printf("\n--YOU WIN THE GAME--\n");
				break;
				case 3:
				printf("\n--YOU LOSE THE GAME--\ni.e.you jump into the river.\n");
				break;
				case 4:
				printf("\n--YOU LOSE THE GAME--\ni.e.you collision with the boundary.\n");
				break;
			} 
	// release the resource
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex);
	pthread_barrier_destroy(&barrier);
    
    pthread_exit(NULL);
	
	
	return 0;

}
