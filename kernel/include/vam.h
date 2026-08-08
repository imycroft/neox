#ifndef VAM_H
#define VAM_H

#include "memory.h"
#include "types.h"

void vam_init(void);

void *vam_alloc_pages(uint32_t count);

void vam_free_pages(uintptr_t addr,
                    uint32_t count);


// Debug
void vam_dump(void);
#endif
