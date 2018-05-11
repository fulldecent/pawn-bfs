Pawn BFS
=============

When counting possible chess positions, pawns are most important because they can promote to any other piece. This quickly increases the number of potential "armies" on the board and the number of
possible diagrams. This project searches for possible pawn diagrams on the board, considering specifically how many diagonal captures are necessary to reach each. Our goal is to demonstrate a tight upper bound on the number of reachable chess positions.

Method
------

A pawn diagram includes the position and color of each pawn on the board. A diagram can be measured on the maximum remaining moves for white and black pawns. For example, at the start position, white and black both have 40 moves remaining (each pawn could progress 5 squares, with captures). With all pawns gone, zero moves are remaining.

This makes 41×41=1681 categories of pawn diagrams. Due to symmetries, only 861 categories are traversed. The included programs begin from the opening position and branch to all possible positions, creating useful tallies.

All programs require a 64-bit little endian system with a GNU compiler.

- `xxd` - Standard command line tool, use to create opening position file,
  which has one record
- `print` - Lists all records in a file for debugging
- `tally` - Summarizes the records in a file
- `bfs` - Reads each record in a file, finds all diagrams reachable by one
  move, saves those records to a new file
- `hsort` - Sorts records in a file using heapsort (qsort was slower on SSDs)
- `merge` - Removes duplicate records in a sorted file

## Record format

The opening position is represented with:

```sh
mkdir -p data
echo 'ff00 0000 00ff ffff 0000 0000 00ff 0000' | xxd -r -p > data/40-40.records
```

Records are stored in files named like: `data/W-B.records`

-   `W` — Maximum remaining moves for white

-   `B` — Maximum remaining moves for black


The 128-bit record format is (little endian values):

-   Bits 0…47 — 1 if pawn is on the board (file A…H, rank 7…2)
-   Bits 48…95 — 1 if the pawn is black
-   Bits 96…126 — required piece captures

For each record we are interested in the required number of white piece (♕♖♗♘♙) captures and black piece captures (♛♜♝♞♟). If a certain position is reachable by either 3W/2B or 2W/2B captures then only the latter is notable because you can easily find another way to remove a white piece from the board. You can encode an NxM efficient curve with N+M−1 bits. For example, the below is encoded `000010101101101` (the last bit can always be inferred).

![encode-captures](/Users/williamentriken/pawn-bfs/encode-captures.png)

## Tally phase

```sh
./tally data/40-40.records
```

The tally program reads each record and finds which number of white captures and black captures is compatible with that record (the efficiency curve). It also counts how many ways the remaining armies could be placed on the board. This also outputs the total number of chess diagrams.

## Branching phase

```sh
./bfs data/40-39.records
```

This will open the W40/B39 file where white pawns are in the opening position and one black pawn made progress. The BFS search will find records where:

* A white pawn is captured (W35/B39)
* A white pawn makes progress (W39/B39)
* A black pawn is captured (W40/B35) or (W40/B34)
* A black pawn makes progress (W40/B38)

In every case, one of the numbers will decrease. Outputs are saved to `data/W-B-branched-NEWW-NEWB.records`.

## Sorting / unique phase

```sh
cat data/*-*-branched-39-39.records > data/39-39.records
rm data/*-*-branched-39-39.records
./hsort data/39-39.records
./uniq data/39-39.records
```

The sorting program moves all records with the same (folded) pawn diagram to be consecutive. The uniquing program calculates the combined efficiency curve and truncates the file to remove duplicates.

Get started
-----------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo 'ff00 0000 00ff ffff 0000 0000 00ff 0000' | xxd -r -p > data/8-8-0-0.records
bfs data/8-8-0-0.records
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Then start BFSing through the `$(wc -l DRY)` number of record files. You need to
manage the dependency that `data/a-b-c-d` --\> `data/(<=a)-(<=b)-(>=c)-(>=d)`,
and the effect of folding on dependency (e.g. `4-3-10-2` =\> `3-3-2-8`). Then
you run `cat data/*.records.a-b-c-d > data/a-b-c-d.records` and all the sorting
and merging.

Project status
--------------

-   MERGE FUNCTION IS NOT MERGING THE POSSIBLE ARMIES
-   need to fix this before counting positions look for symmetric records, their
    possible armies should be symmetric, if not something is wrong
-   Tally and or bfs have a problem noting the number of symmetries
-   Passing SIGINT does not corrupt data in short (it’s a slow process)
-   This is 2^55.4, which would require 785 petabytes of storage (before folding) if all positions were reachable.

Random notes
------------

Get a random pawn diagram:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
shuf -e w w w w w w w w b b b b b b b b . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . | paste -d' ' - - - - - - - - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can intuitively conclude that a vast majority of these are not reachable, so
this project may be worthwhile.

Quickly view record files:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
xxd data/8-7-0-0.records.8-8-0-0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Discussion on Chess upper bounds:

-   <https://groups.google.com/forum/#!topic/rec.games.chess.computer/vmvI0ePH2kI>

-   <http://membres.multimania.fr/albillo/cmain.htm>

-   <http://wismuth.com/chess/chess.html>

-   <http://www.janko.at/Retros/>

-   <http://www.mathhelpforum.com/math-help/discrete-mathematics-set-theory-logic/115926-combinatorics-chessproblem-probably-will-take-some-time-answering.html>
