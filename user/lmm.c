// list memory map
#include <inc/lib.h>

//#define PADDR(kva) (void *)((physaddr_t)(kva) - KERNBASE)

void
umain(int argc, char **argv)
{
    int i, j;
    pte_t *pte = (pte_t *)0xee001000;
    
    cprintf("\n------ MEMORY MAP: ------\n");
    
    for (i=0; i<NENV; i++) {
        if (envs[i].env_status != ENV_FREE) {
            
            cprintf("\n+ Environment %08x:\n", envs[i].env_id);
            for (j=0; j<UTOP; j+=PGSIZE) {
                    if (sys_page_map(envs[i].env_id, (void *)j,
                                     0, pte, PTE_P | PTE_U)) {
                        continue;
                    }
                    cprintf("va: %05x*** --> pa: %05x***\n",
                            (unsigned int)PGNUM(j),
                            (unsigned int)PGNUM(sys_page_pa(envs[i].env_id, (void *)j)));
            }
            
        }
    }
    
    cprintf("\n--- END OF MEMORY MAP ---\n");
}
