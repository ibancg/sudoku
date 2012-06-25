CC=gcc -O3

sudoku: sudoku.c
	$(CC) -o $@ $<
	
clean: 
	@rm -f sudoku *.o
