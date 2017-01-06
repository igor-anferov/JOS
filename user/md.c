// MEMORY DEDUPLICATION UNIT
#include <inc/lib.h>

#define HASH_TABLE_SIZE 100
#define MAX_NODES       1000000
#define PERIOD          10       // sec

extern void pgfault(struct UTrapframe *utf);

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

void pages_are_equal(void *add_pg, void *comp_pg) {
    char *first = (char *)add_pg;
    char *second = (char *)comp_pg;
    int i;
    
    for (i=0; i<PGSIZE; i++)
        if (first[i] != second[i])
            return 0;
    
    return 1;
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
    if (empty_nodes = NULL) {
        cprintf("ERROR!!! Empty nodes for memory deduplication hash table are over.\n");
        exit(0);
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
    static const void *add_pg = (void *)0xe0002000;
    static const void *comp_pg = (void *)0xe0003000;
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
    static const pde_t *pde = (pde_t *) 0xe0000000;
    static const pte_t *pte = (pte_t *) 0xe0001000;
    
    int old_time, new_time;
    
    while (1) {
        
        old_time = new_time;
        new_time = vsys_gettime();
        
        if (new_time - old_time < PERIOD) {
            sys_yield();
        }
        
        cprintf("------ MEMORY DEDUPLICATION UNIT ------\n");
        
        init();
        
        for (i=0; i<NENV; i++) {
            if (envs[i].env_status != ENV_FREE) {
                
                if (sys_page_map(envs[i].env_id, (void *)uvpd,
                                 0, (void *)pde, PTE_P | PTE_U)) {
                    cprintf("sys_page_map ERROR!!! (1)\n");
                    exit();
                }
                
                for (j=0; j<=PDX(USTACKTOP); j++) {
                    if (pde[j] & PTE_P) {
                        if (sys_page_map(envs[i].env_id,
                                         (void *)( &uvpt[ PGNUM( PGADDR(j, 0, 0) ) ] ),
                                         0, (void *)pte, PTE_P | PTE_U)) {
                            cprintf("sys_page_map ERROR!!! (2)\n");
                            exit();
                        }
                        for (k=0; k<NPTENTRIES; k++) {
                            if (pte[k] & PTE_P & ~PTE_SHARE) {
                                
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
        
        cprintf("Deduplicated %d page(s)\n", deduplicated_pages);
        cprintf("\n------ MEMORY DEDUPLICATION UNIT ENDS ------\n\n");
    }
}
