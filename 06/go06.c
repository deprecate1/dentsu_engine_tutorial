#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

double komi = 6.5;

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

int count_score(int turn_color)
{
  int x,y,i;
  int score = 0, win;
  int black_area = 0, white_area = 0, black_sum, white_sum;
  int mk[4];
  int kind[3];

  kind[0] = kind[1] = kind[2] = 0;
  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
    int z = get_z(x+1,y+1);
    int c = board[z];
    kind[c]++;
    if ( c != 0 ) continue;
    mk[1] = mk[2] = 0;  
    for (i=0;i<4;i++) mk[ board[z+dir4[i]] ]++;
    if ( mk[1] && mk[2]==0 ) black_area++;
    if ( mk[2] && mk[1]==0 ) white_area++;
  }
 
  black_sum = kind[1] + black_area;
  white_sum = kind[2] + white_area;
  score = black_sum - white_sum;

  win = 0;
  if ( score - komi > 0 ) win = 1;

//printf("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
//printf("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
//printf("score=%d, win=%d\n",score, win);
  return win;
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
//  print_board();
//  printf("loop=%d,z=%d,c=%d,empty_num=%d,ko_z=%d\n",
//         loop, get81(z), color, empty_num, get81(ko_z) );
    color = flip_color(color);
  }
  return count_score(turn_color);
}

int primitive_monte_calro(int color)
{
  int    try_num    = 30; // number of playout
  int    best_z     =  0;
  double best_value;
  double win_rate;
  int x,y,err,i,win_sum,win;

  int ko_z_copy;
  int board_copy[BOARD_MAX];  // keep current board
  ko_z_copy = ko_z;
  memcpy(board_copy, board, sizeof(board));

  if ( color == 1 ) {
    best_value = -100;
  } else {
    best_value = +100;
  }

  // try all empty point
  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
    int z = get_z(x+1,y+1);
    if ( board[z] != 0 ) continue;

    err = put_stone(z, color, FILL_EYE_ERR);
    if ( err != 0 ) continue;

    win_sum = 0;
    for (i=0;i<try_num;i++) {
      int board_copy2[BOARD_MAX];
      int ko_z_copy2 = ko_z;
      memcpy(board_copy2, board, sizeof(board));

      win = playout(flip_color(color));
      win_sum += win;

      ko_z = ko_z_copy2;
      memcpy(board, board_copy2, sizeof(board));
    }
    win_rate = (double)win_sum / try_num;
//  print_board();
//  printf("z=%d,win=%5.3f\n",get81(z),win_rate);
    
    // select max value, black
    // select min value, white
	if ( (color == 1 && win_rate > best_value) ||
         (color == 2 && win_rate < best_value)    ) {
      best_value = win_rate;
      best_z = z;
      printf("best_z=%d,color=%d,v=%5.3f,try_num=%d\n",get81(best_z),color,best_value,try_num);
    }

    ko_z = ko_z_copy;
    memcpy(board, board_copy, sizeof(board));  // resume board
  }
  return best_z;
}

int main()
{
  int color = 1;
  int z,err,i;
  
  srand( (unsigned)time( NULL ) );
  for (i=0;i<2;i++) {
    z = primitive_monte_calro(color);
    err = put_stone(z, color, FILL_EYE_OK);
    print_board();
    color = flip_color(color);
  }

  return 0;
}
