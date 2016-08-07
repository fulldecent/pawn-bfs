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
#include "common.h"

///////////// NEED TO ACTUALLY DO MERGE

typedef struct {
	uint64_t hi;
	uint64_t lo;
} U128T;
#define SIMILAR(a, b) ( ((a.hi&BOARD)==(b.hi&BOARD)) && ((a.lo&BOARD)==(b.lo&BOARD)) )

int64_t status_in, status_out, total;
int ALARMcount;
int bailout;

void ALARMhandler(int sig)
{
	signal(SIGALRM, SIG_IGN);
	ALARMcount++;
	printf("Status %lld of %lld in, %lld out -- %d seconds\n", status_in, total, status_out, ALARMcount);
	alarm(1);                     /* set alarm for next run   */
	signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
}

void siginthandler(int sig)
{
	signal(SIGINT, SIG_IGN);
	bailout = 1;
}

int main (int argc, char *argv[])
{
	FILE *fin;
	struct stat statbuf;
	U128T *records;
	RECORD a, b;

	if (argc != 2) {
		printf("usage: %s data/WPAWNS-BPAWNS-WPROGRESS-BPROGRESS.records\n", argv[0]);
		exit(1);
	}

	/* open the input file */
	if ((fin = fopen(argv[1], "a+b")) < 0) {
		printf("ERROR: can't open %s for read/write", argv[1]);
		exit(1);
	}

	/* find size of input file */
	if (fstat(fileno(fin),&statbuf) < 0) {
		puts("ERROR: fstat error");
		exit(1);
	}

	if (statbuf.st_size%16 != 0) {
		printf("ERROR: extra %lld unexpected bytes\n", statbuf.st_size%16);
		exit(1);
	}
	total = statbuf.st_size / 16;

	/* mmap the input file */
	if ((records = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fin), 0)) == MAP_FAILED) {
		puts("mmap error");
		exit(1);
	}

	ALARMcount = 0;
	signal(SIGALRM,ALARMhandler);
	signal(SIGINT,siginthandler);
	alarm(1);
	bailout = 0;

	/* SETUP COMPLETE, GET TO WORK */
	status_out = 0;
	for (status_in = 0; status_in < total; status_in++) {
		if (bailout)
			exit(1);
		if (SIMILAR(records[status_in], records[status_out])) {
			/*
			a.whiteBlack = records[status_in].hi;
			a.white = records[status_in].lo;
			b.whiteBlack = records[status_out].hi;
			b.white = records[status_out].lo;
			record_unpack_armies(&a);
			record_unpack_armies(&b);
			RECORD c = record_union(a, b);
			record_pack_armies(&c);
			records[status_out].hi = c.whiteBlack;
			records[status_out].lo = c.white;
			*/
			records[status_out].hi = records[status_in].hi;
			records[status_out].lo = records[status_in].lo;

		} else {
			status_out++;
			records[status_out] = records[status_in];
		}
	}
	status_out++;
	ftruncate(fileno(fin), 16 * status_out);
	printf("Merged %lld of %lld in, %lld out -- %d seconds\n", status_in, total, status_out, ALARMcount);
	return 0;
}
