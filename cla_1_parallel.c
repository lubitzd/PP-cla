#include <stdio.h>
#include <string.h>
#include <mpi.h>


int power(int a, int b) {
    int ret = 1;
    int i;
    for(i = 0; i < b; ++i) {
        ret *= a;
    }
    return ret;
}

void handleInput(int *a, int *b, int size) {
    // Take input from user
    char a_hex[64];
    char b_hex[64];
    scanf("%s", a_hex);
    scanf("%s", b_hex);
    
    // Translate to binary
    char *chars = "0123456789ABCDEF";
    int i;
    int j;
    // For each char in input
    for(i = 0; i < 64; ++i) {
        // Start with the last char, turn it into a char*
        char a_char[2] = {a_hex[63 - i], '\0'};
        // Find its position in lookup string
        char *a_pos = strstr(chars, a_char);
        // Get the integer number corresponding to the hex char
        int a_index = a_pos - chars;
        char b_char[2] = {b_hex[63 - i], '\0'};
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
}

void printHex(int *n, int size) {
    char hex[64];
    binToHex(n, hex, size);
    printf("%s\n", hex);
}

void printArray(int *n, int size) {
    int i;
    for(i = 0; i < size; ++i) {
        printf("%d", n[i]);
    }
    printf("\n");
}


void calcOneGenPro(int *g, int *p, int *gg, int *pp, int size) {
	// Calculate gg and pp for all [size] groups of 4
    int j;
    int i;
    for(j = 0; j < size; ++j) {
        i = j * 4;
        gg[j] = g[i + 3] | (p[i + 3] & g[i + 2])
                         | (p[i + 3] & p[i + 2] & g[i + 1])
                         | (p[i + 3] & p[i + 2] & p[i + 1] & g[i]);
        pp[j] = p[i + 3] & p[i + 2] & p[i + 1] & p[i];
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
        calcOneGenPro(gs[i - 1], ps[i - 1], gs[i], ps[i], size);
    }
}

int carry(int* c, int* prevc, int* g, int* p, int blocksize, int size) {
    //c[0] = 0;
    int taskID;
    int nTasks;
    MPI_Comm_rank(MPI_COMM_WORLD, &taskID);
    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);

    if(taskID == 0) {
        c[0] = 0;
    } else {
        MPI_Request req;
        MPI_Status stat;
        int c_in;
        MPI_Irecv(&c_in, 1, MPI_INT, taskID - 1, taskID, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &stat);
        c[0] = c_in;
    }

    int i;
    for(i = 1; i < size; ++i) {
        // Calculate carry in for part i, i.e. carry out for i-1
        if(i % blocksize == 0) {
            c[i] = g[i - 1] | (p[i - 1] &  prevc[i / blocksize]);
        } else {
            c[i] = g[i - 1] | (p[i - 1] & c[i - 1]);
        }
    }

   if(taskID) {}
   //TODO move MPI code out of function; it only happens once per thread
}


int main(int argc, char** argv) {
    int taskID;
    int nTasks;
    //int ierr;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskID); 
    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);
    
    int MAX_SIZE = 256;
    int BLOCK_SIZE = 4;
    int i;
    int a_in[MAX_SIZE];
    int b_in[MAX_SIZE];
    
    // How much of the input each task gets
    int size = MAX_SIZE / nTasks;
    int a[size];
    int b[size];

    int g[size];
    int p[size];
    int g_size = BLOCK_SIZE;
    int gg[size / g_size];
    int gp[size / g_size];
    int s_size = power(BLOCK_SIZE, 2);
    int sg[size / s_size];
    int sp[size / s_size];
  //  int ssg[MAX_SIZE / 64];
  //  int ssp[MAX_SIZE / 64];
    int* gs[4] = {g, gg, sg/*, ssg*/}; // size = log_{block_size}(MAX_SIZE) + 1
    int* ps[4] = {p, gp, sp/*, ssp*/};
    
    if(taskID == 0) {
        handleInput(a_in, b_in, MAX_SIZE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Scatter(a_in, size, MPI_INT, a, size, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
	
    calcAllGenPro(gs, ps, a, b, BLOCK_SIZE, size);
    
    MPI_Barrier(MPI_COMM_WORLD);

    // Calculate supersection carry for all 4 supersections
   // int ssc[MAX_SIZE / 64];
   // carry(ssc, NULL, ssg, ssp, 4, MAX_SIZE / 64);

    // Calculate section carry for all 16 sections
    int sc[size / s_size];
    carry(sc, /*ssc*/ NULL, sg, sp, BLOCK_SIZE, size / s_size);
    
    // Calculate group carry for all 64 groups
    int gc[size / g_size];
    carry(gc, sc, gg, gp, BLOCK_SIZE, size / g_size);
    
    // Calculate carry for all 256 bits
    int c[size];
    carry(c, gc, g, p, BLOCK_SIZE, size);
    
    // Calculate sum
    int sum[MAX_SIZE];
    for(i = 0; i < MAX_SIZE; ++i) {
        int carryIn;
        if(i == 0) {
            carryIn = 0;
        } else {
            carryIn = c[i];
        }
        sum[i] = a[i] ^ b[i] ^ carryIn;
    }
    
    printHex(sum, MAX_SIZE);
    
    return 0;
}
