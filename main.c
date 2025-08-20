#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define byte unsigned char  // 0x00
#define word unsigned short // 0x0000

/*
    *** MEMORY LAYOUT ***
*/
#define MAX_RAM         1024 * 64

#define ZERO_PAGE_BEGIN  0x0000
#define ZERO_PAGE_END   0x00FF

#define STACK_BEGIN     0x00
#define STACK_END       0xFF
#define STACK_INIT      0xFD

#define ISR_BEGIN       0xFFFA
#define ISR_END         0xFFFF

#define RESET_LOW       0xFFFC
#define RESET_HIGH      0xFFFD

#define NMI_ISR_LOW     ISR_BEGIN
#define NMI_ISR_HIGH    0xFFFB

#define IRQ_ISR_LOW     0xFFFE
#define IRQ_ISR_HIGH    ISR_END

/*
    *** USER SPACE ***
*/
#define USR_REGION_BEGIN 0x00;
#define USR_REGION_END  0x02;


/*
    *** INSTRUCTIONS ***
*/
//  *** LDA                         CYCLES
#define LDA_IMMIDIATE       0xA9    // 2
#define LDA_ZERO_PAGE       0xA5    // 3
#define LDA_ZERO_PAGE_X     0xB5    // 4
#define LDA_ABSOLUTE        0xBD    // 4

typedef struct {
    byte data[MAX_RAM];
}ram;

typedef struct{
    byte carry_flag     : 1;
    byte zero_flag      : 1;
    byte interrupt_flag : 1;
    byte decimal_flag   : 1;
    byte break_flag     : 1;
    byte overflow_flag  : 1;
    byte negative_flag   : 1;
}processor_status;

typedef struct {
    word program_counter;
    byte stack_ptr;
    byte a;
    byte ir_x;
    byte ir_y;
    processor_status ps;
}cpu;

/*
    *** FORWARD DECLARATION ***
*/
word ram_read_word(ram* ram, word location);

ram* ram_routine(){
    ram* r = malloc(sizeof(ram));
    memset(r->data, 0, sizeof(r->data));
    return r;
}

cpu* cpu_routine(ram* r){
    cpu* c = malloc(sizeof(cpu));
    c->stack_ptr = STACK_INIT;
    c->a = 0;
    c->ir_x = 0;
    c->ir_y = 0;
    c->program_counter = ram_read_word(r, RESET_LOW);
    return c;
}

size_t is_bit_set(byte n, size_t bit){
    if (bit > 7) return 0;
    return (n >> bit) & 1;
}

// 1 cpu cycle
byte ram_read_byte(ram* ram, word location){ 
    return ram->data[location];
}

// 3 cpu cycles
word ram_read_word(ram* ram, word location){ 
    return ram->data[location] | (ram->data[location + 1] << 8);
}

void cpu_execute(cpu* c, ram* r, int cycles){
    while (cycles){
        byte op_code = ram_read_byte(r, c->program_counter++);
        cycles--;

        switch (op_code)
        {
        case LDA_IMMIDIATE:
        {
            c->a = ram_read_byte(r, c->program_counter++);
            cycles--;
        }break;
        case LDA_ZERO_PAGE:
        {
            byte addr = ram_read_byte(r, c->program_counter++);
            cycles--;
            c->a = ram_read_byte(r, addr);
            cycles--;
        }break;
        case LDA_ZERO_PAGE_X:
        {
            //abs location
            byte addr = ram_read_byte(r, c->program_counter++);
            cycles--;
            addr += c->ir_x;
            cycles--;
            c->a = ram_read_byte(r, addr);
            cycles--;
        }break;
        case LDA_ABSOLUTE:
        {
            //abs location
            word addr = ram_read_word(r, c->program_counter++);
            c->a = ram_read_byte(r, addr);
            c->program_counter++;
            cycles--;
            cycles--;
            cycles--;
        }break;
        default:
            printf("Instruction 0x%x is not implemented yet. \n", op_code);
            exit(1);
            break;
        }
    }
    c->ps.zero_flag = c->a == 0 ? 0 : 1;
    c->ps.negative_flag = is_bit_set(c->a, 7) ? 1 : 0;
}

int main(void){
    ram* r = ram_routine();
    // ... "parsing" asm
    // User program entry location
    r->data[RESET_LOW]  = USR_REGION_BEGIN;
    r->data[RESET_HIGH] = USR_REGION_END;
    
    cpu* c = cpu_routine(r);
    printf("Proram counter initialized at: 0x%04x, intialization complete at [%s]\n",
        c->program_counter, c->program_counter == 0x0200 ? "UserSpace" : "Undefined");

    r->data[0x0200] = LDA_ZERO_PAGE;
    r->data[0x0201] = 0x00;
    r->data[0x00] =0x45;  

    cpu_execute(c,r,3);
    printf("0x%x\n",c->a);

    exit (0);
}