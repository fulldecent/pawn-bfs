/*
 * Breadth-first search of all reachable pawn positions
 * (c) 2013 William Entriken
 *
 * tally
 * Counts the number of positions represented in a data/WPAWNS-BPAWNS-WPROGRESS-BPROGRESS.sorted file
 */

#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include <assert.h>

extern "C" {
#include "common.h"
}

#define MAXCOUNTPERMUTATIONS 10000

void findPermutationsWithNumPawnsAndProgress(int pawns, int progress, int permutations[MAXCOUNTPERMUTATIONS][6], int &count)
{
	count = 0;

	for (int rank2=0; rank2<=8; rank2++) {
		for (int rank3=0; rank3<=8; rank3++) {
			for (int rank4=0; rank4<=8; rank4++) {
				for (int rank5=0; rank5<=8; rank5++) {
					for (int rank6=0; rank6<=8; rank6++) {
						for (int rank7=0; rank7<=8; rank7++) {
							if (rank2+rank3+rank4+rank5+rank6+rank7 != pawns)
								continue;
							if (rank3*1+rank4*2+rank5*3+rank6*4+rank7*5 != progress)
								continue;
							permutations[count][0] = rank2;
							permutations[count][1] = rank3;
							permutations[count][2] = rank4;
							permutations[count][3] = rank5;
							permutations[count][4] = rank6;
							permutations[count][5] = rank7;
							count++;
							assert(count <= MAXCOUNTPERMUTATIONS);
						}
					}
				}
			}
		}
	}
}

int main (int argc, char *argv[])
{
	int wPawns, bPawns, wProgress, bProgress;
	int permutationsForWhite[MAXCOUNTPERMUTATIONS][6]; // 6 ranks
	int permutationsForBlack[MAXCOUNTPERMUTATIONS][6]; // 6 ranks
	int wNumPermutations, bNumPermutations;
	mpz_t waysToPlacePawns;
    mpz_t facts_mpz[65];                /* precompute fact(0..64) */
	int facts_int[10];

	mpz_init(waysToPlacePawns);
	for (int i=0; i<=64; i++) {
		mpz_init(facts_mpz[i]);
		mpz_fac_ui(facts_mpz[i], i);
	}
	for (int i=0; i<10; i++)
		facts_int[i] = i<=1 ? 1 : facts_int[i-1] * i;

/*
	sigaddset(&signals, SIGINT);··
*/

	if (argc != 2 || sscanf(argv[1], "data/%d-%d-%d-%d.sorted", &wPawns, &bPawns, &wProgress, &bProgress) != 4) {
		printf("usage: %s data/WPAWNS-BPAWNS-WPROGRESS-BPROGRESS.sorted\n", argv[0]);
		exit(1);
	}

	findPermutationsWithNumPawnsAndProgress(wPawns, wProgress, permutationsForWhite, wNumPermutations);
	findPermutationsWithNumPawnsAndProgress(bPawns, bProgress, permutationsForBlack, bNumPermutations);
	for (int i=0; i<wNumPermutations; i++) {
		for (int j=0; j<bNumPermutations; j++) {
			for (int k=0; k<6; k++)
				if (permutationsForWhite[i][k] + permutationsForBlack[j][5-k] > 8)
					goto break2; // wont all fit on this rank
				
			mpz_t waysToPlacePawnsInThisPermutation;
			mpz_init_set_ui(waysToPlacePawnsInThisPermutation, 1);
			for (int k=0; k<6; k++) {
				int waysToPlacePawnsInThisRow = facts_int[8];
				waysToPlacePawnsInThisRow /= facts_int[permutationsForWhite[i][k]];
				waysToPlacePawnsInThisRow /= facts_int[permutationsForBlack[j][5-k]];
				waysToPlacePawnsInThisRow /= facts_int[8 - permutationsForWhite[i][k] - permutationsForBlack[j][5-k]];
//printf("  %d\n", waysToPlacePawnsInThisRow);
				mpz_mul_ui(waysToPlacePawnsInThisPermutation, waysToPlacePawnsInThisPermutation, waysToPlacePawnsInThisRow);
			}
			mpz_add(waysToPlacePawns, waysToPlacePawns, waysToPlacePawnsInThisPermutation);		
//gmp_printf("%Zd\n", waysToPlacePawnsInThisPermutation);
break2:;
		}
	}
	printf("Considering diagrams with %d white and %d black pawns with %d and %d progress respectively\n",
	       wPawns, bPawns, wProgress, bProgress);
	if ((wPawns != bPawns) || (wProgress != bProgress))
		mpz_mul_ui(waysToPlacePawns, waysToPlacePawns, 2);
	gmp_printf("  Conceivable diagrams: %Zd (including symmetry)\n", waysToPlacePawns);

//////////////////////////////////////////

	FILE *fin;
	int64_t finsize, status = 0;
	RECORD record;
	char s[100]={0};
	mpz_t reachedPlacements;
	mpz_init(reachedPlacements);

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
		int symmetries;
		fread(&record, 1, 16, fin);
		symmetries = record_fold_180(&record) * record_fold_we(&record);
		mpz_add_ui(reachedPlacements, reachedPlacements, symmetries);
		status += 16;
	}

	gmp_printf("  Reached diagrams: %Zd (including symmetry)\n", reachedPlacements);


////////////////////	
	printf("  Reachable positions: TODO (and count folds)\n");

}
