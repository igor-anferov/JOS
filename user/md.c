// MEMORY DEDUPLICATION UNIT

#include <inc/lib.h>

#define HASH_TABLE_SIZE 10
#define MAX_NODES       100
#define PERIOD          10       // sec

#define PTE_COW		0x800

//extern void pgfault(struct UTrapframe *utf);
extern void _pgfault_upcall();

struct Stab * STAB_BEGIN;
struct Stab * STAB_END;
char * STABSTR_BEGIN;
char * STABSTR_END;

unsigned int page_hash(void *addr) {
    
    static const unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    unsigned char *str = (unsigned char *)addr;
    int i;
    
    for(i=0; i<PGSIZE; i++, str++) {
        hash = hash * a + *str;
        a *= b;
    }
    
    return hash % HASH_TABLE_SIZE;
}

int pages_are_equal(void *add_pg, void *comp_pg) {
    char *first = (char *)add_pg;
    char *second = (char *)comp_pg;
    int i;
    
    for (i=0; i<PGSIZE; i++)
        if (first[i] != second[i])
            return 0;
    
    return 1;
}

void * find_function(const char * const fname) {
    struct Stab *stabs = STAB_BEGIN, *stab_end = STAB_END;
    char *stabstr = STABSTR_BEGIN, *stabstr_end = STABSTR_END;
    
    char *cur = (char *)stabstr;
    
    for (; cur < stabstr_end - strlen(fname); cur++) {
        if ( strncmp(cur, fname, strlen(fname)) == 0 && cur[strlen(fname)] == ':' && cur[-1] == '\0' ) {
            for (; stabs<stab_end; stabs++) {
                if (stabs->n_type == N_FUN &&
                    stabs->n_strx == cur-stabstr) {
                    return (void *)(stabs->n_value);
                }
            }
        }
    }
    
    return NULL;
}

void set_pgfault_upcall(int envid) {
    static void *stabs = (void *)0xe0200000;
    int i, j;
    void *res;
    
    for (i=0; ; i++) {
        if (sys_page_map(envid, (void *)(USTABDATA+i*PGSIZE),
                         0, (void *)(stabs+i*PGSIZE),
                         PTE_U | PTE_P )) {
            break;
        }
    }

    STAB_BEGIN = (struct Stab *)(0xe0000000+((long *)stabs)[0]);
    STAB_END = (struct Stab *)(0xe0000000+((long *)stabs)[1]);
    STABSTR_BEGIN = (char *)(0xe0000000+((long *)stabs)[2]);
    STABSTR_END = (char *)(0xe0000000+((long *)stabs)[3]);
    
    if ((res = find_function("set_pgfault_handler")) == NULL) {
        cprintf("ERROR!!! _pgfault_upcall not found!\n");
        exit();
    }
    
    sys_env_set_pgfault_upcall(envid, res+0x6F);
    
    for (j=0; j<i; j++) {
        sys_page_unmap(0, (void *)(stabs+j*PGSIZE));
    }
}

typedef struct node {
    int env;
    void *pg;
    int pte;
    struct node *next;
} node;

node *hash_table[HASH_TABLE_SIZE];
node hash_table_data[MAX_NODES];
node *empty_nodes;
int deduplicated_pages;

void init(void) {
    unsigned int i;
    memset(hash_table_data, 0, sizeof(hash_table_data));
    memset(hash_table, 0, sizeof(hash_table));
    empty_nodes = hash_table_data;
    for (i=0; i<MAX_NODES-1; i++) {
        hash_table_data[i].next = &hash_table_data[i+1];
    }
    deduplicated_pages = 0;
}

node *new_node(void) {
    if (empty_nodes == NULL) {
        cprintf("ERROR!!! Empty nodes for memory deduplication hash table are over.\n");
        exit();
    }
    node *ret = empty_nodes;
    empty_nodes = ret->next;
    ret->next = NULL;
    return ret;
}

void free_node(node *ret) {
    ret->env = 0;
    ret->pg = NULL;
    ret->next = empty_nodes;
    empty_nodes = ret;
}

void add_to_hash(node *n) {
    static void *add_pg = (void *)0xe0002000;
    static void *comp_pg = (void *)0xe0003000;
    unsigned int hash;
    node *line;
    
    if (sys_page_map(envs[n->env].env_id, n->pg,
                     0, (void *)add_pg, PTE_P | PTE_U)) {
        cprintf("sys_page_map ERROR!!! (3)\n");
        exit();
    }
    
    hash = page_hash(add_pg);
    line = hash_table[hash];
    
    for (; line; line = line->next) {
        
        if (sys_page_map(envs[line->env].env_id, line->pg,
                         0, (void *)comp_pg, PTE_P | PTE_U)) {
            cprintf("sys_page_map ERROR!!! (4)\n");
            exit();
        }
        
        if (!pages_are_equal(add_pg, comp_pg)) {
            continue;
        }
        
        if ( !(line->pte & PTE_COW) && (line->pte & PTE_W) ) {
            line->pte &= ~PTE_W;
            line->pte |= PTE_COW;
            
            if (!envs[line->env].env_pgfault_upcall) {
                set_pgfault_upcall(envs[line->env].env_id);
            }
            
            if (sys_page_map(envs[line->env].env_id, line->pg,
                             envs[line->env].env_id, line->pg, PTE_COW|PTE_U|PTE_P)) {
                cprintf("sys_page_map ERROR!!! (5)\n");
                exit();
            }
            
            cprintf("[%08x]:0x%05x >-<\n",
                    (unsigned int)envs[line->env].env_id, (unsigned int)PGNUM(line->pg));
        }
        
        if (!envs[n->env].env_pgfault_upcall) {
            set_pgfault_upcall(envs[n->env].env_id);
        }
        
        if (sys_page_map(envs[line->env].env_id, line->pg,
                         envs[n->env].env_id, n->pg,
                         PTE_U | PTE_P | (n->pte & PTE_W ? PTE_COW : 0) )) {
            cprintf("sys_page_map ERROR!!! (6)\n");
            exit();
        }
        
        cprintf("[%08x]:0x%05x --> [%08x]:0x%05x\n",
                (unsigned int)envs[n->env].env_id, (unsigned int)PGNUM(n->pg),
                (unsigned int)envs[line->env].env_id, (unsigned int)PGNUM(line->pg));

        deduplicated_pages++;
        free_node(n);
        
        sys_page_unmap(0, add_pg);
        sys_page_unmap(0, comp_pg);
        return;
    }
    
    n->next = hash_table[hash];
    hash_table[hash] = n;
    
    sys_page_unmap(0, add_pg);
    sys_page_unmap(0, comp_pg);
}

void
umain(int argc, char **argv) {
    
    int i, j, k;
    static pde_t *pde = (pde_t *) 0xe0000000;
    static pte_t *pte = (pte_t *) 0xe0001000;
    
    static int old_time, new_time = 0;
    
    while (1) {
        
        old_time = new_time;
        new_time = vsys_gettime();
        
        if (old_time && new_time - old_time < PERIOD) {
            sys_yield();
        }
        
        cprintf("\n------ MEMORY DEDUPLICATION UNIT ------\n\n");
        
        init();
        
        for (i=0; i<NENV; i++) {
            if (envs[i].env_status != ENV_FREE) {
                
                if (sys_page_map(envs[i].env_id, (void *)uvpd,
                                 0, (void *)pde, PTE_P | PTE_U)) {
                    cprintf("sys_page_map ERROR!!! (1)\n");
                    exit();
                }
                
                for (j=0; j<PDX(USTACKTOP); j++) {
                    if (pde[j] & PTE_P) {
                        if (sys_page_map(envs[i].env_id,
                                         (void *)( &uvpt[ PGNUM( PGADDR(j, 0, 0) ) ] ),
                                         0, (void *)pte, PTE_P | PTE_U)) {
                            cprintf("sys_page_map ERROR!!! (2)\n");
                            exit();
                        }
                        for (k=0; k<NPTENTRIES; k++) {
                            if ( (pte[k] & PTE_P) && !(pte[k] & PTE_SHARE) ) {
                                
                                // Adding page to hashtable
                                node *n = new_node();
                                
                                n->env = i;
                                n->pg = PGADDR(j, k, 0);
                                n->pte = pte[k];
                                
                                add_to_hash(n);
                            }
                        }
                    }
                }
            }
        }
        
        cprintf("\nDeduplicated %d page(s)\n", deduplicated_pages);
        cprintf("\n------ MEMORY DEDUPLICATION UNIT ENDS ------\n\n");
        exit();
    }
}
