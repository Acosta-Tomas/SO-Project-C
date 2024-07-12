#include "main.h"


bool find_tlb(uint32_t pid, uint32_t pagina, uint32_t* frame){
    if (numero_entradas < 1) return false;

    bool page_finder(void* e) {
        t_tlb_entry* entry = (t_tlb_entry*) e;

        return entry->pid == pid && entry->page == pagina;
    };

    t_tlb_entry* hit = list_find(tlb_queue->elements, &page_finder);

    if (hit) {
        *(frame) = hit->frame;

        log_info(logger, "PID: %u - TLB HIT - Pagina: %u", pid, pagina);

        if (isLRU) {
            list_remove_element((tlb_queue->elements), hit);

            queue_push(tlb_queue, hit);
        }
        return true;
    }

    log_info(logger, "PID: %u - TLB MISS - Pagina: %u", pid, pagina);
    return false;
}

void add_tlb(uint32_t pid, uint32_t pagina, uint32_t frame){
    if (numero_entradas < 1) return;
    static int fill_counter = 0;

    if (fill_counter == numero_entradas){
        t_tlb_entry* entry = queue_pop(tlb_queue);

        entry->pid = pid;
        entry->page = pagina;
        entry->frame = frame;

        queue_push(tlb_queue, entry);  
    } else {
        t_tlb_entry* entry = malloc(sizeof(t_tlb_entry));

        entry->pid = pid;
        entry->page = pagina;
        entry->frame = frame;

        fill_counter += 1;
        queue_push(tlb_queue, entry);
    }
}