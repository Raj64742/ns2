#include <stdio.h>

#define fDIFF(FIELD) (now_.FIELD - start_.FIELD)
#define normalize(var) (var > 0 ? var : -var)

struct MemInfo {
  long stack;
  long heap;
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
	}
	void diff(char* prompt) {
		(void) cur_stack(&now_);
		(void) cur_heap(&now_);
		fprintf (stdout, "%s: stack/heap:%d/%d\n", prompt, normalize(fDIFF(stack)), normalize(fDIFF(heap)));
		(void) cur_stack(&start_);
		(void) cur_heap(&start_);
	}
private:
	struct MemInfo start_, now_;
};

