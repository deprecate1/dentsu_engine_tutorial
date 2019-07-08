#define _CRT_SECURE_NO_WARNINGS     // sprintf is Err in VC++

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

#if defined(_MSC_VER)
#else
#include <sys/time.h>
#endif


double komi = 6.5;

int use_time_control = 1;    // 0 ... no time limit

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
int dir8[8] = { +1,+WIDTH,-1,-WIDTH,  +1+WIDTH, +WIDTH-1, -1-WIDTH, -WIDTH+1 };

int ko_z;

#define MAX_MOVES 1000
int record[MAX_MOVES];
double record_time[MAX_MOVES];
int moves = 0;

int all_playouts = 0;
int flag_test_playout = 0;

#define D_MAX 1000
int path[D_MAX];
int depth;


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

// don't call twice in same sentence. like prt("z0=%s,z1=%s\n",get_char_z(z0),get_char_z(z1));
char *get_char_z(int z)
{
  int x,y,ax;
  static char buf[16];
  sprintf(buf,"pass");
  if ( z==0 ) return buf;
  y = z / WIDTH;
  x = z - y*WIDTH;
  ax = x-1+'A';
  if ( ax >= 'I' ) ax++;  // from 'A' to 'T', excluding 'I'
  sprintf(buf,"%c%d",ax,B_SIZE+1 - y);
  return buf;
}

int flip_color(int col)
{
  return 3 - col;
}

void prt(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
//{ FILE *fp = fopen("out.txt","a"); if ( fp ) { vfprt( fp, fmt, ap ); fclose(fp); } }
  vfprintf( stderr, fmt, ap );
  va_end(ap);
}
void send_gtp(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf( stdout, fmt, ap );
  va_end(ap);
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

  prt("   ");
//for (x=0;x<B_SIZE;x++) prt("%d",x+1);
  for (x=0;x<B_SIZE;x++) prt("%c",'A'+x+(x>7));
  prt("\n");
  for (y=0;y<B_SIZE;y++) {
//  prt("%2d ",y+1);
    prt("%2d ",B_SIZE-y);
    for (x=0;x<B_SIZE;x++) {
      prt("%s",str[board[get_z(x+1,y+1)]]);
    }
    if ( y==4 ) prt("  ko_z=%d,moves=%d",get81(ko_z), moves);
    prt("\n");
  }
}

const char *pattern3x3[] = {
  "XO?"  // "X" ... black,  black "X" to play at the center.
  "..."  // "O" ... white
  "???", // "." ... empty
         // "#" ... out of board
  "OX."  // "?" ... dont care
  "..."
  "?.?",

  "XO?"
  "..X"
  "?.?",

  "X.."
  "O.."
  "###",

  NULL
};

#define EXPAND_PATTERN_MAX  (8*100)
int e_pat_num = 0;
int e_pat[EXPAND_PATTERN_MAX][9];      // rotate and flip pattern
int e_pat_bit[EXPAND_PATTERN_MAX][2];  // [0] ...pattern, [1]...mask
int dir_3x3[9] = { -WIDTH-1, -WIDTH, -WIDTH+1,  -1, 0, +1,  +WIDTH-1, +WIDTH, +WIDTH+1 };

void expand_pattern3x3()
{
  int n,i,j;
  e_pat_num = 0;
  for (n=0; ;n++) {
    int i,j;
    const char *p = pattern3x3[n];
    if ( p == NULL ) break;
    if ( e_pat_num > EXPAND_PATTERN_MAX-8 ) { prt("e_pat_num over Err\n"); exit(0); }

    for (i=0;i<9;i++) {
      int m = 0;
      char c = *(p+i);
      if ( c == '.' ) m = 0;
      if ( c == 'X' ) m = 1;
      if ( c == 'O' ) m = 2;
      if ( c == '#' ) m = 3;
      if ( c == '?' ) m = 4;
      e_pat[e_pat_num][i] = m;
    }
    e_pat_num++;
    for (i=0;i<2;i++) {
      int *p;
      int *q;
      for (j=0;j<3;j++) {
        p = e_pat[e_pat_num-1];
        q = e_pat[e_pat_num  ];
        // roteta 90
        //  "012"      "630"
        //  "345"  ->  "741"
        //  "678"      "852"
        q[0] = p[6];
        q[1] = p[3];
        q[2] = p[0];
        q[3] = p[7];
        q[4] = p[4];
        q[5] = p[1];
        q[6] = p[8];
        q[7] = p[5];
        q[8] = p[2];
        e_pat_num++;
      }
      if ( i==1 ) break;
      p = e_pat[e_pat_num-1];
      q = e_pat[e_pat_num  ];
      // vertical flip
      q[0] = p[6];
      q[1] = p[7];
      q[2] = p[8];
      q[3] = p[3];
      q[4] = p[4];
      q[5] = p[5];
      q[6] = p[0];
      q[7] = p[1];
      q[8] = p[2];
      e_pat_num++;
    }
  }

  for (i=0;i<e_pat_num;i++) {
    e_pat_bit[i][0] = 0;
    e_pat_bit[i][1] = 0;
//  prt("%4d\n",i);
    for (j=0;j<9;j++) {
      int c = e_pat[i][j];
      int mask = 3;
//    prt("%d",c);
//    if ((j+1)%3==0) prt("\n");
      if ( c == 4 ) {
        mask = 0;
        c = 0;
      }
      e_pat_bit[i][0] = e_pat_bit[i][0] << 2;
      e_pat_bit[i][1] = e_pat_bit[i][1] << 2;
      e_pat_bit[i][0] |= c;
      e_pat_bit[i][1] |= mask;
    }
//  prt("bit=%08x,mask=%08x\n",e_pat_bit[i][0],e_pat_bit[i][1]);
  }
  prt("pattern3x3 num=%d, e_pat_num=%d\n",n,e_pat_num);
}

// return pattern number, -1 ... not found 
int match_pattern3x3(int z, int col)
{
#if 1    // 413 playouts/sec
  int pat_bit = 0;
  int i,j;
  for (j=0;j<9;j++) {
    int c = board[z+dir_3x3[j]];
    if ( col==2 && (c==1 || c==2) ) c = 3 - c;
    pat_bit = pat_bit << 2;
    pat_bit |= c;
  }
  for (i=0;i<e_pat_num;i++) {
    int e_bit  = e_pat_bit[i][0];
    int e_mask = e_pat_bit[i][1];
    if ( e_bit == (pat_bit & e_mask) ) return i;
  }
  return -1;
#else    // 353 playouts/sec
  int i,j;
  for (i=0;i<e_pat_num;i++) {
    for (j=0;j<9;j++) {
      int c = board[z+dir_3x3[j]];
      int e = e_pat[i][j];
      if      ( col==2 && e==1 ) e = 2;
      else if ( col==2 && e==2 ) e = 1;
      if ( e==4 ) continue;
      if ( c != e ) break;
    }
    if ( j==9 ) return i;
  }
  return -1;
#endif
}

int get_prob(int z, int prev_z, int col)
{
  const int MAX_PROB = INT_MAX / BOARD_MAX;
  int pr = 100;
  int un_col = flip_color(col);
  int y      = z / WIDTH;
  int x      = z - y*WIDTH;
  int prev_y = prev_z / WIDTH;
  int prev_x = prev_z - prev_y*WIDTH;
  int dx = abs(x - prev_x);
  int dy = abs(y - prev_y);
  int m;
  int i;
  int sc[4];

  // add your code, start -->


  sc[0]=sc[1]=sc[2]=sc[3]=0;
  for (i=0;i<8;i++) {
    sc[ board[z+dir8[i]] ]++;
  }
  if ( sc[un_col] >= 3 && sc[   col] == 0 ) pr /= 2;
  if ( sc[   col] >= 3 && sc[un_col] == 0 ) pr *= 2;

  m = -1;
  if ( prev_z != 0 && ((dx+dy)==1 || (dx*dy)==1)) {
    m = match_pattern3x3(z, col);
  }
  if ( m >= 0 ) {
    int n = m/8;  // pattern number
//  prt("match=%3d,z=%s,col=%d\n",m,get_char_z(z),col);
    pr *= 1000;
    if ( n==0 ) pr *= 2;
  }


  // add your code, end <--

  if ( pr < 1        ) pr = 1;
  if ( pr > MAX_PROB ) pr = MAX_PROB;
  return pr;
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

  if ( turn_color == 2 ) win = -win; 

//prt("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
//prt("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
//prt("score=%d, win=%d\n",score, win);
  return win;
}

int playout(int turn_color)
{
  int color =  turn_color;
  int previous_z = 0;
  int loop;
  int loop_max = B_SIZE*B_SIZE + 200;  // for triple ko

  all_playouts++;
  for (loop=0; loop<loop_max; loop++) {
    // all empty points are candidates.
    int empty[BOARD_MAX][2];  // [0]...z, [1]...probability
    int empty_num = 0;
    int prob_sum = 0;
    int x,y,z,err,pr;
    for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
      int z = get_z(x+1,y+1);
      if ( board[z] != 0 ) continue;
      empty[empty_num][0] = z;
      pr = get_prob(z, previous_z, color);
      empty[empty_num][1] = pr;
      prob_sum += pr;
      empty_num++;
    }
    for (;;) {
      int i = 0;
      if ( empty_num == 0 ) {
        z = 0;
      } else {
        int r = rand() % prob_sum;
        int sum = 0;
        for (i=0; i<empty_num; i++) {
          sum += empty[i][1];    // 0,1,2   [0]=1, [1]=1, [2]=1 
          if ( sum > r ) break;
        }
        if ( i==empty_num ) { prt("Err! prob_sum=%d,sum=%d,r=%d,r=%d\n",prob_sum,sum,r,i); exit(0); }
        z = empty[i][0]; 
      }
      err = put_stone(z, color, FILL_EYE_ERR);
      if ( err == 0 ) break;  // pass is ok.
      prob_sum -= empty[i][1];
      empty[i][0] = empty[empty_num-1][0];  // err, delete
      empty[i][1] = empty[empty_num-1][1];
      empty_num--;
    }

    if ( flag_test_playout ) record[moves++] = z;
    if ( depth < D_MAX ) path[depth++] = z;

    if ( z == 0 && previous_z == 0 ) break;  // continuous pass
    previous_z = z;
//  prt("loop=%d,z=%s,c=%d,empty_num=%d,ko_z=%d\n",loop,get_char_z(z),color,empty_num,ko_z);
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

  best_value = -100;

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

      win = -playout(flip_color(color));
      win_sum += win;

      ko_z = ko_z_copy2;
      memcpy(board, board_copy2, sizeof(board));
    }
    win_rate = (double)win_sum / try_num;
//  print_board();
//  prt("z=%d,win=%5.3f\n",get81(z),win_rate);
    
    if ( win_rate > best_value ) {
      best_value = win_rate;
      best_z = z;
//    prt("best_z=%d,color=%d,v=%5.3f,try_num=%d\n",get81(best_z),color,best_value,try_num);
    }

    ko_z = ko_z_copy;
    memcpy(board, board_copy, sizeof(board));  // resume board
  }
  return best_z;
}



// following are for UCT

typedef struct {
  int z;        // move position
  int games;    // number of games
  double rate;  // winrate
  int next;     // next node
  double bonus; // shape bonus
} CHILD;

#define CHILD_SIZE  (B_SIZE*B_SIZE+1)  // +1 for PASS

typedef struct {
  int child_num;
  CHILD child[CHILD_SIZE];
  int child_games_sum;
} NODE;

#define NODE_MAX 10000
NODE node[NODE_MAX];
int node_num = 0;
const int NODE_EMPTY = -1; // no next node
const int ILLEGAL_Z  = -1; // illegal move


void add_child(NODE *pN, int z, double bonus)
{
  int n = pN->child_num;
  pN->child[n].z     = z;
  pN->child[n].games = 0;
  pN->child[n].rate  = 0;
  pN->child[n].next  = NODE_EMPTY;
  pN->child[n].bonus = bonus;  // from 0 to 10, good move has big bonus.
  pN->child_num++;
}

double get_bonus(int z, int prev_z)
{
  int y = z / WIDTH;
  int x = z - y*WIDTH;
  int prev_y = prev_z / WIDTH;
  int prev_x = prev_z - prev_y*WIDTH;
  int dx = abs(x - prev_x);
  int dy = abs(y - prev_y);
  double b = 1.0;

  if ( x==1 || x==B_SIZE   ) b *= 0.5;
  if ( y==1 || y==B_SIZE   ) b *= 0.5;
  if ( x==2 || x==B_SIZE-1 ) b *= 0.8;
  if ( y==2 || y==B_SIZE-1 ) b *= 0.8;
  
  if ( prev_z != 0 && (dx + dy) == 1 ) b *= 2.0;
  
  return b;
}

// create new node. return node index.
int create_node(int prev_z)
{
  int x,y,z,i,j;
  double bonus;
  NODE *pN;
  
  if ( node_num == NODE_MAX ) { prt("node over Err\n"); exit(0); }
  pN = &node[node_num];
  pN->child_num = 0;
  pN->child_games_sum = 0;

  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) {
    z = get_z(x+1,y+1);
    if ( board[z] != 0 ) continue;
    bonus = get_bonus(z, prev_z);
    add_child(pN, z, bonus);
  }
  add_child(pN, 0, 0);  // add PASS

  // sort children
  for (i=0; i<pN->child_num-1; i++) {
    double max_b = pN->child[i].bonus;
    int    max_i = i;
    CHILD tmp;
    for (j=i+1; j<pN->child_num; j++) {
      CHILD *c = &pN->child[j];
      if ( max_b >= c->bonus ) continue;
      max_b = c->bonus;
      max_i = j;
    }
    if ( max_i == i ) continue;
    tmp              = pN->child[    i];
    pN->child[    i] = pN->child[max_i];
    pN->child[max_i] = tmp;
  }

  node_num++;
  return node_num-1; 
}

int select_best_ucb(int node_n)
{
  NODE *pN = &node[node_n];
  int select = -1;
  double max_ucb = -999;
  double ucb = 0;
  int i;
  int legal_num = 0;
  int pw_num = (int)(1.0 + log(pN->child_games_sum+1.0) / log(1.8));
  if ( pw_num < 1 ) pw_num = 1;

  for (i=0; i<pN->child_num; i++) {
    CHILD *c = &pN->child[i];
    if ( c->z == ILLEGAL_Z ) continue;

    legal_num++;
    if ( legal_num > pw_num ) break;

    if ( c->games==0 ) {
      ucb = 10000 + (rand() & 0x7fff);  // try once
    } else {
      const double C = 1;    // depends on program
      const double B0 = 0.1;
      const double B1 = 100;
      double plus = B0 * log(1.0 + c->bonus) * sqrt( B1 / (B1 + c->games));
      ucb = c->rate + C * sqrt( log((double)pN->child_games_sum) / c->games ) + plus;
    }
    if ( ucb > max_ucb ) {
      max_ucb = ucb;
      select = i;
    }
  }
  if ( select == -1 ) { prt("Err! select\n"); exit(0); }
  return select;
}

int search_uct(int color, int node_n)
{
  NODE *pN = &node[node_n];
  CHILD *c = NULL;  
  int select, z, err, win;
  for (;;) {
    select = select_best_ucb(node_n);
    c = &pN->child[select];
    z = c->z;
    err = put_stone(z, color, FILL_EYE_ERR);
    if ( err == 0 ) break;
    c->z = ILLEGAL_Z;     // select other move
  }

  path[depth++] = c->z;

  // playout in first time. <= 10 can reduce node.
  if ( c->games <= 0 || depth == D_MAX || (c->z == 0 && depth>=2 && path[depth-2]==0) ) {
    win = -playout(flip_color(color));
  } else {
    if ( c->next == NODE_EMPTY ) c->next = create_node(c->z);
    win = -search_uct(flip_color(color), c->next);
  }

  // update winrate
  c->rate = (c->rate * c->games + win) / (c->games + 1);
  c->games++;
  pN->child_games_sum++;
  return win;  
}


// get mill second time. clock() returns process CPU times on Linux, not proper when multi thread.
double get_clock()
{
#if defined(_MSC_VER)
  return clock();
#else
  struct timeval  val;
  struct timezone zone;
  if ( gettimeofday( &val, &zone ) == -1 ) { prt("time err\n"); exit(0); }
  double t = val.tv_sec*1000.0 + (val.tv_usec / 1000.0);
  return t;
#endif
}

// get sec time.
double get_spend_time(double ct)
{
//int div = CLOCKS_PER_SEC;	// 1000 ...VC, 1000000 ...gcc
  int div = 1000;
  return (double)(get_clock()+1 - ct) / div;
}

double start_time;
double time_limit_sec = 3.0;

int is_time_over()
{
  if ( use_time_control == 0 ) return 0;
  if ( get_spend_time(start_time) >= time_limit_sec ) return 1;
  return 0;
}

double count_total_time()
{
  int i;
  double total_time[2];

  total_time[0] = 0;  // black time
  total_time[1] = 0;  // white

  for (i=0; i<moves; i++) {
    total_time[i&1] += record_time[i];
  }
  return total_time[moves & 1];
}


int uct_loop = 1000000;  // number of uct loop

int get_best_uct(int color)
{
  int next, i, best_z, best_i = -1;
  int max = -999;
  NODE *pN;
  int prev_z = 0;

  if ( moves > 0 ) prev_z = record[moves-1];
  node_num = 0;
  next = create_node(prev_z);

  for (i=0; i<uct_loop; i++) {
    int board_copy[BOARD_MAX];
    int ko_z_copy = ko_z;
    memcpy(board_copy, board, sizeof(board));

     depth = 0;
     search_uct(color, next);

    ko_z = ko_z_copy;
    memcpy(board, board_copy, sizeof(board));

    if ( is_time_over() ) break;
  }
  pN = &node[next];
  for (i=0; i<pN->child_num; i++) {
    CHILD *c = &pN->child[i];
    if ( c->games > max ) {
      best_i = i;
      max = c->games;
    }
    prt("%2d:z=%2d,rate=%.4f,games=%3d,bonus=%.4f\n",i, get81(c->z), c->rate, c->games, c->bonus);
  }
  best_z = pN->child[best_i].z;
  prt("best_z=%d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n",
       get81(best_z), pN->child[best_i].rate, max, all_playouts, node_num);
  return best_z;
}

void init_board()
{
  int i,x,y;
  for (i=0;i<BOARD_MAX;i++) board[i] = 3;
  for (y=0;y<B_SIZE;y++) for (x=0;x<B_SIZE;x++) board[get_z(x+1,y+1)] = 0;
  moves = 0;
  ko_z = 0;
}

void add_moves(int z, int color, double sec)
{
  int err = put_stone(z, color, FILL_EYE_OK);
  if ( err != 0 ) { prt("put_stone err=%d\n",err); exit(0); }
  record[moves] = z;
  record_time[moves] = sec;
  moves++;
  print_board();
}

const int SEARCH_PRIMITIVE = 0;
const int SEARCH_UCT       = 1;

int play_computer_move(int color, int search)
{
  double sec;
  int z;

  double total_time = count_total_time();

  double base_time = 60*10;    // 10 minutes
  double left_time = base_time - total_time;
  int div = 12; // 40 ... 13x13, 70 ... 19x19
  time_limit_sec = left_time / div;
  if ( left_time < 60       ) time_limit_sec = 1.0;
  if ( left_time < 20       ) time_limit_sec = 0.2;
  if ( use_time_control != 0 ) {
    prt("time_limit_sec=%.1f, total=%.1f, left=%.1f\n",time_limit_sec, total_time, left_time);
  }
  
  start_time = get_clock();
  all_playouts = 0;

  if ( search == SEARCH_UCT ) {
    z = get_best_uct(color);
  } else {
    z = primitive_monte_calro(color);
  }

  sec = get_spend_time(start_time);
  prt("z=%s,color=%d,moves=%d,playouts=%d, %.1f sec(%.0f po/sec),depth=%d\n",
       get_char_z(z), color, moves, all_playouts, sec, all_playouts/sec, depth );

  add_moves(z, color, sec);
  return z;
}

void undo()
{
  int moves_copy = moves - 1;
  int color = 1;
  int i;
  
  if ( moves == 0 ) return;
  init_board();
 
  for (i=0; i<moves_copy; i++) {
    int z = record[i];
    int err = put_stone(z, color, FILL_EYE_OK);
    if ( err != 0 ) { prt("put_stone err=%d\n",err); exit(0); }
    color = flip_color(color);
  }
  moves = moves_copy;
  print_board();
}

// print SGF game record
void print_sgf()
{
  int i;
  prt("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n",B_SIZE,komi); 
  for (i=0; i<moves; i++) {
    int z = record[i];
    int y = z / WIDTH;
    int x = z - y*WIDTH;
    const char *sStone[2] = { "B", "W" };
    prt(";%s",sStone[i&1]);
    if ( z == 0 ) {
      prt("[]");
    } else {
      prt("[%c%c]",x+'a'-1, y+'a'-1 );
    }
    if ( ((i+1)%10)==0 ) prt("\n");
  }
  prt(")\n");
}

void selfplay()
{
  int color = 1;
  int z,search;
  
  for (;;) {
    if ( color == 1 ) {
      search = SEARCH_PRIMITIVE;
    } else {
      search = SEARCH_UCT;
    }
    z = play_computer_move(color, search);
    if ( z == 0 && moves > 1 && record[moves-2] == 0 ) break;
    if ( moves > 300 ) break;  // too long
    color = flip_color(color);
  }

  print_sgf();
}

void test_playout()
{
  flag_test_playout = 1;
  playout(1);
  print_board();
  print_sgf();
}


#define STR_MAX 256
#define TOKEN_MAX 3

int main()
{
  char str[STR_MAX];
  char sa[TOKEN_MAX][STR_MAX];
  char seps[] = " ";
  char *token;
  int x,y,z,ax,count;

  srand( (unsigned)time( NULL ) );
  init_board();
  expand_pattern3x3();

  if ( 0 ) { selfplay(); return 0; }
  if ( 0 ) { test_playout(); return 0; }

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  for (;;) {
    if ( fgets(str, STR_MAX, stdin)==NULL ) break;
//  prt("gtp<-%s",str);
    count = 0;
    token = strtok( str, seps );
    while ( token != NULL ) {
      strcpy(sa[count], token);
      count++;
      if ( count == TOKEN_MAX ) break;
      token = strtok( NULL, seps );
    }

    if ( strstr(sa[0],"boardsize")     ) {
//    int new_board_size = atoi( sa[1] );
      send_gtp("= \n\n");
    } else if ( strstr(sa[0],"clear_board")   ) {
      init_board();
      send_gtp("= \n\n");
    } else if ( strstr(sa[0],"quit") ) {
      break;
    } else if ( strstr(sa[0],"protocol_version") ) {
      send_gtp("= 2\n\n");
    } else if ( strstr(sa[0],"name")          ) {
      send_gtp("= your_program_name\n\n");
    } else if ( strstr(sa[0],"version")       ) {
      send_gtp("= 0.0.1\n\n");
    } else if ( strstr(sa[0],"list_commands" ) ) {
      send_gtp("= boardsize\nclear_board\nquit\nprotocol_version\nundo\n"
               "name\nversion\nlist_commands\nkomi\ngenmove\nplay\n\n");
    } else if ( strstr(sa[0],"komi") ) {
      komi = atof( sa[1] );
      send_gtp("= \n\n");
    } else if ( strstr(sa[0],"undo") ) {
      undo();
      send_gtp("= \n\n");
    } else if ( strstr(sa[0],"genmove") ) {
      int color = 1;
      if ( tolower(sa[1][0])=='w' ) color = 2;

      z = play_computer_move(color, SEARCH_UCT);
      send_gtp("= %s\n\n",get_char_z(z));
    } else if ( strstr(sa[0],"play") ) {  // "play b c4", "play w d17"
      int color = 1;
      if ( tolower(sa[1][0])=='w' ) color = 2;
      ax = tolower( sa[2][0] );
      x = ax - 'a' + 1;
      if ( ax >= 'i' ) x--;
      y = atoi(&sa[2][1]);
      z = get_z(x, B_SIZE-y+1);
      if ( strstr(sa[2],"pass") || strstr(sa[2],"PASS") ) z = 0;  // pass
      add_moves(z, color, 0);
      send_gtp("= \n\n");
    } else {
      send_gtp("? unknown_command\n\n");
    }
  }
  return 0;
}
