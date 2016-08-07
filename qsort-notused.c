/*
  BFS of all possible pawn arrangements

  Implements SORT PHASE on a records file:

  Put records in order
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

const uint64_t BOARD  = 0x0000ffffffffffff; /* works on record L or record H */
int64_t status, total, statusinc, lastprint;
int64_t curpivotsize, curpivotprog, curpivottype; /* type: 0=pivot, 1=insertion sort */
int ALARMcount;
int bailout;

void  ALARMhandler(int sig)
{
     signal(SIGALRM, SIG_IGN);
     ALARMcount++;
     printf("Sorted %lld of %lld -- pivot %lld of %lld (type %lld) -- %d seconds\n", status, total, curpivotprog, curpivotsize, curpivottype, ALARMcount);
     alarm(1);                     /* set alarm for next run   */
     signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
}

void  siginthandler(int sig)
{
     signal(SIGINT, SIG_IGN);
     bailout = 1;
}


/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 * code from http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libc/stdlib/qsort.c?rev=1.10;content-type=text/plain
 * edits as per http://www.codeproject.com/Articles/51963/Issues-of-64-bit-code-in-real-programs-qsort.aspx
 *
 * Note that Bentley & McIlroy swap pivot out to a temporary var. OpenBSD and we do not.
 *
 * My edits: block signals during swap, report status
 */

#define min(a, b)	(a) < (b) ? a : b

static __inline void
swapfunc(char *a, char *b, int n)
{
	/* I can guarantee n <= 100 based on the dataset */
	long i = n / sizeof(long);
	long *pi = (long *) a;
	long *pj = (long *) b;
	do {
		long t = *pi;
		*pi++ = *pj;
		*pj++ = t;
	} while (--i > 0);
	if (bailout)
		exit(1);
}

#define swap(a, b)		swapfunc(a, b, es)

#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n)

static __inline char *
med3(char *a, char *b, char *c, int64_t (*cmp)(const void *, const void *))
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}


/* data, num records, each size (bytes), comparitor */
void qsort64(void *aa, size_t n, size_t es, int64_t (*cmp)(const void *, const void *))
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	size_t d, r;
        int swap_cnt;
	char *a = aa;

loop:	swap_cnt = 0;

	curpivottype = 0;
	curpivotsize = n;
	curpivotprog = 0;

	if (n < 7) {
		for (pm = (char *)a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
                status += n;
//                if (status - lastprint > 1000)
//                        printf("Sorted %lld of %lld\n", lastprint=status/1000*1000, total);
		return;
	}
	pm = (char *)a + (n / 2) * es;
	if (n > 7) {
		pl = (char *)a;
		pn = (char *)a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap(a, pm);

	pa = pb = (char *)a + es;

	pc = pd = (char *)a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}

		curpivotprog = (pc - pb) / es;

		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		curpivottype = 1;
		curpivotprog = -1;
		curpivotsize = n;

		status += 1;
		for (pm = (char *) a + es; pm < (char *) a + n * es; pm += es) {
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0; 
			     pl -= es)
				swap(pl, pl - es);
			status += 1;
			curpivotsize -=1;
		}
		return;
	}

	pn = (char *)a + n * es;
	r = min(pa - (char *)a, pb - pa);
	vecswap(a, pb - r, r);
	r = min(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);

	statusinc = n - ((r = pb - pa)>es?r/es:0);
        statusinc -= ((r = pd - pc)>es?r/es:0);
        status += statusinc;
/*
        if (status - lastprint > 1000)
                printf("Sorted %lld of %lld\n", lastprint=status/1000*1000, total);
*/

	if ((r = pb - pa) > es)
		qsort64(a, r / es, es, cmp);
	if ((r = pd - pc) > es) { 
		/* Iterate rather than recurse to save stack space */
/*		qsort(pn - r, r / es, es, cmp);*/
		a = pn - r;
		n = r / es;
		goto loop;
	}
}


static __inline int64_t compare (const void *a, const void *b) 
{
  if ((*(uint64_t*)a & BOARD) == (*(uint64_t*)b & BOARD))
    return ((*((uint64_t*)a+1) & BOARD) - (*((uint64_t*)b+1) & BOARD));
  return ((*((uint64_t*)a) & BOARD) - (*((uint64_t*)b) & BOARD));
/*
  if ((*(uint64_t*)a & BOARD) == (*(uint64_t*)b & BOARD))
    return 1;
  if ((*(uint64_t*)a & BOARD) < (*(uint64_t*)b & BOARD))
    return -1;
  if ((*((uint64_t*)a+1) & BOARD) > (*((uint64_t*)b+1) & BOARD))
    return 1;
  if ((*((uint64_t*)a+1) & BOARD) < (*((uint64_t*)b+1) & BOARD))
    return -1;
  return 0;
*/
}

int main (int argc, char *argv[])
{
  FILE *fin;
  struct stat statbuf;
  uint64_t *records;
  status=lastprint=0;

  if (argc != 2) {
    printf("usage: %s data/A_B.records\n", argv[0]);
    exit(1);
  }

  /* open the input file */
  if ((fin = fopen(argv[1], "a+b")) < 0) {
    printf("can't open %s for read/write", argv[1]);
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
  total = statbuf.st_size / 16;

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
  qsort64(records, statbuf.st_size/16, 16, compare);
  printf("Sorted %lld of %lld\n", total, total);

  return 0;
}
