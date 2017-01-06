// list memory map
#include <inc/lib.h>

#define UVPD (UVPT+(UVPT>>12)*4)

void
umain(int argc, char **argv)
{
    int i, j, k;
    pde_t *pde = (pde_t *)0xe0000000;
    pte_t *pte = (pte_t *)0xe0001000;
    
    cprintf("\n------------------ MEMORY MAP: ------------------\n");
    
    for (i=0; i<NENV; i++) {
        if (envs[i].env_status != ENV_FREE) {
            
            cprintf("\n+ Environment %08x:\n", envs[i].env_id);
            
            if (sys_page_map(envs[i].env_id, (void *)UVPD,
                             0, (void *)pde, PTE_P | PTE_U)) {
                cprintf("sys_page_map ERROR!!! (1)\n");
                exit();
            }
            
            for (j=0; j<PDX(UTOP); j++) {
                if (pde[j] & PTE_P) {
                    cprintf("    PDX: %04d\n", (unsigned int)j);
                    if (sys_page_map(envs[i].env_id,
                                     (void *)( UVPT + sizeof(pte_t)*PGNUM( PGADDR(j, 0, 0) ) ),
                                     0, (void *)pte, PTE_P | PTE_U)) {
                        cprintf("sys_page_map ERROR!!! (2)\n");
                        exit();
                    }
                    for (k=0; k<NPTENTRIES; k++) {
                        if (pte[k] & PTE_P) {
                            cprintf("        PTX: %04d | va: %05x*** --> pa: %05x***\n",
                                    k,
                                    (unsigned int)(PGNUM(PGADDR(j, k, 0))),
                                    (unsigned int)PGNUM(PTE_ADDR(pte[k])));
                        }
                    }
                }
            }
            
        }
    }
    
    cprintf("\n--------------- END OF MEMORY MAP ---------------\n\n");
}
