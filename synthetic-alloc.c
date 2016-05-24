#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

/*
   The program read pairs of size (in MB) and time (in seconds) from
   the standard input. The program tries to allocate up to size MB in
   the specified time. For instance, for the next sequence
   30 30
   15 60
   15 30
   the program 1) allocates up to 30 MB in the first 30 seconds of
   execution, then 2) reduces the allocated memory down to 15 MB in
   the next 60 seconds, and finally 3) maintains that during the next
   30 seconds. The next figure shows the evolution of the reserved
   memory.

   30MB - |    *
          |   *    * 
   15MB - |  *         *****
          | *      
    0MB - |*
          --------------------
           |   |   |   |   | 
          0s  30s 60s 90s 120s 
*/

void *ptr = NULL;  /* Array pointer */
size_t ns = 0; /* Size of the array in bytes */
pthread_mutex_t m_ns; /* Mutex about the access to the array */
time_t ot; /* Origin of time */

void * print_flops(void *);

/* Print the localtime */
void printlt(void) {
	char outstr[200];
	time_t t;
	struct tm *tmp;
	int ierr;
	
	t = time(NULL);
	tmp = localtime(&t); assert(tmp);
	
	ierr = strftime(outstr, sizeof(outstr), "%F %T", tmp); assert(ierr > 0);
	printf("%s ", outstr);
}

int main(int na, char**va) {
	int s, t, ierr, i;
	time_t t0, t1, dt;
	size_t ons = 0, s0, ps=0;
	FILE *f;
	pthread_t th;

	/* If there is a file in the commandline, read the sequence from it */
	if (na == 2) {
		f = fopen(va[1], "r");
	} else {
		f = stdin;
	}

	ierr = pthread_mutex_init(&m_ns, NULL);
	assert(ierr == 0);

	/* Flops calculation thread */
	ierr = pthread_create(&th, NULL, print_flops, NULL);
	assert(ierr == 0);
	
	t0 = ot = time(NULL);
	while(fscanf(f, "%d %d", &s, &t) != EOF) {
		if (t < 1) t = 1; /* set the minimum interval of time */
		s0 = (size_t)(s)*1024*1024;
		while(1) {
			ierr = pthread_mutex_lock(&m_ns); assert(ierr == 0);
			/* Compute the new allocated size (careful with
			   unsigned types and subtractions */
			t1 = time(NULL);
			dt = t1 - t0;
			if (dt > t) dt = t;
			ns = (s0*dt + ps*t - ps*dt)/t;

			/* Resize array */
			ptr = realloc(ptr, ns);
			assert(ptr != NULL || ns == 0);

			/* Initialize the new memory segment with float ones */
			for (i = ons/sizeof(float); i<ns/sizeof(float); i++)
				((float*)ptr)[i] = 1.f;
			ons = ns;

			/* Monitor */
			printlt();
			printf("MEM: %d MB\n", (int)(ns/1024/1024));
			ierr = pthread_mutex_unlock(&m_ns); assert(ierr == 0);
 
			/* Check if the operation is finished on time */
			if (dt >= t) break;

			sleep(1);
		}

		/* Store the current size */
		ps = s0;

		/* Acumulate time */
		t0+= t;
	}

	return 0;
	/* 
	ierr = pthread_mutex_destroy(&m_ns);
	assert(ierr == 0);
	if (ptr) { ierr = free(ptr); assert(ierr == 0); }
	*/
}

void * print_flops(void * noused) {
	int ierr, i;
	double f;
	struct timeval t0, t1;
	double dt;
	double ops; /* operation counter */

	while(1) {
		ierr = pthread_mutex_lock(&m_ns); assert(ierr == 0);
		if (ns == 0) {
			ierr = pthread_mutex_unlock(&m_ns); assert(ierr == 0);
			sleep(1);
			continue;
		}
		/* With this function we have microseconds of resolution (wow!) */
		ierr = gettimeofday(&t0, NULL); assert(ierr == 0);
		ops = 0.;
		f = 0.;
		while(1) {
			for (i=1; i<ns/sizeof(float); i+=2) f+= ((float*)ptr)[i-1]*((float*)ptr)[i];
			ops+= ns/sizeof(float);
			ierr = gettimeofday(&t1, NULL); assert(ierr == 0);
			dt = (double)(t1.tv_sec - t0.tv_sec) + ((double)t1.tv_usec - t0.tv_usec)/1e6;
			if (dt > 4.) break;
			f = 0.;
			ops+= 4.;
		}
		printlt();
		printf("CPU: %g MFLOPS %g\n", ops/1e6/dt, f);
		ierr = pthread_mutex_unlock(&m_ns); assert(ierr == 0);
		sleep(1);
	}
	return NULL;
}

