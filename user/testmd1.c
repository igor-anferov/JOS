// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    int * first_page  = (int *)0xe0000000;
    int * second_page = (int *)0xe0001000;
    int * third_page = (int *)0xe0002000;
    
	cprintf("test1 running...\n");
    
    if (sys_page_alloc(0, (void *) first_page, PTE_P | PTE_U | PTE_W))
        panic("sys_page_alloc fail!!!");
    if (sys_page_alloc(0, (void *) second_page, PTE_P | PTE_U | PTE_W))
        panic("sys_page_alloc fail!!!");
    
    for (int i=0; i<1024; i++) {
        first_page[i] = second_page[i] = i;
    }
    
    cprintf("test1: pages filled\n");
    
    sys_yield();
    // Test2 allocate and fill new page
    sys_yield();

    second_page[0] = 100000;
    cprintf("test1: page 2 changed\n");
    
    sys_yield();
    
    if (sys_page_alloc(0, (void *) third_page, PTE_P | PTE_U | PTE_W))
        panic("sys_page_alloc fail!!!");
    
    for (int i=0; i<1024; i++) {
        third_page[i] = i;
    }

    cprintf("test1: page 3 filled\n");
    
    while (1) {
        sys_yield();
    }
}
