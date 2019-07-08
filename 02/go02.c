#include <stdio.h>
#include <stdlib.h>


#define B_SIZE     9
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

int board[BOARD_MAX] = {
  3,3,3,3,3,3,3,3,3,3,3,  //    1 2 3 4 5 6 7 8 9
  3,0,0,0,0,0,2,0,0,0,3,  // 1 „¡„¦„¦„¦„¦›„¦„¦„¢
  3,0,0,0,0,2,1,2,2,2,3,  // 2 „¥„©„©„©›œ›››
  3,0,0,0,0,2,1,1,1,1,3,  // 3 „¥„©„©„©›œœœœ
  3,0,0,0,0,0,2,1,2,2,3,  // 4 „¥„©„©„©„©›œ››
  3,0,0,0,0,0,0,0,0,0,3,  // 5 „¥„©„©„©„©„©„©„©„§
  3,0,1,2,0,0,0,0,0,0,3,  // 6 „¥œ›„©„©„©„©„©„§
  3,1,2,0,2,0,0,0,0,0,3,  // 7 œ›„©›„©„©„©„©„§
  3,0,1,2,0,2,2,1,1,0,3,  // 8 „¥œ›„©››œœ„§
  3,0,0,0,0,2,1,0,2,1,3,  // 9 „¤„¨„¨„¨›œ„¨›œ
  3,3,3,3,3,3,3,3,3,3,3
};

int dir4[4] = { +1,+WIDTH,-1,-WIDTH };

int ko_z;

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

// put stone. success returns 0
int put_stone(int tz, int color)
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
//if ( wall + mycol_safe == 4                            ) return 3; // eye
  if ( board[tz] != 0                                    ) return 4;

  for (i=0;i<4;i++) {
    int lib = around[i][0];
    int c   = around[i][2];
    if ( c == un_col && lib == 1 && board[tz+dir4[i]] ) {
      take_stone(tz+dir4[i], un_col);
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

int main()
{
  int err;
  
  print_board();
  err = put_stone(get_z(7,5), 2);	// take black 6 stones.
  printf("err=%d\n",err);  print_board();
//  err = put_stone(get_z(3,7), 1);	// take white one stone.
//  printf("err=%d\n",err);  print_board();
//  err = put_stone(get_z(2,7), 2);	// retake black, ko, illegal
//  printf("err=%d\n",err);  print_board();
//  err = put_stone(get_z(7,9), 2);	// take black one stone. not ko.
//  printf("err=%d\n",err);  print_board();

  return 0;
}
