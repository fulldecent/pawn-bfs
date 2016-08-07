Project intro
=============

When counting possible chess positions, pawns are an important part of the
process because they can become any other piece when promoted. This quickly
increases the number of potential "armies" on the board and the number of
possible diagrams. This project searches for possible pawn diagrams on the
board, considering specifically how many diagonal captures are necessary to
reach each. Hopefully this count will illustrate that fewer chess positions are
possible than previously thought.

Method
------

Every pawn diagram is noted for its number of white and black pawns and total
progress of each piece from its home square. This makes 35,721 categories of
pawns, and the precedents and antecedents of a given category are predictable.
Due to symmetries, only 20,598 categories are traversed.

The included programs begin from the opening position and branch to all possible
positions, creating useful tallies.

ALL PROGRAMS require a 64-bit little endian system with a GNU compiler.

Record Format
-------------

The opening position is represented with:

`mkdir -p data`

`echo 'ff00 0000 00ff ffff 0000 0000 00ff 0000' | xxd -r -p >
data/8-8-0-0.records  `

Records are stored in files named like: `data/A-B-C-D.records`

-   `A` — Number of white pawns

-   `B` — Number of black pawns

-   `C` — Sum forward progress of white pawns from home position

-   `D` — Sum forward progress of black pawns from home position

The 128-bit record format is (little endian values):

-   Bits 0…47 — 1 if pawn is on the board (file A…H, rank 7…2)

-   Bits 48…95 — 1 if the pawn is black

-   Bits 96…127 — the captures (to document)

Sorting / unique phase
----------------------

`./hsort data/8-8-0-0.records; ./uniq data/8-8-0-0.records`

Before a records file is branched, or when disk space needs saving, it is
sorted/uniqued. This process finds any record that "dominates" another:

a) the same pawn diagram

b) the same or greater number of white promotions

c) the same or greater number of black promotions

d) the same or lesser number of white captures of non-pawns

e) the same or lesser number of black captures of non-pawns

and deletes the dominated position.

### Branching phase

`./bfs data/8-8-0-0.records`

A records file is opened and each record inside is expanded through all possible
moves. These new positions are all saved based on the file-naming convention
above after being folded for symmetry.

### Tally phase

`./bfs data/8-8-0-0.records # creates the file data/8-8-0-0.tally`

This reads a record file and counts the number of chess positions for each
(white to move + black to move + opportunity for en passant) while taking into
consideration symmetry. The tally sums positions as they are distinct in \<white
promotions, black promotions, white captures of non pawns, black captures\>.

Programs
========

-   `xxd` - Standard command line tool, use to create opening position file,
    which has one record

-   `print` - Lists all records in a file for debugging

-   `tally` - Summarizes the records in a file

-   `bfs` - Reads each record in a file, finds all diagrams reachable by one
    move, saves those records to a new file

-   `hsort` - Sorts records in a file using heapsort (qsort was slower on SSDs)

-   `merge` - Removes duplicate records in a sorted file

Get started
===========

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
==============

-   MERGE FUNCTION IS NOT MERGING THE POSSIBLE ARMIES

-   need to fix this before counting positions look for symmetric records, their
    possible armies should be symmetric, if not something is wrong

-   Tally and or bfs have a problem noting the number of symmetries

-   Passing SIGINT does not corrupt data in short (it’s a slow process)

Notation
========

**Army** refers to pieces on the board other than the king.

White and black pawns are placed on the board on the home row. Each respective
move forward increases that pawn's **"progress"**.

**Symmetry and folding** are applied so we only consider boards where:

-   Pawns on left lexigraphically more than pawns on right, and

-   Either

    -   White pawns \> black pawns, or

    -   Equal pawns and white progress \> black progress, or

    -   Equal pawns, equal progress, and white arranged lexicographically before
        black

During counting, folded positions are counted multiple times, as appropriate.

Each pawn diagram on the board is represented by a 128-bit record. This record
is:

-   White and black pawn diagram

-   16x16 matrix of whether this diagram is reachable with a given size of white
    and black armies

-   TODO: explain binary coding scheme here

**Conceivable diagrams** are ways you could place pawns on a board with given
restrictions. **Reached diagrams** have tracable retrograde to the opening
position.

Conceivable diagrams
====================

The `tally` program will calculate the number of conceivable diagrams before it
calculates the actual reached diagrams found in the `data/` files. As a sanity
check, run:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
for w in {0..8}
do for b in {0..$w}
    do for wp in {0..$[w*5]}
        do for bp in {0..$[b*5]}
            do [ "$w" -eq "$b" ] && [ "$bp" -gt "$wp" ] && continue
            echo -ne "$w-$b-$wp-$bp\t"
            echo `./tally data/$w-$b-$wp-$bp.dry | grep 'Conceivable diagrams' | cut -d' ' -f 5`
        done
    done
done
done > DRY
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These numbers are divided by 1 billion and saved in `SCOREBOARD.ods`. You can
confirm by imagining ways to fit 2 white and 1 black marble in 48 slots:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
grep '^2-1-' DRY | cut -f2 | paste -sd+ - | bc
# Should be 48!/(48-2-1)!/2!/1!*2
# = 103 776
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

And also:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
grep '^8-8-' DRY | cut -f2 | paste -sd+ - | bc
# Should be 48!/(48-8-8)!/8!/8!
# = 29 019 905 518 636 890 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To count all conceivable:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cut -f2 DRY | paste -sd+ - | bc
# Should be sum(i=0..8) sum(j=0..8) 48!/(48-i-j)!/i!/j! 
# = 49 095 495 585 283 107
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is 2\^55.4, which would require 785 petabytes of storage (before folding)
if all positions were reachable.

Random notes
============

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
