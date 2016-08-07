/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * Utility functions
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/* Stores pawns for each player in a bitboard,
 * stats on the pawns, and
 * possible army combinations */
typedef struct RECORD {
	uint64_t whiteBlack;
	uint64_t white;
	int numWhite;
	int numBlack;
	int progressWhite;
	int progressBlack;
	int _numBlackPossibleForGivenWhite[16]; /* private */
} RECORD;

/* Bitboard masks */
extern const uint64_t BOARD;
extern const uint64_t FILEA;
extern const uint64_t FILEH;
extern const uint64_t RANK7;
extern const uint64_t RANK6;
extern const uint64_t RANK3;
extern const uint64_t RANK2;
extern const uint64_t CAPDATA;

void record_print(RECORD record);
int record_fold_180(RECORD *record);
int record_fold_we(RECORD *record);

void record_unpack_armies(RECORD *record); /* Must unpack before using below fcns, and pack after */
void record_pack_armies(RECORD *record);
int record_can_take_white(RECORD record);
int record_can_take_black(RECORD record);
void record_take_white(RECORD *record); /* Reduces one white piece from each possible army */
void record_take_black(RECORD *record); /* Reduces one white piece from each possible army */
RECORD record_union(RECORD a, RECORD b); /* Set union of possible armies */
int record_army_is_possible(RECORD record, int whiteArmy, int blackArmy);

#endif /* COMMON_H */
