#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

int main (int argc, char *argv[])
{
  int white_pawns, black_pawns, white_progress, black_progress;
  int wp, bp;
  int i=0;

  puts("#");
  puts("# Rules to create a *.records files from component *.records.* files");
  puts("#");

  for (white_pawns=0; white_pawns<=8; white_pawns++) 
    for (black_pawns=0; black_pawns<=8 && black_pawns <= white_pawns; black_pawns++)  
      for (white_progress=0; white_progress<=white_pawns*5; white_progress++) 
        for (black_progress=0; black_progress<=black_pawns*5; black_progress++) {
          if (white_pawns==8 && black_pawns==8 && white_progress==0 && black_progress==0)
            continue; // egg comes before the chicken

          printf("data/%d-%d-%d-%d.records ", white_pawns, black_pawns, white_progress, black_progress);
          if (++i % 4 == 0)
            puts("\\");
        }
  puts(":");
  puts("\ttouch $@-merging");
  puts("\ttouch $@.NULL"); /* empty file so the glob below wont errors if no records are available */
  puts("\tfor x in $@.* ; do \\");
  puts("\tcat $$x /dev/null >> $@-merging; \\");
  puts("\tdone;");
  puts("\tmv $@-merging $@");
  puts("\t rm $@.*");

  puts("");
  puts("#");
  puts("# Set the dependencies for each record. We can start branching on a record");
  puts("# after all the feeding records have been fully branched and tallied.");
  puts("#");

  for (white_pawns=0; white_pawns<=8; white_pawns++) 
    for (black_pawns=0; black_pawns<=8 && black_pawns <= white_pawns; black_pawns++)  
      for (white_progress=0; white_progress<=white_pawns*5; white_progress++) 
        for (black_progress=0; black_progress<=black_pawns*5; black_progress++) {

          if (white_pawns==8 && black_pawns==8 && white_progress==0 && black_progress==0)
            continue; // egg comes before the chicken

          printf("data/%d-%d-%d-%d.records: ", white_pawns, black_pawns, white_progress, black_progress);

          if (white_progress>0)
            printf("data/%d-%d-%d-%d.tally ", white_pawns, black_pawns, white_progress-1, black_progress);

          if (black_progress>0)
            printf("data/%d-%d-%d-%d.tally ", white_pawns, black_pawns, white_progress, black_progress-1);

          for (wp = white_progress; white_pawns+1 <= 8 && wp<=white_progress+5; wp++)
            printf("data/%d-%d-%d-%d.tally ", white_pawns+1, black_pawns, wp, black_progress);

          for (bp = black_progress; black_pawns+1 <= 8 && bp<=black_progress+5; bp++)
            if (white_pawns >= black_pawns+1)
              printf("data/%d-%d-%d-%d.tally ", white_pawns, black_pawns+1, white_progress, bp);
            else
              printf("data/%d-%d-%d-%d.tally ", black_pawns+1, white_pawns, bp, white_progress);

          for (wp = white_progress; white_pawns+1 <= 8 && wp<=white_progress+4 && black_pawns > 0; wp++)
            if (black_progress >= 5 - (wp - white_progress))
              printf("data/%d-%d-%d-%d.tally ", white_pawns+1, black_pawns, wp, black_progress-1);

          for (bp = black_progress; black_pawns+1 <= 8 && bp<=black_progress+4 && white_pawns > 0; bp++)
            if ((white_progress >= 5 - (bp - black_progress)) && white_pawns >= black_pawns+1)
              printf("data/%d-%d-%d-%d.tally ", white_pawns, black_pawns+1, white_progress-1, bp);
            else if ((white_progress >= 5 - (bp - black_progress)))
              printf("data/%d-%d-%d-%d.tally ", black_pawns+1, white_pawns, bp, white_progress-1);

          puts("");
        }

  return 0;
}
