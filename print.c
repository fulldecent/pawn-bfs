/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * print.c
 * Prints all records in a file
 *
 * WARNING: uses fread on a struct, update this when 128-bit alignment is popular
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "common.h"

int main (int argc, char *argv[])
{
	FILE *fin;
	int64_t finsize, status = 0;
	RECORD record;
	char s[100]={0};

	if (argc != 2 || sscanf(argv[1], "data/%d-%d-%d-%d.sorted", &record.numWhite, &record.numBlack, &record.progressWhite, &record.progressBlack) != 4) {
		printf("usage: %s data/WPAWNS-BPAWNS-WPROGRESS-BPROGRESS.sorted\n", argv[0]);
		exit(1);
	}

	if ((fin = fopen(argv[1], "rb"))) {
		fseek(fin, 0L, SEEK_END);
		finsize = ftell(fin);
		rewind(fin);
		printf("Opened input file %s with %lld records\n", argv[1], finsize/16);
		if (finsize%16 != 0) {
			printf("ERROR: Finsize mod 16 = %lld\n", finsize%16);
			exit(1);
		}
	} else {
		printf("Could not open input file %s\n", s);
		exit(1);
	}

	while (status < finsize) {
		fread(&record, 1, 16, fin);
		printf("\nRECORD #%" PRId64 "\n", status/16+1);
		record_print(record);

/*
		// TEST
		record_unpack_armies(&record);
		record_pack_armies(&record);
		record_fold_180(&record);
		record_fold_we(&record);
		record_print(record);
*/

		status += 16;
	}
	return 0;
}
