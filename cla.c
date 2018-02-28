#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<mpi.h>

// Compile Code: mpicc -g -Wall mpi-cla-io.c -o mpi-cla-io
// Example Run Code: mpirun -np 4 ./mpi-cla-io test_input_1.txt test_output_1.txt
// Both input and output files will 524490 bytes in size. The two additional
//    characters are due to a newline characters between each input and at the
//    end of the file.

// blokc size should be 16

#define HEX_INPUT_SIZE 262144

FILE *my_input_file = NULL;
FILE *my_output_file = NULL;

// Add 1 to array size because strings must be null terminated
char hex_input_a[HEX_INPUT_SIZE + 1] = {0};
char hex_input_b[HEX_INPUT_SIZE + 1] = {0};


void hexToBin(char *hex_a, char *hex_b int *bin_a, int *bin_b) {
	// Translate to binary
    char *chars = "0123456789ABCDEF";
    int i;
    int j;
    // For each char in input
    for(i = 0; i < HEX_INPUT_SIZE; ++i) {
        // Start with the last char, turn it into a char*
        char a_char[2] = {hex_a[HEX_INPUT_SIZE - 1 - i], '\0'};
        // Find its position in lookup string
        char *a_pos = strstr(chars, a_char);
        // Get the integer number corresponding to the hex char
        int a_index = a_pos - chars;
        char b_char[2] = {hex_b[HEX_INPUT_SIZE - 1 - i], '\0'};
        char *b_pos = strstr(chars, b_char);
        int b_index = b_pos - chars;
        
        // Each hex char is represented by four binary digits
        for(j = 0; j < 4; ++j) {
            // Copy the first bit of the number into the array
            bin_a[i * 4 + j] = a_index & 1;
            // Shift the number over by one bit so we can copy the next bit
            a_index >>= 1;
            bin_b[i * 4 + j] = b_index & 1;
            b_index >>= 1;
        }
    }
}

void binToHex(int *n, char *hex, int size) {
    int value = 0;
    char *chars = "0123456789ABCDEF";
    int hexLen = size / 4 - 1;
    
    int i;
    // For each bit in the array
    for(i = 0; i < size; ++i) {
        // Sum up the values of each bit
        value += n[i] << (i % 4);
        // Every four bits...
        if(i % 4 == 3) {
            // Convert value to a character and add it to the string
            hex[hexLen] = chars[value];
            --hexLen;
            value = 0;
        }
    }
}


int main(int argc, char** argv) {
    int my_mpi_size = -1;
	int my_mpi_rank = -1;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &my_mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_mpi_rank);

	if(argc != 3) {
		printf("Not sufficient arguments, only %d found \n", argc);
		exit(-1);
	}

	if(0 == my_mpi_rank) { // compare 0 first ensures == operator must be used and not just =
		printf("MPI Rank %d: Attempt to Read File Data \n", my_mpi_rank );

		if((my_input_file = fopen(argv[1], "r")) == NULL) {
			printf("Failed to open input data file: %s \n", argv[1]);
		}
		
		if((my_output_file = fopen( argv[2], "w")) == NULL) {
			printf("Failed to open output data file: %s \n", argv[2]);
		}

		fscanf(my_input_file, "%s %s", hex_input_a, hex_input_b);
		
		
		
		fprintf(my_output_file, "%s\n%s\n", hex_input_a, hex_input_b);

		printf("MPI Rank %d: Finished Reading and Write File Data \n", my_mpi_rank);

		fclose(my_input_file);
		fclose(my_output_file);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	printf("Rank %d: Hello World \n", my_mpi_rank);

	MPI_Finalize();
}
