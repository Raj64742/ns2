#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

extern "C" {
        int getrusage(int who, struct rusage* rusage);
} 
	
#define fDIFF(FIELD) (now_.FIELD - start_.FIELD)
#define absolute(var) (var > 0 ? var : -var)
#define normalize(var) var.tv_sec * 1000 + (int) (var.tv_usec / 1000)

struct MemInfo {
  long stack;
  long heap;
  struct rusage time;
};

void *cur_stack(struct MemInfo *meminfo)
{
        int i;
	meminfo->stack = (long)&i;
}

void *cur_heap(struct MemInfo *meminfo)
{
	meminfo->heap = sbrk(0);
}

class MemTrace {
public:
	MemTrace() {
		(void) cur_stack(&start_);
		(void) cur_heap(&start_);
                (void) getrusage(RUSAGE_SELF, &(start_.time));  
	}
	void diff(char* prompt) {
		(void) cur_stack(&now_);
		(void) cur_heap(&now_);
                (void) getrusage(RUSAGE_SELF, &(now_.time));  
	fprintf (stdout, "%s: utime/stime: %d %d \tstack/heap: %d %d\n",\
		prompt, \
	normalize(now_.time.ru_utime) - normalize(start_.time.ru_utime), \
	normalize(now_.time.ru_stime) - normalize(start_.time.ru_stime), \
		fDIFF(stack), fDIFF(heap));
		(void) cur_stack(&start_);
		(void) cur_heap(&start_);
                (void) getrusage(RUSAGE_SELF, &(start_.time));  
	}
private:
	struct MemInfo start_, now_;
};

