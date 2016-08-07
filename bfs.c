/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * Reads all records in a SEGMENT and makes all possible moves from each,
 * saving and collating results into other SEGMENTS. To reduce redundancy,
 * records are FOLDED before saving.
 *
 * Note: "move" is when a pawn moves forward or captures a non-pawn piece
 * "delete" is when a pawn is captured by a non-pawn piece
 * "cap" is when a pawn captures another pawn, so en passant counts
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "common.h"

void record_write(RECORD record, RECORD parent, FILE *fout[9][9][41][41])
{
	char filename[100] = {0};
	record_fold_180(&record);
	record_fold_we(&record);
	if (!fout[record.numWhite][record.numBlack][record.progressWhite][record.progressBlack]) {
		sprintf(filename, "data/%d-%d-%d-%d.records.%d-%d-%d-%d",
				record.numWhite, record.numBlack, record.progressWhite, record.progressBlack,
				parent.numWhite, parent.numBlack, parent.progressWhite, parent.progressBlack);
		if ((fout[record.numWhite][record.numBlack][record.progressWhite][record.progressBlack] = fopen(filename, "wb")))
			printf("Opened output file %s\n", filename);
		else {
			printf("Could not open output file %s\n", filename);
			exit(1);
		}
	}
	fwrite(&record, 1, 16, fout[record.numWhite][record.numBlack][record.progressWhite][record.progressBlack]);
}

int main (int argc, char *argv[])
{
	FILE *fin;
	FILE *fout[9][9][41][41] = {{{{0}}}};
	RECORD record, copyrecord;
	uint64_t square;

	int i;
	int64_t finsize, status=0;
	char s[100]={0};

	/* Possible moves in a record, bitboard, compass directions */
	uint64_t N, NW, NE, S, SW, SE;

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

	/* Read each record for branching */
	while (status < finsize) {
		fread(&record, 1, 16, fin);
		record_unpack_armies(&record);

		/* For this record, simultaneously find all possible moves from each 
		   square on the board. Ignore capture moves and double move off home row,
		   as all pawn positions are reachable without. */
		N = record.white & ~record.whiteBlack << 8;
		NW = record.white & ~record.whiteBlack << 9 & ~FILEA;
		NE = record.white & ~record.whiteBlack << 7 & ~FILEH;
		S = record.whiteBlack & ~record.white & ~record.whiteBlack >> 8;
		SW = record.whiteBlack & ~record.white & ~record.whiteBlack >> 7 & ~FILEA;
		SE = record.whiteBlack & ~record.white & ~record.whiteBlack >> 9 & ~FILEH;
		if (!record_can_take_black(record))
			NW = NE = 0x0;
		if (!record_can_take_white(record))
			SW = SE = 0x0;

		for (square=1, i=0; square & BOARD; square<<=1, i++) {
			if (~record.whiteBlack & square) /* No pawn on this square */
				continue;
			if (square & record.white) {
				/* Record where this pawn is deleted */
				copyrecord = record;
				copyrecord.whiteBlack = record.whiteBlack & ~square;
				copyrecord.white = record.white & ~square;
				copyrecord.numWhite = record.numWhite-1;
				copyrecord.progressWhite = record.progressWhite - (5-i/8);
                if (!(square & RANK7)) {
                    record_take_white(&copyrecord);
                    record_pack_armies(&copyrecord);
                }
				record_write(copyrecord, record, fout);

				if (square & N) { /* White advance */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square >> 8);
					copyrecord.white = (record.white & ~square) | (square >> 8);
					copyrecord.progressWhite = record.progressWhite + 1;
					record_write(copyrecord, record, fout);
				}
				if (square & NW) { /* White attacks */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square >> 9);
					copyrecord.white = (record.white & ~square) | (square >> 9);
					copyrecord.progressWhite = record.progressWhite + 1;
					record_take_black(&copyrecord);
					record_pack_armies(&copyrecord);
					record_write(copyrecord, record, fout);
				}
				if (square & NE) { /* White attacks */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square >> 7);
					copyrecord.white = (record.white & ~square) | (square >> 7);
					copyrecord.progressWhite = record.progressWhite + 1;
					record_take_black(&copyrecord);
					record_pack_armies(&copyrecord);
					record_write(copyrecord, record, fout);
				}
			} else { /* Square is black */
				/* Record where this pawn is deleted */
				copyrecord = record;
				copyrecord.whiteBlack = record.whiteBlack & ~square;
				copyrecord.numBlack = record.numBlack - 1;
				copyrecord.progressBlack = record.progressBlack - (i/8);
                if (!(square & RANK2)) {
                    record_take_black(&copyrecord);
                    record_pack_armies(&copyrecord);
                }
				record_write(copyrecord, record, fout);

				if (square & S) { /* Black advance */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square << 8);
					copyrecord.progressBlack = record.progressBlack + 1;
					record_write(copyrecord, record, fout);
				}
				if (square & SW) { /* Black attacks */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square << 7);
					copyrecord.progressBlack = record.progressBlack + 1;
					record_take_white(&copyrecord);
					record_pack_armies(&copyrecord);
					record_write(copyrecord, record, fout);
				}
				if (square & SE) { /* Black attacks */
					copyrecord = record;
					copyrecord.whiteBlack = (record.whiteBlack & ~square) | (square << 9);
					copyrecord.progressBlack = record.progressBlack + 1;
					record_take_white(&copyrecord);
					record_pack_armies(&copyrecord);
					record_write(copyrecord, record, fout);
				}
			}
		}
		status += 16;
		if (status % 160000 == 0 || status == finsize)
			printf("Processed %lld of %lld\n", status/16, finsize/16);
	}
	return 0;
}
