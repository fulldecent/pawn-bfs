/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * Utility functions
 */

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "common.h"

/* Each record is 128 bits, spread over two 64-bit integers, "L" and "H" */
const uint64_t BOARD   = 0x0000ffffffffffff; /* works on record L or record H */
const uint64_t FILEA   = 0x0000010101010101; 
const uint64_t FILEH   = 0x0000808080808080; 
const uint64_t RANK7   = 0x00000000000000ff; 
const uint64_t RANK6   = 0x000000000000ff00; 
const uint64_t RANK3   = 0x000000ff00000000; 
const uint64_t RANK2   = 0x0000ff0000000000; 
const uint64_t CAPDATA = 0xffff000000000000; /* captures are saved here in L and H */

void record_print(RECORD record)
{
	uint64_t square;

	printf("Pawn bitboard\n  ");
	for (square=1; square & BOARD; square<<=1) {
		if (square & ~record.whiteBlack) 
			putchar('.');
		else if (square & record.white)
			putchar('W');
		else
		putchar('B');
		if (square & FILEH & ~RANK2)
			printf("\n  ");
	}

	puts("\nPossible armies");
	record_unpack_armies(&record);
	for (int b=15; b>=0; b--) {
		printf("  B %2d", b);
		for (int w=0; w<16; w++)
			printf(" %c ", record_army_is_possible(record, w, b) ? '*' : '.');
		printf("\n");
	}
	printf("  W-->");
	for (int w=0; w<16; w++)
		printf("%2d ", w);

	printf("\nWith symmetry, this record represents %d arrangements\n", record_fold_180(&record) * record_fold_we(&record));
}

int record_fold_180(RECORD *record)
{
	uint64_t square, compare;
	RECORD newRecord = {0};

	if (record->numWhite > record->numBlack)
		return 2; /* already in normal form */
	else if (record->numWhite < record->numBlack)
		goto doflip180; /* in anti-normal form, need to correct */
	if (record->progressWhite > record->progressBlack)
		return 2; /* already in normal form */
	else if (record->progressWhite < record->progressBlack)
		goto doflip180; /* in anti-normal form, need to correct */
	for (square=1ULL<<0, compare=1ULL<<47; square<compare; square<<=1, compare>>=1) {
		if ((record->whiteBlack & square) && !(record->whiteBlack & compare))
			return 2;
		if (!(record->whiteBlack & square) && (record->whiteBlack & compare))
			goto doflip180;
		if ((record->white & square) && !(record->white & compare))
			return 2;
		if (!(record->white & square) && (record->white & compare))
			goto doflip180;
	}
	return 1; /* perfectly symmetric */
doflip180:
	/* Record is anti-normal, need to flip it */
	for (square=1ULL<<0, compare=1ULL<<47; square<compare; square<<=1, compare>>=1) {
		if (record->whiteBlack & square)
			newRecord.whiteBlack |= compare;
		if (record->whiteBlack & compare)
			newRecord.whiteBlack |= square;
		if (record->whiteBlack & ~record->white & square)
			newRecord.white |= compare;
		if (record->whiteBlack & ~record->white & compare)
			newRecord.white |= square;
	}
	newRecord.numWhite = record->numBlack;
	newRecord.numBlack = record->numWhite;
	newRecord.progressWhite = record->progressBlack;
	newRecord.progressBlack = record->progressWhite;
	/* again, this representation is based on driving to grandmom's house in city grid */
	for (square=1ULL<<48, compare=1ULL<<63; square<compare; square<<=1, compare>>=1) {
		if (~record->whiteBlack & square)
			newRecord.white |= compare;
		if (~record->whiteBlack & compare)
			newRecord.white |= square;
		if (~record->white & square)
			newRecord.whiteBlack |= compare;
		if (~record->white & compare)
			newRecord.whiteBlack |= square;
	}
	*record = newRecord; /* invalidates _numBlackPossibleForGivenWhite */
	return 2;
}

int record_fold_we(RECORD *record)
{
	uint64_t square, compare;
	RECORD newRecord = *record;
	int rank, file;

	for (rank=0; rank<48; rank+=8) {
		for (file=0; file<4; file++) {
			square = 1ULL<<(rank+file);
			compare = 1ULL<<(rank+7-file);
			if ((record->whiteBlack & square) && !(record->whiteBlack & compare))
				return 2; /* already in normal form */
			if (!(record->whiteBlack & square) && (record->whiteBlack & compare))
				goto doflipwe;
			if ((record->white & square) && !(record->white & compare))
				return 2;
			if (!(record->white & square) && (record->white & compare))
				goto doflipwe;
		}
	}
	return 1; /* perfectly symmetric */
doflipwe:
	/* Record is anti-normal, need to flip it */
	record->whiteBlack &= ~BOARD;
	record->white &= ~BOARD;
	for (rank=0; rank<48; rank+=8) {
		for (file=0; file<4; file++) {
			square = 1ULL<<(rank+file);
			compare = 1ULL<<(rank+7-file);
			if (newRecord.whiteBlack & square)
				record->whiteBlack |= compare;
			if (newRecord.whiteBlack & compare)
				record->whiteBlack |= square;
			if (newRecord.white & square)
				record->white |= compare;
			if (newRecord.white & compare)
				record->white |= square;
		}
	}
	return 2;
}

int record_can_take_white(RECORD record)
{
	return record._numBlackPossibleForGivenWhite[15-record.numWhite-1] >= 15-record.numBlack;
}

int record_can_take_black(RECORD record)
{
	return record._numBlackPossibleForGivenWhite[15-record.numWhite] >= 15-record.numBlack-1;
}

int record_army_is_possible(RECORD record, int whiteArmy, int blackArmy)
{
	return record._numBlackPossibleForGivenWhite[whiteArmy] >= blackArmy;
}

/* Reduces one white/black piece from each possible inside ORIGINAL */
void record_take_white(RECORD *record)
{
	for (int i=0; i<15; i++)
		record->_numBlackPossibleForGivenWhite[i] = record->_numBlackPossibleForGivenWhite[i]+1;
	record->_numBlackPossibleForGivenWhite[15] = -1;
}

void record_take_black(RECORD *record)
{
	for (int i=0; i<16; i++)
		record->_numBlackPossibleForGivenWhite[i]--;
}

/* Set union */
RECORD record_union(RECORD a, RECORD b)
{
	RECORD retval;
	for (int i=0; i<16; i++)
		retval._numBlackPossibleForGivenWhite[i] = 
		  a._numBlackPossibleForGivenWhite[i] > b._numBlackPossibleForGivenWhite[i] ?
		  a._numBlackPossibleForGivenWhite[i] :
		  b._numBlackPossibleForGivenWhite[i];
	return retval;
}

void record_unpack_armies(RECORD *record)
{
	int w = 0, b = 15;
	uint32_t bits32 = ((record->whiteBlack & CAPDATA) >> 48) | ((record->white & CAPDATA) >> 32);
	assert(__builtin_popcount(bits32) == 16);
	for (uint32_t pointer = 1; pointer; pointer<<=1) {
		if (bits32 & pointer)
			record->_numBlackPossibleForGivenWhite[w++] = b;
		else
			b--;
	}
}

void record_pack_armies(RECORD *record)
{
	uint32_t bits32 = 0, pointer = 0x1;
	int w = 0, b = 15;
	while (pointer) {
		if (b<0 || (w<=15 && record_army_is_possible(*record, w, b))) {
			w++;
			bits32 |= pointer;
		} else {
			b--;
		}
		pointer <<= 1;
	}
	record->whiteBlack = (record->whiteBlack & BOARD) | (((uint64_t)bits32 << 48) & CAPDATA);
	record->white = (record->white & BOARD) | (((uint64_t)bits32 << 32) & CAPDATA);
}
