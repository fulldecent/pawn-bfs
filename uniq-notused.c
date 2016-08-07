/*
  BFS of all possible pawn arrangements

  Removes duplicate record. See readme for what is a duplicate.

  File must be sorted
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> /* GNU SPECIFIC */

const uint64_t DEAD   = 0xffffffffffffffff; 

const uint64_t BOARD  = 0x0000ffffffffffff; /* works on record L or record H */
const uint64_t FILEA  = 0x0000010101010101; 
const uint64_t FILEH  = 0x0000808080808080; 
const uint64_t ROW7   = 0x00000000000000ff; 
const uint64_t ROW6   = 0x000000000000ff00; 
const uint64_t ROW3   = 0x000000ff00000000; 
const uint64_t ROW2   = 0x0000ff0000000000; 

const uint64_t WPROMS = 0x000f000000000000; /* bit masks for first 64 bits "record L" */
const uint64_t BPROMS = 0x00f0000000000000;
const uint64_t WCAPS  = 0x0f00000000000000;
const uint64_t BCAPS  = 0xf000000000000000;

const uint64_t WCOUNT = 0x000f000000000000; /* bit masks for second 64 bits "record H" */
const uint64_t BCOUNT = 0x00f0000000000000;
const uint64_t ENPS   = 0x0f00000000000000;
const uint64_t RESERV = 0xf000000000000000;


size_t pcurrecord, poutput, total;
sigset_t signals;
int ALARMcount;
int bailout;

void  ALARMhandler(int sig)
{
     signal(SIGALRM, SIG_IGN);
     ALARMcount++;
     printf("Cleaned %zu of %zu, shrunk to %zu -- %d seconds\n", pcurrecord/2, total/2, (total+pcurrecord-poutput)/2, ALARMcount);
     alarm(1);                     /* set alarm for next run   */
     signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
}

void  siginthandler(int sig)
{
     signal(SIGINT, SIG_IGN);
     bailout = 1;
}



void print(uint64_t record[2])
{
  uint64_t square;

  for (square=1; square & BOARD; square<<=1) {
    if (!(record[0] & square))
      putchar('.');
    else if (record[1] & square)
      putchar('W');
    else
      putchar('B');
    if (square & FILEH & ~ROW2)
      puts("");
  }
  printf(" w/b proms %02lld %02lld caps %02lld %02lld count %02lld %02lld ep %02lld\n\n",
         (record[0] & WPROMS) / (WPROMS/0x0f), (record[0] & BPROMS) / (BPROMS/0x0f),
         (record[0] & WCAPS ) / (WCAPS /0x0f), (record[0] & BCAPS)  / (BCAPS /0x0f),
         (record[1] & WCOUNT) / (WCOUNT/0x0f), (record[1] & BCOUNT) / (BCOUNT/0x0f),
         (record[1] & ENPS  ) / (ENPS  /0x0f));
}


int main (int argc, char *argv[])
{
  FILE *fin;
  struct stat statbuf;
  uint64_t *records;
  uint64_t curset[2];
  int64_t pcurset, i, j;

  sigaddset(&signals, SIGINT);  

  /* Phase 1 - Open sorted records file and delete any duplicate position
     for example, if the same diagram is reachable with 0 and 2 white captures
     of non-pawn pieces, the former is better. See README for details
  */

  pcurrecord = pcurset = poutput = 0;

  if (argc != 2) {
    printf("usage: %s data/A_B.records\n", argv[0]);
    exit(1);
  }

  /* open the input file */
  if ((fin = fopen(argv[1], "a+b")) < 0) {
    printf("can't open %s for reading", argv[1]);
    exit(1);
  }

  /* find size of input file */
  if (fstat(fileno(fin),&statbuf) < 0) {
    puts("fstat error");
    exit(1);
  }

  if (statbuf.st_size%16 != 0) {
    printf("ERROR: extra %lld unexpected bytes\n", statbuf.st_size%16);
    exit(1);
  }
  total = statbuf.st_size / 8;

  if (total == 0) {
    printf("Cleaned 0 of 0, shrunk to 0\n");
    return 0;
  }

  /* mmap the input file */
  if ((records = mmap (0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fin), 0))
    == MAP_FAILED) {
    puts("mmap error");
    exit(1);
  }

  ALARMcount = 0;
  signal(SIGALRM,ALARMhandler);    
  signal(SIGINT,siginthandler);    
  alarm(1);  
  bailout = 0;

  pcurrecord = poutput = 0;

  while (pcurrecord<total && (records[pcurrecord] == DEAD))
    pcurrecord += 2;

  while (pcurrecord<total) {
    curset[0] = records[pcurrecord] & BOARD;
    curset[1] = records[pcurrecord+1] & BOARD;
    pcurset = poutput;
    if (bailout)
      exit(1);
    if (pcurrecord != poutput) {
      records[poutput] = records[pcurrecord];
      records[poutput+1] = records[pcurrecord+1];
    }
    poutput += 2;

    do 
      pcurrecord += 2;
    while (pcurrecord<total && (records[pcurrecord] == DEAD));

    while (pcurrecord<total && ((records[pcurrecord]&BOARD) == curset[0]) && ((records[pcurrecord+1]&BOARD) == curset[1])) {
      int novel = 1;
      for (i=pcurset; i<poutput && novel; i+=2) {
        if ((records[i] & WPROMS) >= (records[pcurrecord] & WPROMS) &&
            (records[i] & BPROMS) >= (records[pcurrecord] & BPROMS) &&
            (records[i] & WCAPS) <= (records[pcurrecord] & WCAPS) &&
            (records[i] & BCAPS) <= (records[pcurrecord] & BCAPS)) {
/*
          puts("RECORD A:");
          print(&records[i]);
          puts("DOMINATES RECORD B:");
          print(&records[pcurrecord]);
*/
          if (bailout)
            exit(1);

          records[pcurrecord] = DEAD;
          records[pcurrecord+1] = DEAD;
          novel = 0;
        } else if ((records[i] & WPROMS) <= (records[pcurrecord] & WPROMS) &&
                   (records[i] & BPROMS) <= (records[pcurrecord] & BPROMS) &&
                   (records[i] & WCAPS) >= (records[pcurrecord] & WCAPS) &&
                   (records[i] & BCAPS) >= (records[pcurrecord] & BCAPS)) {
/*
          puts("RECORD A:");
          print(&records[i]);
          puts("DOMINATED BY RECORD B:");
          print(&records[pcurrecord]);
*/
          if (bailout)
            exit(1);

          /* Splice this record from the output array */
          for (j=records[i]; j<poutput-2; j+=2) {
            records[j] = records[j+2];
            records[j+1] = records[j+3];
          }
          poutput -= 2;
          i-=2;
        }
      }
      if (novel) {
        if (pcurrecord != poutput) {
          if (bailout)
            exit(1);
          records[poutput] = records[pcurrecord];
          records[poutput+1] = records[pcurrecord+1];
        }
        poutput += 2;
      }

      do 
        pcurrecord += 2;
      while (pcurrecord<total && (records[pcurrecord] == DEAD));
    }
    if (pcurrecord == total)
      break;
  }

  ftruncate(fileno(fin), sizeof(uint64_t) * poutput);
  printf("Cleaned %zu of %zu, shrunk to %zu\n", pcurrecord/2, total/2, poutput/2);
  total = poutput;

  return 0;
}
