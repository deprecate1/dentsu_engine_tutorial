#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define B_SIZE     9
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

int board[BOARD_MAX] = {
  3,3,3,3,3,3,3,3,3,3,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,0,0,0,0,0,0,0,0,0,3,
  3,3,3,3,3,3,3,3,3,3,3
};

int dir4[4] = { +1,+WIDTH,-1,-WIDTH };

int ko_z;

#define MAX_MOVES 1000
int record[MAX_MOVES];
int moves = 0;


int get_z(int x,int y)
{
  return y*WIDTH + x;  // 1<= x <=9, 1<= y <=9
}

int get81(int z)            // for display only
{
  int y = z / WIDTH;
  int x = z - y*WIDTH;    // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
  if ( z==0 ) return 0;
  return x*10 + y;        // x*100+y for 19x19
}

int flip_color(int col)
{
  return 3 - col;
}

int check_board[BOARD_MAX];

void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone)
{
  int z,i;

  check_board[tz] = 1;     // search flag
  (*p_stone)++;            // number of stone
  for (i=0;i<4;i++) {
    z = tz + dir4[i];
    if ( check_board[z] ) continue;
    if ( board[z] == 0 ) {
      check_board[z] = 1;
      (*p_liberty)++;      // number of liberty
    }
    if ( board[z] == color ) count_liberty_sub(z, color, p_liberty, p_stone);
  }
}

void count_liberty(int tz, int *p_liberty, int *p_stone)
{
  int i;
  *p_liberty = *p_stone = 0;
  for (i=0;i<BOARD_MAX;i++) check_board[i] = 0;
  count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

void take_stone(int tz,int color)
{
  int z,i;
  
  board[tz] = 0;
  for (i=0;i<4;i++) {
    z = tz + dir4[i];
    if ( board[z] == color ) take_stone(z,color);
  }
}

const int FILL_EYE_ERR = 1;
const int FILL_EYE_OK  = 0;

// put stone. success returns 0. in playout, fill_eye_err = 1.
int put_stone(int tz, int color, int fill_eye_err)
{
  int around[4][3];
  int un_col = flip_color(color);
  int space = 0;
  int wall  = 0;
  int mycol_safe = 0;
  int capture_sum = 0;
  int ko_maybe = 0;
  int liberty, stone;
  int i;

  if ( tz == 0 ) { ko_z = 0; return 0; }  // pass

  // count 4 neighbor's liberty and stones.
  for (i=0;i<4;i++) {
    int z, c, liberty, stone;
    around[i][0] = around[i][1] = around[i][2] = 0;
    z = tz+dir4[i];
    c = board[z];  // color
    if ( c == 0 ) space++;
    if ( c == 3 ) wall++;
    if ( c == 0 || c == 3 ) continue;
    count_liberty(z, &liberty, &stone);
    around[i][0] = liberty;
    around[i][1] = stone;
    around[i][2] = c;
    if ( c == un_col && liberty == 1 ) { capture_sum += stone; ko_maybe = z; }
    if ( c == color  && liberty >= 2 ) mycol_safe++;
  }

  if ( capture_sum == 0 && space == 0 && mycol_safe == 0 ) return 1; // suicide
  if ( tz == ko_z                                        ) return 2; // ko
  if ( wall + mycol_safe == 4 && fill_eye_err            ) return 3; // eye
  if ( board[tz] != 0                                    ) return 4;

  for (i=0;i<4;i++) {
    int lib = around[i][0];
    int c   = around[i][2];
    if ( c == un_col && lib == 1 && board[tz+dir4[i]] ) {
      take_stone(tz+dir4[i],un_col);
    }
  }

  board[tz] = color;

  count_liberty(tz, &liberty, &stone);
  if ( capture_sum == 1 && stone == 1 && liberty == 1 ) ko_z = ko_maybe;
  else ko_z = 0;
  return 0;
}

void print_board()
{
  int x,y;
  const char *str[4] = { ".","X","O","#" };

  printf("   ");
  for (x=0;x<B_SIZE;x++) printf("%d",x+1);
  printf("\n");
  for (y=0;y<B_SIZE;y++) {
    printf("%2d ",y+1);
    for (x=0;x<B_SIZE;x++) {
      printf("%s",str[board[get_z(x+1,y+1)]]);
    }
    if ( y==4 ) printf("  ko_z=%d",get81(ko_z));
    printf("\n");
  }
}

int playout(int turn_color)
{
  int color =  turn_color;
  int previous_z = 0;
  int loop;
  int loop_max = B_SIZE*B_SIZE + 200;  // for triple ko

  for (loop=0; loop<loop_max; loop++) {
    // all empty points are candidates.
    int empty[BOARD_MAX];
    int empty_num = 0;
    int x,y,z,r,err;
    for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
      int z = get_z(x+1,y+1);
      if ( board[z] != 0 ) continue;
      empty[empty_num] = z;
      empty_num++;
    }
    r = 0;
    for (;;) {
      if ( empty_num == 0 ) {
        z = 0;
      } else {
        r = rand() % empty_num;
        z = empty[r];
      }
      err = put_stone(z, color, FILL_EYE_ERR);
      if ( err == 0 ) break;
      empty[r] = empty[empty_num-1];  // err, delete
      empty_num--;
    }
    if ( z == 0 && previous_z == 0 ) break;  // continuous pass
    previous_z = z;
    print_board();
    printf("loop=%d,z=%d,c=%d,empty_num=%d,ko_z=%d\n",
           loop, get81(z), color, empty_num, get81(ko_z) );
    color = flip_color(color);
  }
  return 0;
//return count_score(turn_color);
}

int main()
{
  int color = 1;

  srand( (unsigned)time( NULL ) );
  playout(color);

  return 0;
}
