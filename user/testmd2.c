// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    int * first_page  = (int *)0xe0003000;
    
    sys_yield();
    
	cprintf("test2 running...\n");
    
    if (sys_page_alloc(0, (void *) first_page, PTE_P | PTE_U | PTE_W))
        panic("sys_page_alloc fail!!!");
    
    for (int i=0; i<1024; i++) {
        first_page[i] = i;
    }
    
    cprintf("test2: page filled\n");
    
    while (1) {
        sys_yield();
    }
}
