/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * hsort.c
 * Sorts all records in a file, identical records are not merged
 *
 * WARNING: MAY HAVE PROBLEMS WITH FILES OVER 2GB or 3GB or 4GB
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"

typedef struct {
	uint64_t hi;
	uint64_t lo;
} U128T;
#define COMPARE(a, b) ( (a.hi&BOARD) > (b.hi&BOARD) || ((a.hi&BOARD) == (b.hi&BOARD) && (a.lo&BOARD) > (b.lo&BOARD)) )

int64_t status, total;
int ALARMcount;
int bailout;

void ALARMhandler(int sig)
{
	signal(SIGALRM, SIG_IGN);
	ALARMcount++;
	printf("Status %lld of %lld -- %d seconds\n", status, total, ALARMcount);
	alarm(1);                     /* set alarm for next run   */
	signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
}

void siginthandler(int sig)
{
	signal(SIGINT, SIG_IGN);
	bailout = 1;
}

/*
 * Heapsort routine from http://rosettacode.org/wiki/Sorting_algorithms/Heapsort#C
 */
void siftDown(U128T records[], size_t root, size_t bottom);

void hsort64(U128T records[], size_t count)
{
	int64_t start, end;
	U128T temp;

	status = 0;
	puts("Heapifying records");
	for (start=count/2-1; start>=0; start--) {
		if (bailout)
			exit(1);
		siftDown(records, start, count);
		status = count - 2*start;
	}

	status = 0;
	puts("Sorting records");
	for (end=count-1; end>0; end--) {
		if (bailout)
			exit(1);
		temp = records[end];
		records[end] = records[0];
		records[0] = temp;
		siftDown(records, 0, end);
		status = count - end + 1;
	}
}
 
void siftDown(U128T records[], size_t start, size_t end)
{
	int root = start;
	U128T temp;

	while (root*2+1 < end) {
		int child = 2*root + 1;
		if ((child + 1 < end) && COMPARE(records[child], records[child+1]))
			child += 1;
		if (COMPARE(records[root], records[child])) {
			temp = records[child];
			records[child] = records[root];
			records[root] = temp;
			root = child;
		}
		else
			return;
	}
}

int main (int argc, char *argv[])
{
	FILE *fin;
	struct stat statbuf;
	U128T *records;

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
	hsort64(records, total);
	printf("Sorted %lld of %lld\n", total, total);

	return 0;
}
