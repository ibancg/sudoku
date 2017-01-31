
Description
===========

This program can solve a sudoku puzzle.


Compiling
=========

Compile it entering the command "make". You will need the GCC compiler and the
GNU Make tool.


Running
=======

	Usage: sudoku [-a] [-p] [-i input_file] [-o output_file]
	Sudoku solver.
	Exampe: sudoku -a -i table_in.txt -o table_out.txt

	Options:
	  -i, --input=FILE   uses FILE as the input table
	  -o, --output=FILE  write the solutions to FILE
	  -a, --search-all   search all the solutions
	  -p, --print-all    search and print all the solutions
	  -h, --help         display the help and exit

If you don't specify the input or the output parameters, the stdin and stdout
will be used.

If the search takes too time you will be noticed periodically of the progress.

Board Fotmats
=============

The input format is quite flexible, you only have to respect that any digit
between '1' and '9' will be considered as a fixed digit, and a '0','.','-' or
'*' will be considered as a blank. Any other character will be ignored, so the
following formats are accepted for the same table.
	
	.2.4.37.........32........4.4.2...7.8...5.........1...5.....9...3.9....7..1..86..
	
	. 2 . | 4 . 3 | 7 . .
	. . . | . . . | . 3 2
	. . . | . . . | . . 4
	======+=======+======
	. 4 . | 2 . . | . 7 .
	8 . . | . 5 . | . . .
	. . . | . . 1 | . . .
	======+=======+======
	5 . . | . . . | 9 . .
	. 3 . | 9 . . | . . 7
	. . 1 | . . 8 | 6 . .

	- 2 - 4 - 3 7 - - 
	- - - - - - - 3 2 
	- - - - - - - - 4 
	- 4 - 2 - - - 7 - 
	8 - - - 5 - - - - 
	- - - - - 1 - - - 
	5 - - - - - 9 - - 
	- 3 - 9 - - - - 7 
	- - 1 - - 8 6 - - 

Also, the last format is the one used to display the solutions.

