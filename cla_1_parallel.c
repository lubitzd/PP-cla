#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>


int power(int a, int b) {
    int ret = 1;
    int i;
    for(i = 0; i < b; ++i) {
        ret *= a;
    }
    return ret;
}

#define HEX_IN_SIZE 262144 
void handleInput(int *a, int *b, int size, char** argv) {
    int hex_size = size / 4;
   printf("%d size\n", hex_size );
    // Take input from user
    char a_hex[HEX_IN_SIZE + 1] = {0};  // 262145]; //hex_size + 1];
    char b_hex[HEX_IN_SIZE + 1] = {0};  //262145]; //hex_size + 1];

    FILE *my_input_file = NULL;
    FILE *my_output_file = NULL;
    if((my_input_file = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open input data file: %s \n", argv[1]);
    }
    if((my_output_file = fopen(argv[2], "w")) == NULL) {
        printf("Failed to open input data file: %s \n", argv[2]);
    }

    printf("hello\n");
    fscanf(my_input_file, "%s %s", a_hex, b_hex);
    printf("hi\n");
    fprintf(my_output_file, "%s\n%s\n", a_hex, b_hex);
                        
    fclose(my_input_file);
    fclose(my_output_file);


   // scanf("%s", a_hex);
   // scanf("%s", b_hex);
    
    // Translate to binary
    char *chars = "0123456789ABCDEF";
    int i;
    int j;
    // For each char in input
    for(i = 0; i < HEX_IN_SIZE; ++i) {
        //printf("I'm %d\n", i);
        // Start with the last char, turn it into a char*
        char a_char[2] = {a_hex[HEX_IN_SIZE - 1 - i], '\0'};
        // Find its position in lookup string
        char *a_pos = strstr(chars, a_char);
        // Get the integer number corresponding to the hex char
        int a_index = a_pos - chars;
        char b_char[2] = {b_hex[HEX_IN_SIZE - 1 - i], '\0'};
        char *b_pos = strstr(chars, b_char);
        int b_index = b_pos - chars;
        
        // Each hex char is represented by four binary digits
        for(j = 0; j < 4; ++j) {
            // Copy the first bit of the number into the array
            a[i * 4 + j] = a_index & 1;
            // Shift the number over by one bit so we can copy the next bit
            a_index >>= 1;
            b[i * 4 + j] = b_index & 1;
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
    hex[size / 4] = '\0';
}

void printHex(int *n, int size, char** argv) {
    char hex[size / 4 + 1];
    binToHex(n, hex, size);
    
    FILE *my_output_file=NULL;
    if((my_output_file = fopen(argv[2], "a")) == NULL) {
        printf("Failed to open input data file: %s \n", argv[2]);
    }
    fprintf(my_output_file, "%s\n", hex);
    fclose(my_output_file);
    printf("%s\n", hex);
}

void printArray(int *n, int size) {
    int i;
    for(i = 0; i < size; ++i) {
        printf("%d", n[i]);
    }
    printf("\n");
    fflush(stdout);
}

int calculateLevels(int block_size, int size) {
    int levels = 1;
    while(size > block_size) {
        size /= block_size;
        ++levels;
    }
    return levels;
}


void calcOneGenPro(int *g, int *p, int *gg, int *pp, int block_size, int size) {
    // Calculate gg and pp for all [size] groups of 4
    int j;
    int i;
    int k;
    for(j = 0; j < size; ++j) {
        i = j * block_size;
        

        int prop = 1;
        gg[j] = g[i + block_size - 1];
        for(k = 0; k < block_size - 1; ++k) {
            prop &= p[i + block_size - 1 - k];
            gg[j] |= g[i + block_size - 2 - k]
                     & prop;
        }

    //    gg[j] = g[i + 3] | (p[i + 3] & g[i + 2])
   //                      | (p[i + 3] & p[i + 2] & g[i + 1])
    //                     | (p[i + 3] & p[i + 2] & p[i + 1] & g[i]);
    
        pp[j] = prop & p[i];

    //    pp[j] = p[i + 3] & p[i + 2] & p[i + 1] & p[i];
    }
}

void calcAllGenPro(int **gs, int **ps, int *a, int *b, int block_size, int max_size) {
    int i;
    // Calculate generate and propagate for all bits
    for(i = 0; i < max_size; ++i) {
        gs[0][i] = a[i] & b[i];
        ps[0][i] = a[i] | b[i];
    }
    
    int size;
    for(i = 1, size = max_size / block_size;
                size > block_size; ++i) {
        size = max_size / power(block_size, i);
        calcOneGenPro(gs[i - 1], ps[i - 1], gs[i], ps[i], block_size, size);
    }
}

int carry(int c_in, int* c, int* prevc, int* g, int* p, int blocksize, int size) {
    c[0] = c_in;
    
    int i;
    for(i = 1; i < size; ++i) {
        // Calculate carry in for part i, i.e. carry out for i-1
        if(i % blocksize == 0) {
            c[i] = g[i - 1] | (p[i - 1] &  prevc[i / blocksize]);
        } else {
            c[i] = g[i - 1] | (p[i - 1] & c[i - 1]);
        }
    }
    
    // Return final carry out
    return g[size - 1] | (p[size - 1] & c[size - 1]);
}

void carryAll(int c_in, int** cs, int** gs, int** ps, int block_size, int* sizes, int levels) {
    int i;
    for(i = levels - 2; i >= 0; --i) {
        carry(c_in, cs[i], cs[i+1], gs[i], ps[i], block_size, sizes[i]);
        MPI_Barrier(MPI_COMM_WORLD);
    }
}


int main(int argc, char** argv) {
    int taskID;
    int nTasks;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskID); 
    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);
    
   // printf("%d: here\n", taskID);
   // fflush(stdout);

    int MAX_SIZE = 1048576;
    int BLOCK_SIZE = 16;
    int i;
 //   printf("aaa\n");
    int* a_in = (int*) calloc(MAX_SIZE, sizeof(int));
    int* b_in = (int*) calloc(MAX_SIZE, sizeof(int)); 
  //   printf("bbb\n");
  //  int a_in[MAX_SIZE];
  //  int b_in[MAX_SIZE];
    
   // printf("%d: A\n", taskID);
   //fflush(stdout);

    // How much of the input each task gets
    int size = MAX_SIZE / nTasks;
    int a[size];
    int b[size];
  //   printf("ccc\n");

    // Create generate and propegate arrays
    int levels = calculateLevels(BLOCK_SIZE, size);
    int* gs[levels];
    int* ps[levels];
    int sizes[levels];
    for(i = 0; i < levels; ++i) {
        sizes[i] = size / power(BLOCK_SIZE, i);
        gs[i] = (int*) calloc(sizes[i], sizeof(int));
        ps[i] = (int*) calloc(sizes[i], sizeof(int));
    }


   /* int g[size];
    int p[size];
    int g_size = BLOCK_SIZE;
    int gg[size / g_size];
    int gp[size / g_size];
    int s_size = g_size * BLOCK_SIZE;
    int sg[size / s_size];
    int sp[size / s_size];
    int ss_size = s_size * BLOCK_SIZE;
    int ssg[size / ss_size];
    int ssp[size / ss_size];*/
    //int* gs[4] = {g, gg, sg, ssg}; // size = log_{block_size}(MAX_SIZE) + 1
    //int* ps[4] = {p, gp, sp, ssp};
    
    if(taskID == 0) {
        handleInput(a_in, b_in, MAX_SIZE, argv);
    }

   printf("Task %d after reading input\n", taskID);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Scatter(a_in, size, MPI_INT, a, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(b_in, size, MPI_INT, b, size, MPI_INT, 0, MPI_COMM_WORLD); 

    printf("Task %d after scattering\n", taskID);
    MPI_Barrier(MPI_COMM_WORLD);

    calcAllGenPro(gs, ps, a, b, BLOCK_SIZE, size);
    
    printf("Task %d after calculating gen/pro\n", taskID);
    MPI_Barrier(MPI_COMM_WORLD);
    
    
    
   // int ssc[sizes[3]];
    //int sc[sizes[2]];
   // int gc[sizes[1]];
   // int c[sizes[0]];
    
    int* cs[levels];
    for(i = 0; i < levels; ++i) {
        cs[i] = (int*) calloc(sizes[i], sizeof(int));
    }
    int c_in;
    // carryAll(int c_in, int** cs, int** gs, int** ps, int block_size, int* sizes, int levels)
    
    MPI_Request req;
    MPI_Status stat;
    // Don't receive c_in if it's the first task
    if(taskID == 0) {
        c_in = 0;
    } else {
        MPI_Irecv(&c_in, 1, MPI_INT, taskID - 1, taskID, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &stat);
       // printf("%d: received %d\n", taskID, c_in);
       // fflush(stdout);
    }

   // printf("%d: recieved\n", taskID);
   // fflush(stdout);
   
    // Calculate upper level carry
    //ssc[0] = c_in;
    int c_out = carry(c_in, cs[levels-1], NULL, gs[levels-1], ps[levels-1], BLOCK_SIZE, sizes[levels-1]);
    
    // Don't send if it's the last one
    if(taskID != (nTasks - 1)) {
        // Send cout
        MPI_Isend(&c_out, 1, MPI_INT, taskID + 1, taskID + 1, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &stat);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    carryAll(c_in, cs, gs, ps, BLOCK_SIZE, sizes, levels);
    
    /**
    // Calculate section carry for all sections
    //sc[0] = c_in;
    carry(c_in, sc, ssc, gs[2], ps[2], BLOCK_SIZE, sizes[2]);

    MPI_Barrier(MPI_COMM_WORLD);
    
    // Calculate group carry for all 64 groups
    //gc[0] = c_in;
    carry(c_in, gc, sc, gs[1], ps[1], BLOCK_SIZE, sizes[1]);
    
    MPI_Barrier(MPI_COMM_WORLD);

    // Calculate carry for all 256 bits
   // c[0] = c_in;
    carry(c_in, c, gc, gs[0], ps[0], BLOCK_SIZE, sizes[0]);
    
    MPI_Barrier(MPI_COMM_WORLD);
*/
    // Calculate sum
    int sum[size];
    for(i = 0; i < size; ++i) {
        sum[i] = a[i] ^ b[i] ^ cs[0][i];
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    
    int total[MAX_SIZE];
    MPI_Gather(sum, size, MPI_INT, total, size, MPI_INT, 0, MPI_COMM_WORLD);
   
    MPI_Barrier(MPI_COMM_WORLD);

    if(taskID == 0) {
       // printArray(a_in, MAX_SIZE);
       // printArray(b_in, MAX_SIZE);
       // printArray(c, size);
       // printArray(total, MAX_SIZE);
        printHex(total, MAX_SIZE, argv);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for(i = 0; i < levels; ++i) {
        free(gs[i]);
        free(ps[i]);
        free(cs[i]);
    }
    free(a_in);
    free(b_in);

    MPI_Finalize();

    return 0;
}
