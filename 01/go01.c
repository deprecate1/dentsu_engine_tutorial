#include <stdio.h>
#include <stdlib.h>


#define B_SIZE     9
#define WIDTH      (B_SIZE + 2)
#define BOARD_MAX  (WIDTH * WIDTH)

int board[BOARD_MAX] = {
  3,3,3,3,3,3,3,3,3,3,3,
  3,2,1,1,0,1,0,0,0,0,3,
  3,2,2,1,1,0,1,2,0,0,3,
  3,2,0,2,1,2,2,1,1,0,3,
  3,0,2,2,2,1,1,1,0,0,3,
  3,0,0,0,2,1,2,1,0,0,3,
  3,0,0,2,0,2,2,1,0,0,3,
  3,0,0,0,0,2,1,1,0,0,3,
  3,0,0,0,0,2,2,1,0,0,3,
  3,0,0,0,0,0,2,1,0,0,3,
  3,3,3,3,3,3,3,3,3,3,3
};

void print_board()
{
  int x,y;
  const char *str[4] = { ".","X","O","#" };

  printf("   ");
  for (x=0; x<B_SIZE; x++) printf("%d",x+1);
  printf("\n");
  for (y=0; y<B_SIZE; y++) {
    printf("%2d ",y+1);
    for (x=0; x<=B_SIZE; x++) {
      int c = board[ (y+1)*WIDTH + (x+1) ];
      printf("%s",str[c]);
    }
    printf("\n");
  }
}

int main()
{
  print_board();
  return 0;
}
