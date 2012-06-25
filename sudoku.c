/*
 sudoku.c - A sudoku solver program
 Copyright 2005-2009, Ibán Cereijo Graña <ibancg at gmail dot com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define SOLVED		 0
#define ERROR		-1
#define INCOMPLETE	 1

#define GCOLUMN		0
#define GROW		1
#define GMATRIX		2

struct square_group_t; // forward definition.

// this structure models an square
struct square_t {
	// digit set, each bit codifies one possible digit for this square
	unsigned short int digits;

	// number of possible digits, this is our uncertainty measure: 1 means no
	// uncertainty, and 9 means maximum uncertainty.
	unsigned short int size;

	// each square belongs to three groups over which uniqueness constraints
	// are imposed
	struct square_group_t* group[3];
};

// square set over which an uniqueness constraint is imposed
struct square_group_t {
	struct square_t* square[9]; // squares that make up the set.
};

typedef struct square_t square_t;
typedef struct square_group_t square_group_t;

unsigned long int solutions = 0; // number of solutions found.
unsigned long int assumptions = 0; // number of assumptions made.
unsigned char unsolved_squares = 81; // unsolved squares, global uncertainty.
int first_solution = 1;
int search_all_solutions = 0;
int print_all_solutions = 0;
char* input_file_name = NULL;
char* output_file_name = NULL;
FILE* output_file = NULL;
char buff[80];

square_t square[9][9]; // 9x9 squares table.
// 27 restriction groups (9 rows + 9 columns + 9 matrixes).
square_group_t square_group[3][9];

void alarm_callback(int h) {
	printf("%lu solutions found for now (%lu assumptions made)\n", solutions,
			assumptions);
	signal(SIGALRM, alarm_callback);
	alarm(1);
}

void ctrl_c_callback(int h) {
	if (output_file_name)
		fclose(output_file);
	exit(1);
}

void print_result(square_t table[][9]);

//-----------------------------------------------------------------------------

// reduces the square uncertainty by removing from the possibilities the digit
// set coded in mask
int uncertainty_reduction(square_t* s, unsigned short int mask) {
	register int group, i, n;
	int r;
	square_t* s2;
	square_t* uncontained[9];
	unsigned short int uncontained_set_size;

	if (!(s->digits & mask))
		return INCOMPLETE; // no changes.

	s->digits &= ~mask;
	n = __builtin_popcount(s->digits); // remaining digits
	s->size = n;

	if (n == 0)
		return ERROR;

	if (n == 1) { // the square has been fixed
		// as the fixed square belongs to three uniqueness restriction groups,
		// lets make some uncertainty reduction to the squares of that groups.
		// 1st order reduction
		for (group = 0; group < 3; group++)
			for (i = 0; i < 9; i++) {
				s2 = s->group[group]->square[i];
				if ((s2 != s) && (s2->digits & s->digits)) {
					r = uncertainty_reduction(s2, s->digits);
					if (r != INCOMPLETE)
						return r;
				}
			}

		// if the fixed square were the last one, the puzzle is solved
		if (--unsolved_squares == 0) {
			if (print_all_solutions || first_solution)
				print_result(square);
			if (first_solution)
				first_solution = 0;
			solutions++;
			return SOLVED;
		}
	} else {

		// N-th order reductions
		for (group = 0; group < 3; group++) {
			// lets see the squares with states contained in this state
			uncontained_set_size = 0;
			for (i = 0; i < 9; i++) {
				s2 = s->group[group]->square[i];
				if ((s2 != s) && ((s2->digits | s->digits) != s->digits)) {
					uncontained[uncontained_set_size++] = s2;
				}
			}

			// if the set of squares with contained states has cardinal n, then
			// the other squares can't have any of my possible digits.
			if (uncontained_set_size == 9 - n) {
				for (i = 0; i < uncontained_set_size; i++) {
					r = uncertainty_reduction(uncontained[i], s->digits);
					if (r != INCOMPLETE)
						return r;
				}
			}

		}

	}

	if (unsolved_squares > 0)
		return INCOMPLETE;
	else if (unsolved_squares == 0)
		return SOLVED;
	else
		return ERROR;
}

//----------------------------------------------------------------------------

// tries to make assumptions in the squares with uncertainty
int make_assumption(int i, int j) {
	register int k;
	int r;
	square_t *s;
	square_t square_backup[9][9]; // backup table
	unsigned char unsolved_squares_backup;

	// find the next square with uncertainty
	while (square[i][j].size == 1) {
		j++;
		if (j == 10) {
			j = 0;
			i++;

			if (i == 10)
				return INCOMPLETE; // end reached
		}
	}

	s = &square[i][j];

	for (k = 0; k < 9; k++) // try all the possible digits in this square
		if ((s->digits >> k) & 1) { // check if it's a possible digit

			assumptions++; // a new set digit is a new assumption

			// backup
			memcpy(square_backup, square, 81 * sizeof(square_t));
			unsolved_squares_backup = unsolved_squares;

			// a fixed digit reduces the uncertainty in this way
			r = uncertainty_reduction(s, ~(1 << k));
			if (r == INCOMPLETE) {
				// if the last assumption didn't solve the puzzle, lets make an
				// assumption in the next square
				r = make_assumption(i, j);
			}

			if ((r == SOLVED) && !search_all_solutions)
				return SOLVED;

			// restore.
			memcpy(square, square_backup, 81 * sizeof(square_t));
			unsolved_squares = unsolved_squares_backup;
		}

	return INCOMPLETE;
}

//----------------------------------------------------------------------------

void parse_input_file(square_t table[][9]) {
	const char* unsolved_char = "-.*0";
	FILE* f = stdin; // default.
	char c;
	int i = 0;
	int j = 0;

	printf("reading input table ...\n");

	if (input_file_name)
		if ((f = fopen(input_file_name, "rt")) == NULL) {
			sprintf(buff, "unable to open input file %s", input_file_name);
			perror(buff);
			exit(-1);
		}

	while (fscanf(f, "%c", &c) > 0)
		if (strchr(unsolved_char, c) || ((c >= '1') && (c <= '9'))) {
			if ((c >= '1') && (c <= '9')) {
				table[i][j].digits = 1 << (c - '1');
				table[i][j].size = 1;
			}
			if (++i == 9) {
				j++;
				i = 0;
			}
		}

	if (input_file_name)
		fclose(f);

	if ((i != 0) || (j != 9)) {
		printf("incorrect input file format\n");
		exit(-1);
	}
}

//----------------------------------------------------------------------------

void print_result(square_t table[][9]) {
	int i, j, n;
	for (j = 0; j < 9; j++) {
		for (i = 0; i < 9; i++) {
			n = table[i][j].size;
			fprintf(output_file, "%c ", (n > 1) ? '-' : ((n < 1) ? '*'
					: __builtin_ffs(table[i][j].digits) + '0'));
		}
		fprintf(output_file, "\n");
	}
	fprintf(output_file, "\n");
}

//----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	int i, j, squarex, squarey, r, ich;
	square_t* s;
	const char* const op_short = "i:o:aph";
	const struct option op_long[] = { //
			{ "input", 1, NULL, 'i' }, //
					{ "output", 1, NULL, 'o' },//
					{ "all", 0, NULL, 'a' },//
					{ "print-all", 0, NULL, 'p' },//
					{ "help", 0, NULL, 'h' }//
			};
	square_t square_in[9][9];

	output_file = stdout;

	while ((ich = getopt_long(argc, argv, op_short, op_long, NULL)) != EOF) {
		switch (ich) {
		case 'i':
			input_file_name = optarg;
			break;
		case 'o':
			output_file_name = optarg;
			if (output_file_name)
				if ((output_file = fopen(output_file_name, "wt")) == NULL) {
					sprintf(buff, "unable to open output file %s",
							input_file_name);
					perror(buff);
					exit(-1);
				}
			break;
		case 'a':
			search_all_solutions = 1;
			break;
		case 'p':
			search_all_solutions = 1;
			print_all_solutions = 1;
			break;
		case 'h':
			printf("Usage: sudoku [-a] [-p] [-i input_file] [-o output_file]");
			printf("\nSudoku solver.\n");
			printf("Example: sudoku -a -i table_in.txt -o table_out.txt\n\n");
			printf("Options:\n");
			printf("  -i, --input=FILE   uses FILE as the input table\n");
			printf("  -o, --output=FILE  write the solutions to FILE\n");
			printf("  -a, --search-all   search all the solutions\n");
			printf("  -p, --print-all    search and print all the solutions\n");
			printf("  -h, --help         display this help and exit\n");
			printf("\n");
			exit(0);
		default:
			printf("Usage: sudoku [-a] [-p] [-i input_file] [-o output_file]\n");
			printf("Try sudoku --help for more information.\n");
			exit(0);
		}
	}

	// initialization
	for (i = 0; i < 9; i++)
		for (j = 0; j < 9; j++) {

			square_in[i][j].digits = 0x01ff; // square initialization
			square_in[i][j].size = 9; // all the bits are set on
			square[i][j].digits = 0x01ff; // square initialization
			square[i][j].size = 9; // all the bits are set on

			// square to cell assignation
			s = &square[i][j];
			square_group[GROW][i].square[j] = s;
			s->group[GROW] = &square_group[GROW][i];

			s = &square[j][i];
			square_group[GCOLUMN][i].square[j] = s;
			s->group[GCOLUMN] = &square_group[GCOLUMN][i];

			squarex = 3 * (i % 3) + (j % 3);
			squarey = 3 * (i / 3) + (j / 3);
			s = &square[squarex][squarey];
			square_group[GMATRIX][i].square[j] = s;
			s->group[GMATRIX] = &square_group[GMATRIX][i];
		}

	parse_input_file(square_in);
	print_result(square_in); // prints the initial table

	if (search_all_solutions)
		printf("searching all the solutions\n");
	if (print_all_solutions)
		printf("printing all solutions\n");

	signal(SIGALRM, alarm_callback);
	signal(SIGINT, ctrl_c_callback);
	alarm(1);

	r = INCOMPLETE;

	// starts simple reductions
	for (j = 0; j < 9; j++) {
		for (i = 0; i < 9; i++)
			if (square_in[i][j].size == 1) {
				// a fixed digit reduces the uncertainty in this way
				r = uncertainty_reduction(&square[i][j],
						~square_in[i][j].digits);
			}
	}

	if (r == INCOMPLETE)
		r = make_assumption(0, 0);

	if (r == ERROR) {
		printf("error: the puzzle cannot be solved!\n");
		exit(-1);
	}

	printf("%lu solutions found (%lu assumptions made).\n", solutions,
			assumptions);

	if (output_file_name)
		fclose(output_file);

	return 0;
}
