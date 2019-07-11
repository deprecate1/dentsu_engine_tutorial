Monte-Carlo Go samples.

Hiroshi Yamashita


2015/11/14

2016/11/07 go10.c - go15.c, fixed "play b p3" makes "pass" in 19x19.


01/go01.c * print Go board

02/go02.c * teaching Go rule

03/go03.c * ramdom player

04/go04.c   do playout by random player

05/go05.c * calculate win or loss in final position

06/go06.c   primitive monte-carlo Go program

07/go07.c   primitive monte-carlo Go program in NegaMax style

08/go08.c   search by UCT

09/go09.c   selfplay UCT and primitive MC

10/go10.c   GTP. 3x3 pattern, Progressive Widening

11/go11.c   RAVE

12/go12.c   Zoblist Hashing

13/go13.c   print playout owner

14/go14.c   Criticality

15/go15.c   Time limit play within 10 minutes


NOTE : * source includes err intentionally. Correct source is in "fixed" directly.


RAVE (go11.c) is strongest.

This samples are for UEC Computer Go Workshop at August 2015.


如果时间允许，这15课会写个教程（也许没必要，因为代码很容易理解）：主要讲了mcts,uct(ucb1)算法，简单模式识别，

RAVE优化，zobrist hash，对杀剧烈程度表示，棋子强弱表示（可以参考实现形势判断）。




不足之处是没有[alpha-beta](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)剪枝优化搜索树（论文中有描述），有兴趣的同学可以研究[pachi](https://github.com/pasky/pachi)源码


原版mcts/uct paper: http://www.cs.utexas.edu/~pstone/Courses/394Rspring13/resources/mcrave.pdf


QQ群：732665928，非技术勿聊



