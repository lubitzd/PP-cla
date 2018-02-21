#include "stdio.h"
#include "string.h"


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


int main(int argc, char** argv) {
    int MAX_SIZE = 256;
	int i;
    int a[MAX_SIZE];
    int b[MAX_SIZE];
	
    int g[MAX_SIZE];
    int p[MAX_SIZE];
    int gg[MAX_SIZE / 4];
    int gp[MAX_SIZE / 4];
    int sg[MAX_SIZE / 16];
    int sp[MAX_SIZE / 16];
    int ssg[MAX_SIZE / 64];
    int ssp[MAX_SIZE / 64];
	int* gs[4] = {g, gg, sg, ssg}; // size = log_{block_size}(MAX_SIZE) + 1
	int* ps[4] = {p, gp, sp, ssp};
    
    handleInput(a, b, MAX_SIZE);
	
	calcAllGenPro(gs, ps, a, b, 4, MAX_SIZE);
    
    // Calculate generate and propagate for all 256 bits
	
    // Calculate group gen and pro for all 64 groups of 4 bits
    /*calcOneGenPro(g, p, gg, gp, MAX_SIZE / 4);
    
    // Calculate section gen and pro for all 16 sections of 4 groups
    calcOneGenPro(gg, gp, sg, sp, MAX_SIZE / 16);
    
    // Calculate supersection gen and pro for all 4 supersections of 4 sections
    calcOneGenPro(sg, sp, ssg, ssp, MAX_SIZE / 64);*/
    
    // Calculate supersection carry for all 4 supersections
    int ssc[MAX_SIZE / 64];
	int l;
    for(l = 0; l < MAX_SIZE / 64; ++l) {
        // If this is the first carry operation, the carry in is 0
        int carryIn;
        if(l == 0) {
            carryIn = 0;
        } else {
            carryIn = ssc[l - 1];
        }
        
        ssc[l] = ssg[l] | (ssp[l] & carryIn);
    }
    
    // Calculate section carry for all 16 sections
    int sc[MAX_SIZE / 16];
	int k;
    for(k = 0; k < MAX_SIZE / 16; ++k) {
        // If this is the first carry operation, the carry in is 0
        int carryIn;
        if(k == 0) {
            carryIn = 0;
        } else if(k % 4 == 0) {
            // If this is the first carry in of the supersection,
            //  use the previous supersection carry
            carryIn = ssc[k / 4 - 1];
        } else {
            carryIn = sc[k - 1];
        }
        
        sc[k] = sg[k] | (sp[k] & carryIn);
    }
    
    // Calculate group carry for all 64 groups
    int gc[MAX_SIZE / 4];
	int j;
    for(j = 0; j < MAX_SIZE / 4; ++j) {
        // If this is the first carry operation, the carry in is 0
        int carryIn;
        if(j == 0) {
            carryIn = 0;
        } else if(j % 4 == 0) {
            // If this is the first carry in of the section,
            //  use the previous section carry
            carryIn = sc[j / 4 - 1];
        } else {
            carryIn = gc[j - 1];
        }
        
        gc[j] = gg[j] | (gp[j] & carryIn);
    }
    
    // Calculate carry for all 256 bits
    int c[MAX_SIZE];
    for(i = 0; i < MAX_SIZE; ++i) {
        // If this is the first carry operation, the carry in is 0
        int carryIn;
        if(i == 0) {
            carryIn = 0;
        } else if(i % 4 == 0) {
            // If this is the first carry in of the group,
            //  use the previous group carry
            carryIn = gc[i / 4 - 1];
        } else {
            carryIn = c[i - 1];
        }
        
        c[i] = g[i] | (p[i] & carryIn);
    }
    
    // Calculate sum
    int sum[MAX_SIZE];
    for(i = 0; i < MAX_SIZE; ++i) {
        int carryIn;
        if(i == 0) {
            carryIn = 0;
        } else {
            carryIn = c[i - 1];
        }
        sum[i] = a[i] ^ b[i] ^ carryIn;
    }
    
    printHex(sum, MAX_SIZE);
    
    return 0;
}