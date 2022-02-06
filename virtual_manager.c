#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define VMEMORY_SIZE 256
#define FRAME_SIZE 256
#define PMEMORY_SIZE 128 * FRAME_SIZE
#define PAGE_SIZE 256
#define TLB_SIZE 16 
//A TLB contém 128 bytes para armazenar 16 tuplas de inteiros
// onde cada tupla tem dois inteiros de 4 bytes, logo o tamanho em bytes da tlb é:
// 16(numero de tuplas) x 2(inteiros por tupla) x 4(bytes por inteiro) = 128. 

typedef struct queue_elem
{
    int value;
    struct queue_elem *next_elem;
}elem;

struct fifo_queue
{
    int len;
    elem *first_elem;
}fifo;

struct queue_elem *init_queue_elemn(int value){
    struct queue_elem *elem = (struct queue_elem *)malloc(sizeof(struct queue_elem));
    elem->value = value;
    elem->next_elem = NULL;

    return elem;
}

struct fifo_queue *init_queue(){
    struct fifo_queue *fifo = (struct fifo_queue *)malloc(sizeof(struct fifo_queue));
    fifo->len = 0;
    return fifo;
    }

void push_queue (struct fifo_queue *queue, int value){
    struct queue_elem *elem = init_queue_elemn(value);
    if (queue->len == 0)
    {
        queue->first_elem = elem;
        queue->len++;
    }else{
        printf("cuzin2\n");

        struct queue_elem *aux = queue->first_elem;
        queue->first_elem = elem;
        queue->len++;
        elem->next_elem = aux;
    }
    
}

int get_elem_queue(struct fifo_queue *queue){
    struct queue_elem *aux = queue->first_elem;
    struct queue_elem *aux_ant = queue->first_elem;
    while (aux->next_elem != NULL)
    {
        aux_ant = aux;
        aux = aux->next_elem;
    }
    int page_number = aux->value;
    free(aux);
    aux_ant->next_elem = NULL;
    return page_number;
}

// utilizar contador para instante de acesso
int LRU(int counter[], int min){
    int index = 0;
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        if (counter[i] < min && counter[i] != 0 )
        {
            min = counter[i];
            index = i;
        }
    }
    
    return index;
}

int main(int argc, char* argv[]){

    if(argc<2)
	{
		printf("Missing arguments\n");
		return 0;
	}

    

    char* addresses_filename = argv[1];
    FILE* addresses = fopen(argv[1],"r");
	if(addresses==NULL)
	{
		printf("File address.txt could not be opened\n");
		return 0;
	}

    FILE* storage = fopen("teste.dat", "rb"); // Open for reading only
        if ( storage == NULL) {
            // error and exit
            printf("teste.bat file could not be opened.\n");
            return 0;
        }

    
	
    struct fifo_queue *queue = init_queue();


    int TLB[TLB_SIZE][2];
	int pagetable[PAGE_SIZE];
    char physical_memory[PMEMORY_SIZE];
    char temporary[PAGE_SIZE];
    int choice;
    printf("Digite [1] para utilizar LRU \nDigite [2] para utilizar FIFO\n ");
    scanf("%d", &choice);
    if (choice != 1 && choice != 2)
    {
        printf("Escolha inválida.\n");
        return 0;
    }
    

    int exists_on_pagetable[PAGE_SIZE]= {};
    int page_table_fifo[128] = {};
    

    ssize_t read;
    char *value=NULL;
    size_t len=0;
    int mask=255, n_addresses, tlb_hits, frame, faults;
    int virtual_address, physical_address, page_number, frame_number, offset;
    int actual_memory_index = 0;


    n_addresses = 0;
    faults = 0;
    int i;
    int TLB_index = 0;
    bool tlb_hit, fault, memory_is_full = false;
    unsigned char byte_value;
    int actual_add = 0;
    int fifo_counter = 0;
    int page_replacements = 0;
    while((read=getline(&value,&len,addresses))!=-1){
        virtual_address=atoi(value);
        n_addresses++;
        tlb_hit = false;
        actual_add++;

        // obtendo o numero da página
        page_number=atoi(value);
        page_number=page_number>>8;
		page_number=page_number & mask;

        // obtendo o offset 
        offset=atoi(value);
		offset=offset & mask;
		// printf("%lld %lld\n\n",page_no,offset);


        // procurar na TLB

        for (i = 0; i < TLB_SIZE; i++)
        {
            if (page_number == TLB[i][0])
            {
                tlb_hit = true;
                tlb_hits++;
                frame = TLB[i][1];
                printf("TLB HIT\n");
                break;
            }            
        }
        if (tlb_hit)
        {
            physical_address = page_number + offset;
            byte_value = physical_memory[physical_address];
        }else{//nao encontrou na TLB, portanto TLB miss, procura na pagetable
            // se o pagenumber já existe na pagetable
            if (exists_on_pagetable[page_number] != 0)
            {
                frame = pagetable[page_number];
                exists_on_pagetable[page_number] = actual_add;

                physical_address = frame + offset; //calculando o endereço fisico
                byte_value = physical_memory[physical_address]; //obtendo o valor armazenado no endereço fisico

                //atualizando a TLB
                TLB[TLB_index][0] = page_number;
                TLB[TLB_index][1] = frame;
                TLB_index++;
			    TLB_index=TLB_index%15;	
            }else{
                faults++;

                if (!memory_is_full)
                {
                    // o endereço nao se encontra na page table mas a memoria fisica não está cheia ainda
                    // ler do BACKING_STORE para armazenar
                    fseek(storage, page_number * PAGE_SIZE, SEEK_SET);
                    fread(temporary, 1, PAGE_SIZE, storage);
                    memcpy(physical_memory + actual_memory_index, temporary, PAGE_SIZE);

                    frame = actual_memory_index;
                    physical_address = frame + offset;
                    byte_value = physical_memory[physical_address];

                    // atualizando a pagetable
                    pagetable[page_number] = frame;
                    exists_on_pagetable[page_number] = actual_add;
                    // adiciona na fila da pagetable
                    push_queue(queue, page_number);

                    // atualizando a TLB
                    TLB[TLB_index][0] = page_number;
                    TLB[TLB_index][1] = frame;
                    TLB_index++;
			        TLB_index=TLB_index%15;	


                    if (actual_add < PMEMORY_SIZE - FRAME_SIZE)
                    {
                        actual_add += FRAME_SIZE;
                    }else{
                        memory_is_full = true;
                    }
                    
                }else{
                    page_replacements++;
                    int lru_replace_index;
                    int frame_to_replace;
                    if(choice == 1){
                        lru_replace_index = LRU(exists_on_pagetable, actual_add);
                        frame_to_replace = pagetable[lru_replace_index];
                    }else if (choice == 2)
                    {
                        lru_replace_index = get_elem_queue(queue);
                        frame_to_replace = pagetable[lru_replace_index];
                    }
                    

                    fseek(storage, page_number*PAGE_SIZE, SEEK_SET);
                    fread(temporary, 1, PAGE_SIZE, storage);
                    memcpy(physical_memory + frame_to_replace, temporary, PAGE_SIZE);

                    physical_address = frame_to_replace + offset;
                    byte_value = physical_memory[physical_address];

                    // atualizando pagetable
                    pagetable[page_number] = frame_to_replace;
                    exists_on_pagetable[page_number] = actual_add;
                    // adiciona na fila da pagetable
                    push_queue(queue, page_number);

                    // atualizando a TLB
                    TLB[TLB_index][0] = page_number;
                    TLB[TLB_index][1] = frame;
                    TLB_index++;
			        TLB_index=TLB_index%15;	
                }

                
            }
        }
        

        if(virtual_address<10000)
		printf("VIRTUAL ADDRESS = %d \t\t\t",virtual_address);
		else
		printf("VIRTUAL ADDRESS = %d \t\t",virtual_address);
    
        // multiplica por PAGE_SIZE para fazer o shift
		printf("physical_adr=%d   value = %d\n",physical_address, byte_value);
        fifo_counter++;
    }

    float page_fault_rate = (float) faults / (float) n_addresses;
    float TLB_rate = (float) tlb_hit / (float) n_addresses;


    printf("Endereços verificados = %d\n", n_addresses);
    printf("TLB hits = %d\n", tlb_hits);
    printf("Page faults: %d\n", faults);
    printf("TLB Hit Rate : %.4f%%\n", TLB_rate*100);
    printf("Page Fault Rate : %.4f%%\n", page_fault_rate*100);

    printf("Page-replacement: %d\n", page_replacements);
}
