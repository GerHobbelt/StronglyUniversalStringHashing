/////////////////////////////////////
// This C code is a companion to the paper
//
/////////////////////////////////////

//
// this code will hash strings of 64-bit characters. To use on
// strings of 8-bit characters, you may need some adequate padding.
//
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#ifdef __AVX__
#define __PCLMUL__ 1
#endif

typedef unsigned long long ticks;

// Taken from stackoverflow (see http://stackoverflow.com/questions/3830883/cpu-cycle-count-based-profiling-in-c-c-linux-x86-64)
// Can give nonsensical results on multi-core AMD processors.
ticks rdtsc() {
	unsigned int lo, hi;
	asm volatile (
			"cpuid \n" /* serializing */
			"rdtsc"
			: "=a"(lo), "=d"(hi) /* outputs */
			: "a"(0) /* inputs */
			: "%ebx", "%ecx");
	/* clobbers*/
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

ticks startRDTSC(void) {
	return rdtsc();
}

ticks stopRDTSCP(void) {
	return rdtsc();
}
// start and stop are as recommended by
// Gabriele Paoloni, How to Benchmark Code Execution Times on Intel® IA-32 and IA-64 Instruction Set Architectures
// September 2010
// http://edc.intel.com/Link.aspx?id=3954

static __inline__ ticks fancystartRDTSC(void) {
	unsigned cycles_low, cycles_high;
	asm volatile ("CPUID\n\t"
			"RDTSC\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
			"%rax", "%rbx", "%rcx", "%rdx");
	return ((ticks) cycles_high << 32) | cycles_low;
}

static __inline__ ticks fancystopRDTSCP(void) {
	unsigned cycles_low, cycles_high;
/// This should work fine on most machines, if the RDTSCP thing
/// fails for you, use the  rdtsc() call instead.
	asm volatile("RDTSCP\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t"
			"CPUID\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax",
			"%rbx", "%rcx", "%rdx");
	return ((ticks) cycles_high << 32) | cycles_low;
}

#include "hashfunctions32bits.h"
#include "hashfunctions64bits.h"


#include "clmulhashfunctions32bits.h"
#include "clmulhashfunctions64bits.h"
#include "clmulpoly64bits.h"
#include "clmulhierarchical64bits.h"

#define HowManyFunctions64 3


hashFunction64 funcArr64[HowManyFunctions64] = {&hashVHASH64,
                &CLHASH,
		    &hashCity};

const char* functionnames64[HowManyFunctions64] = {
	"64-bit VHASH        ",
	"64-bit CLHASH       ",
	"Google's City       ",

};

int main(int c, char ** arg) {
	(void) (c);
	(void) (arg);
	const int N = 524288; // should be divisible by two!
	int i, k, j;
	int length;
	int elapsed;
	int SHORTTRIALS;
	hashFunction thisfunc;
	const char * functionname;
	ticks bef, aft;
	struct timeval start, finish;
	uint64_t randbuffer[128] __attribute__ ((aligned (16)));
	uint32_t sumToFoolCompiler = 0;
	uint64_t * intstring = malloc(N*sizeof(uint64_t))  ; // // could force 16-byte alignment with  __attribute__ ((aligned (16)));
	for (i = 0; i < 128; ++i) {
		randbuffer[i] = rand() | ((uint64_t)(rand()) << 32);
	}
	for (i = 0; i < N; ++i) {
		intstring[i] =  rand() | ((uint64_t)(rand()) << 32);
	}
	printf(
			"#Reporting the number of cycles per byte.\n");
	printf("#First number is input length in  8-byte words.\n");
	printf("#          ");
	for (i = 0; i < HowManyFunctions64; ++i) {
		printf("%s ", functionnames64[i]);
	}
	printf("\n");
	fflush(stdout);
	for(length = 1; length<=N; length+=1) {
		SHORTTRIALS = 40000000/length;
		printf("%8d \t\t", length );
		hashFunction64 thisfunc64;
		for (i = 0; i < HowManyFunctions64; ++i) {
			thisfunc64 =  funcArr64[i];
			functionname = functionnames64[i];
			gettimeofday(&start, 0);
			bef = startRDTSC();
			//assert(length / 2 * 2 == length);
			for (j = 0; j < SHORTTRIALS; ++j) {
				sumToFoolCompiler += thisfunc64(randbuffer,
						intstring,length );
                        }
			aft = stopRDTSCP();
			gettimeofday(&finish, 0);
			elapsed = (1000000 * (finish.tv_sec - start.tv_sec)
					+ (finish.tv_usec - start.tv_usec));
			printf(
					" %f ",
					(aft - bef) * 1.0 / (8.0 * SHORTTRIALS * length));
		}
		printf("\n");
	}
        free(intstring);
	printf("# ignore this #%d\n", sumToFoolCompiler);

}

