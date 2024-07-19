#include "main.h"

/*
    Armo lista de frames, con cantidad de bytes a leer en cada frame por orden de pagina digamos.
    Es recursiva, disminuyendo el size de lo que hay leer a medida que avanzo de pagina.
*/

pid_status mmu(int memoria_fd, uint32_t direccion_logica, uint32_t size, t_list* frames){
    uint32_t pagina = floor(direccion_logica / page_size);
    uint32_t desplazamiento = direccion_logica - pagina * page_size;
    t_memoria_fisica* frame = calloc(1, sizeof(t_memoria_fisica));
    pid_status status = RUNNING;
    uint32_t marco;

    if (desplazamiento + size <= page_size){
        frame->bytes = size;
        size = 0;
    } else {
        frame->bytes = page_size - desplazamiento;
        size = size - frame->bytes;
    }

    if(!find_tlb(pcb->pid, pagina, &marco)){
      t_paquete* paquete = crear_paquete(MEM_PID_PAGE);
        
        agregar_uint_a_paquete(paquete, &pcb->pid, sizeof(uint32_t));
        agregar_uint_a_paquete(paquete, &pagina, sizeof(uint32_t));

        enviar_paquete(paquete, memoria_fd);
        eliminar_paquete(paquete);

        op_code code_op;
        uint32_t size_discard;

        recv(memoria_fd, &code_op, sizeof(op_code), MSG_WAITALL);
        
        if (code_op == MEM_ERROR) return ERROR;

        recv(memoria_fd, &size_discard, sizeof(uint32_t), MSG_WAITALL);
        recv(memoria_fd, &marco, sizeof(uint32_t), MSG_WAITALL);

        add_tlb(pcb->pid, pagina, marco);
    }

    frame->direccion_fisica = marco * page_size + desplazamiento;
    list_add(frames, frame);

    if (size > 0) status = mmu(memoria_fd, (pagina + 1) * page_size, size, frames);     

    return status;
}

bool find_tlb(uint32_t pid, uint32_t pagina, uint32_t* frame){
    if (numero_entradas < 1) return false;

    bool page_finder(void* e) {
        t_tlb_entry* entry = (t_tlb_entry*) e;

        return entry->pid == pid && entry->page == pagina && entry->valid;
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
        entry->valid = true;

        queue_push(tlb_queue, entry);  
    } else {
        t_tlb_entry* entry = malloc(sizeof(t_tlb_entry));

        entry->pid = pid;
        entry->page = pagina;
        entry->frame = frame;
        entry->valid = true;

        fill_counter += 1;
        queue_push(tlb_queue, entry);
    }
}

void modify_tlb_on_resize(uint32_t new_size, uint32_t pid){
    int max_page = ceil((double)new_size / page_size);
    
    void entry_to_modify(void* e) {
        t_tlb_entry* entry = (t_tlb_entry*) e;

        if (entry->pid == pid && ((int) entry->page > (max_page - 1))){
            entry->valid = false;
        }
    };

    list_iterate(tlb_queue->elements, &entry_to_modify);
}
