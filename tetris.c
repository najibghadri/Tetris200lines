#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <ctype.h>

#define ROWS 20 // you can change height and width of table with ROWS and COLS 
#define COLS 10
#define TRUE 1
#define FALSE 0
#define MAXSHAPES 7
#define MAXNEXTPIECES 6
#define MAXWIDTH 4

char table[ROWS][COLS] = {0};
char *Table[ROWS] = {0};
char Buffer[ROWS][COLS] = {0};
char NextPzBuffer[MAXNEXTPIECES][MAXWIDTH*MAXWIDTH] = {0};
int score = 0;
char GameOn = TRUE;
suseconds_t timer = 400000; // decrease this to make it faster
int decrease = 1000;
int newLines = 0;
char TSpin = 0;

typedef struct {
    char array[MAXWIDTH][MAXWIDTH];
    char wasHold;
    int width, row, col;
} Shape;
Shape current, pieceHold;

Shape nextPiece[MAXNEXTPIECES];
int iNextPiece = 0, jNextPiece = MAXNEXTPIECES - 1;

Shape ShapesArray[MAXSHAPES]= {
	{{{0,'G','G',0},{'G','G',0,0}, {0,0,0,0}, {0,0,0,0}}, 0, 3}, //S shape     
	{{{'R','R',0,0},{0,'R','R',0}, {0,0,0,0}, {0,0,0,0}}, 0, 3}, //Z shape     
	{{{0,'P',0,0},{'P','P','P',0}, {0,0,0,0}, {0,0,0,0}}, 0, 3}, //T shape     
	{{{0,0,'O',0},{'O','O','O',0}, {0,0,0,0}, {0,0,0,0}}, 0, 3}, //L shape     
	{{{'B',0,0,0},{'B','B','B',0}, {0,0,0,0}, {0,0,0,0}}, 0, 3}, //flipped L shape    
	{{{'Y','Y',0,0},{'Y','Y',0,0}, {0,0,0,0}, {0,0,0,0}}, 0, 2}, //square shape
//	{{{0,1,0,0},{0,1,0,0}, {0,1,0,0}, {1,1,1,0}}, 'P', 4}, //long T shape     
	{{{0,0,0,0},{'C','C','C','C'}, {0,0,0,0}, {0,0,0,0}}, 0, 4}  //long bar shape
	// you can add any shape like it's done above. Don't be naughty.
};

void increaseNewLines(int lines){
	newLines += lines;
}
int getNewLines(){
	return newLines;
}
char getGameOn(){
	return GameOn;
}
int getScore(){
	return score;
}
char** getBuffer(){
	return (char**)Buffer;
}
char** getNextPzBuffer(){
	return (char**)NextPzBuffer;
}

char **getHoldPiece(){
	return (char**)pieceHold.array;
}

char CopyMem(char *dst, char *org, unsigned int size){
    int i, j;
	long long *orgL = (long long*)org, *dstL = (long long*)dst;
	int *orgI = (int*)org, *dstI = (int*)dst;
	short *orgS = (short*)org, *dstS = (short*)dst;
	if (dst == NULL)
		return 0;
	for (i = 0; i < size / sizeof(long long); i++)
		dstL[i] = org != NULL ? orgL[i] : 0;
	i = i * 2;
	for (i = i; i < size / sizeof(int); i++)
		dstI[i] = org != NULL ? orgI[i] : 0;
	i = i * 2;
	for (i = i; i < size / sizeof(short); i++)
		dstS[i] = org != NULL ? orgS[i] : 0;
	i = i * 2;
	for (i = i; i < size; i++)
		dst[i] = org != NULL ? org[i] : 0;
	return 1;
}

char CopyMemShape(Shape *dst, Shape *org){
	return CopyMem((char*)dst, (char*)org, sizeof(Shape));
}

Shape CopyShape(Shape shape){
	Shape new_shape = {0};
	CopyMem((char*)&new_shape, (char*)&shape, sizeof(Shape));
    return new_shape;
}

void DeleteShape(Shape shape){
    int i;
    for(i = 0; i < shape.width; i++){
//		free(shape.array[i]);
    }
//    free(shape.array);
}

int CheckPosition(Shape shape){ //Check the position of the copied shape
	int i, j;
	for(i = 0; i < shape.width;i++) {
		for(j = 0; j < shape.width ;j++){
			if((shape.col+j < 0 || shape.col+j >= COLS || shape.row+i >= ROWS)){ //Out of borders
				if(shape.array[i][j]) //but is it just a phantom?
					return FALSE;
				
			}
			else if(Table[shape.row+i][shape.col+j] && shape.array[i][j])
				return FALSE;
		}
	}
	return TRUE;
}

void SetNewRandomShape(){ //updates [current] with new shape
	Shape new_shape = nextPiece[iNextPiece];
    new_shape.col = rand()%(COLS-new_shape.width+1);
    new_shape.row = 0;
//    DeleteShape(current);
	current = new_shape;
	if(!CheckPosition(current)){
		GameOn = FALSE;
	}
	iNextPiece = (iNextPiece + 1) % MAXNEXTPIECES == 0 ? 0 : iNextPiece + 1;
	jNextPiece = (jNextPiece + 1) % MAXNEXTPIECES == 0 ? 0 : jNextPiece + 1;
	CopyMemShape(&nextPiece[jNextPiece],&ShapesArray[rand()%MAXSHAPES]);
}

void RotateShape(Shape *shape, char rotation){ //rotates clockwise
	Shape temp = CopyShape(*shape);
	int i, j, k, width;
	width = shape->width;
	for(i = 0; i < width ; i++)
		for(j = 0, k = width-1; j < width ; j++, k--)
			if (rotation == 'r')
				shape->array[i][j] = temp.array[k][i];
			else
				shape->array[k][i] = temp.array[i][j];
//	DeleteShape(temp);
}

void WriteToTable(){
	int i, j;
	for(i = 0; i < current.width ;i++){
		for(j = 0; j < current.width ; j++){
			if(current.array[i][j])
				Table[current.row+i][current.col+j] = current.array[i][j];
		}
	}
}

void RemoveFullRowsAndUpdateScore(){
	int i, j, sum, count=0;
	for(i=0;i<ROWS;i++){
		sum = 0;
		for(j=0;j< COLS;j++) {
			if (Table[i][j])
				sum += 1;
		}
		if(sum==COLS){
			count++;
			int l, k;
			char *cleanRow = Table[i];
			for (k = i; k >= 1; k--)
				Table[k] = Table[k-1];
			Table[0] = cleanRow;
			CopyMem(cleanRow, NULL, sizeof(char)*COLS);
		}
	}
	score += 100*count;
	if (count > 1){
		newLines -= count; 
	}
}

void AddPendingLines(){
	Shape temp = CopyShape(current);
	temp.row++;
	int i, j;
	while (CheckPosition(temp) && newLines > 0){
		char *incrementLine = Table[0];
		j = rand() % COLS;
		for (i = 0; i < COLS; i++)
			incrementLine[i] = i == j ? 0 : '#';
		for (i = 1; i < ROWS; i++)
			Table[i - 1] = Table[i];
		Table[ROWS - 1] = incrementLine;
		newLines--;
	}
}

void FillOutput(){
	Shape temp = CopyShape(current);
	while (CheckPosition(temp))
		temp.row++;
	temp.row--;
	int i, j;
	CopyMem((char*)Buffer, NULL, sizeof(char)*ROWS*COLS);
	for(i = 0; i < ROWS ;i++){
		if (i < MAXNEXTPIECES){
			char *oOutput = (char*)nextPiece[(iNextPiece + i) % MAXNEXTPIECES].array;
			CopyMem((char*)NextPzBuffer[i],oOutput,sizeof(char)*MAXWIDTH*MAXWIDTH);
		}
		for(j = 0; j < COLS ; j++){
			if((i < MAXWIDTH && j < MAXWIDTH) && current.row >=0 && current.array[i][j])
				Buffer[current.row+i][current.col+j] = current.array[i][j];
			if((i < MAXWIDTH && j < MAXWIDTH) && temp.row >=0 && temp.row - 1 > current.row && temp.array[i][j])
				Buffer[temp.row+i][temp.col+j] = tolower(temp.array[i][j]);
			Buffer[i][j] = (Table[i][j] + Buffer[i][j]) ? (Buffer[i][j]?Buffer[i][j]:Table[i][j]): '.';
		}
	}
}

void PrintTable(){
	int i, j;
	clear();
	for(i=0; i<COLS-9; i++)
		printw(" ");
	printw("Covid Tetris NEXT: ");
	j = iNextPiece;
	for (i = 0; i < MAXNEXTPIECES; i++){
		printw("%c -> ", NextPzBuffer[i][1*MAXWIDTH+1]);
	}
	printw(" HOLD: %c \n", pieceHold.wasHold == 0 ? ' ' : pieceHold.array[1][1]);
	for(i = 0; i < ROWS ;i++){
		for(j = 0; j < COLS ; j++){
			printw("%c ", Buffer[i][j]);
		}
		printw("\n");
	}
	printw("\nScore: %d\n", score);
}

void ManipulateCurrent(int action){
	Shape temp = CopyShape(current);
	switch(action){
		case 'w':
			if (newLines < 0)
				newLines = 0;
			temp.row++;  //move down
			while (CheckPosition(temp))
				temp.row++;
			current.row = temp.row - 1;
			WriteToTable();
			RemoveFullRowsAndUpdateScore();
            SetNewRandomShape();
			AddPendingLines();
			TSpin = 0;
			break;
		case 's':
			if (newLines < 0)
				newLines = 0;
			temp.row++;  //move down
			if(CheckPosition(temp))
				current.row++;
			else {
				if (TSpin > 2){
					WriteToTable();
					RemoveFullRowsAndUpdateScore();
					SetNewRandomShape();
					AddPendingLines();
					TSpin = 0;
				} else TSpin++;
			}
			break;
		case 'd':
			temp.col++;  //move right
			if(CheckPosition(temp))
				current.col++;
			break;
		case 'a':
			temp.col--;  //move left
			if(CheckPosition(temp))
				current.col--;
			break;
		case 'r':
			if (current.wasHold)
				break;
			if (pieceHold.wasHold == 0){
				CopyMemShape(&pieceHold,&current);
				SetNewRandomShape();
				current.wasHold = 1;
				pieceHold.wasHold = 1;
			} else{
				current.wasHold = 1;
				CopyMemShape(&temp,&current);
				CopyMemShape(&current,&pieceHold);
				CopyMemShape(&pieceHold,&temp);
				current.col = rand()%(COLS-current.width+1);
				current.row = 0;
//			    DeleteShape(current);
				if(!CheckPosition(current)){
					GameOn = FALSE;
				}
			}
			break;
		case 'e':
			RotateShape(&temp, 'r'); // rotate clockwise
			if(CheckPosition(temp))
				RotateShape(&current,'r');
			break;
		case 'q':
			RotateShape(&temp, 'l'); // rotate anticlockwise
			if(CheckPosition(temp))
				RotateShape(&current,'l');
			break;
	}
//	DeleteShape(temp);
	FillOutput();
}

struct timeval before_now, now;
int hasToUpdate(){
	return ((suseconds_t)(now.tv_sec*1000000 + now.tv_usec) -((suseconds_t)before_now.tv_sec*1000000 + before_now.tv_usec)) > timer;
}

int main() {
	srand(time(0));
	for (int i = 0; i < ROWS; i++)
		Table[i] = table[i];
	for (int i = 0; i < MAXNEXTPIECES; i++){
		Shape new_shape = CopyShape(ShapesArray[rand()%MAXSHAPES]);
		CopyMemShape(&nextPiece[i], &new_shape);
	}
    score = 0;
    int c;
    initscr();
	gettimeofday(&before_now, NULL);
	timeout(1);
	SetNewRandomShape();
	FillOutput();
    PrintTable();
	int wait = 10;
	while(GameOn){
		if ((c = getch()) != ERR) {
		  ManipulateCurrent(c);
		  PrintTable();
		}
		gettimeofday(&now, NULL);
		if (hasToUpdate()) { //time difference in microsec accuracy
			ManipulateCurrent('s');
			PrintTable();
			gettimeofday(&before_now, NULL);
			if (wait == 0){
				wait = 10;
				newLines += 1;
			} else wait--;
		}
	}
	DeleteShape(current);
	endwin();
	int i, j;
	for(i = 0; i < ROWS ;i++){
		for(j = 0; j < COLS ; j++){
			printf("%c ", Table[i][j] ? '#': '.');
		}
		printf("\n");
	}
	printf("\nGame ouvre!\n");
	printf("\nScore: %d\n", score);
    return 0;
}
