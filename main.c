#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte ;  // 0x00
typedef unsigned short word ; // 0x0000

/*
    *** MEMORY LAYOUT ***
*/
#define MAX_RAM          1024 * 64

#define ZERO_PAGE_BEGIN  0x0000
#define ZERO_PAGE_END    0x00FF

#define STACK_BEGIN      0x00
#define STACK_END        0xFF
#define STACK_INIT       0xFD

#define ISR_BEGIN        0xFFFA
#define ISR_END          0xFFFF

#define RESET_LOW        0xFFFC
#define RESET_HIGH       0xFFFD

#define NMI_ISR_LOW      ISR_BEGIN
#define NMI_ISR_HIGH     0xFFFB

#define IRQ_ISR_LOW      0xFFFE
#define IRQ_ISR_HIGH     ISR_END

/*
    *** USER SPACE ***
*/
#define USR_REGION_BEGIN 0x00
#define USR_REGION_END   0x02

/*
    *** INSTRUCTIONS ***
*/
//  *** LDA                         CYCLES
#define LDA_IMMIDIATE       0xA9    // 2
#define LDA_ZERO_PAGE       0xA5    // 3
#define LDA_ZERO_PAGE_X     0xB5    // 4
#define LDA_ABSOLUTE        0xAD    // 4
#define LDA_ABSOLUTE_X      0xBD    // 4 (+1 crossed)
#define LDA_ABSOLUTE_Y      0xB9    // 4 (+1 crossed)
#define LDA_INDERECT_X      0xA1    // 6
#define LDA_INDERECT_Y      0xB1    // 5 (+1 crossed)

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
    byte negative_flag  : 1;
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
    memset(&c->ps, 0, sizeof(c->ps));
    return c;
}

size_t is_bit_set(byte n, size_t bit){
    if (bit > 7) return 0;
    int a = (n >> bit) & 1;
    return a;
}

// 1 cpu cycles
byte ram_read_byte(ram* ram, word location){ 
    return ram->data[location];
}

// 3 cpu cycles
word ram_read_word(ram* ram, word location){ 
    return ram->data[location] | (ram->data[location + 1] << 8);
}

// 3 cpu cycles
word ram_read_word_wrap(ram* ram, word location){
    return ram->data[location] | (ram->data[(location + 1) & 0xFF] << 8);
}

size_t cpu_process_LDA(cpu* c, ram* r, byte op_code){
    size_t cycles_used = 0;
    switch (op_code)
    {
    case LDA_IMMIDIATE:
    {
        c->a = ram_read_byte(r, c->program_counter++);
        cycles_used++;
    }break;
    case LDA_ZERO_PAGE:
    {
        byte zp_addr = ram_read_byte(r, c->program_counter++);
        cycles_used++;
        c->a = ram_read_byte(r, zp_addr);
        cycles_used++;
    }break;
    case LDA_ZERO_PAGE_X:
    {
        //abs location
        byte addr = ram_read_byte(r, c->program_counter++);
        cycles_used++;
        addr += c->ir_x;
        cycles_used++;
        c->a = ram_read_byte(r, addr);
        cycles_used++;
    }break;
    case LDA_ABSOLUTE:
    {
        word addr = ram_read_word(r, c->program_counter++);
        cycles_used++;
        cycles_used++;
        c->a = ram_read_byte(r, addr);
        cycles_used++;
        c->program_counter++;
    }break;
    case LDA_ABSOLUTE_X:
    {
        word addr = ram_read_word(r, c->program_counter++);
        cycles_used++;
        cycles_used++;

        //cross
        if ((addr & 0xFF) + c->ir_x >= 0x100)
            cycles_used++;

        addr += c->ir_x;
        c->a = ram_read_byte(r, addr);
        cycles_used++;
        c->program_counter++;
    }break;
    case LDA_ABSOLUTE_Y:
    {
        word addr = ram_read_word(r, c->program_counter++);
        cycles_used++;
        cycles_used++;

        //cross
        if ((addr & 0xFF) + c->ir_y >= 0x100)
            cycles_used++;

        addr += c->ir_y;
        c->a = ram_read_byte(r, addr);
        cycles_used++;
        c->program_counter++;
    }break;
    case LDA_INDERECT_X:
    {
        byte zp_addr = ram_read_byte(r, c->program_counter++);
        cycles_used++;
        zp_addr += c->ir_x;
        cycles_used++;
        word addr = ram_read_word_wrap(r, zp_addr);
        cycles_used++;
        cycles_used++;
        c->a = ram_read_byte(r, addr);        
        cycles_used++;
    }break;
    case LDA_INDERECT_Y:
    {
        byte zp_addr = ram_read_byte(r, c->program_counter++);
        cycles_used++;
        word addr = ram_read_word_wrap(r, zp_addr);
        cycles_used++;
        cycles_used++;

        //cross
        if ((addr & 0xFF) + c->ir_y >= 0x100)
            cycles_used++;
        
        addr += c->ir_y;
        c->a = ram_read_byte(r, addr);        
        cycles_used++;
    }break;
    default:
        printf("Instruction 0x%x is not an LDA instruction. \n", op_code);
        break;
    }
    return cycles_used;
}

size_t cpu_execute(cpu* c, ram* r) {
    size_t cycles_used = 0;
    byte op_code = ram_read_byte(r, c->program_counter++);
    cycles_used++;

    switch(op_code)
    {
    case LDA_IMMIDIATE:
    case LDA_ZERO_PAGE:
    case LDA_ZERO_PAGE_X:
    case LDA_ABSOLUTE:
    case LDA_ABSOLUTE_X:
    case LDA_ABSOLUTE_Y:
    case LDA_INDERECT_X:
    case LDA_INDERECT_Y:
        cycles_used += cpu_process_LDA(c,r,op_code); break;
    default:
        printf("Instruction 0x%x is not implemented yet or undefined. \n", op_code);
        exit(1);
        break;
    }

    c->ps.zero_flag = c->a == 0 ? 1 : 0;
    c->ps.negative_flag = is_bit_set(c->a, 7) ? 1 : 0;
    return cycles_used;
}

void cpu_dump_display(cpu* c){
    printf(":: CPU DUMP:\n");
    printf("    REGISTERS:\n");
    printf("        program counter : 0x%04x\n", c->program_counter);
    printf("        stack ptr       : 0x%02x\n", c->stack_ptr);
    printf("        register a      : 0x%02x\n", c->a);
    printf("        register x      : 0x%02x\n", c->ir_x);
    printf("        register y      : 0x%02x\n", c->ir_y);
    printf("    PROCESSOR STATUS:\n");
    printf("        carry           : %d\n", c->ps.carry_flag);
    printf("        zero            : %d\n", c->ps.zero_flag);
    printf("        interrupt       : %d\n", c->ps.interrupt_flag);
    printf("        decimal         : %d\n", c->ps.decimal_flag);
    printf("        break           : %d\n", c->ps.break_flag);
    printf("        overflow        : %d\n", c->ps.overflow_flag);
    printf("        negative        : %d\n", c->ps.negative_flag);
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

    r->data[0x0200] = LDA_INDERECT_Y;
    r->data[0x0201] = 0x44;
    r->data[0x44] = 0x10;
    r->data[0x45] = 0x12;
    
    r->data[0x1213] = 0x70;
    
    //-- hack--
    c->ir_y = 0x03;

    size_t cu = cpu_execute(c,r);
    printf("CPU cycles used for execution: %d\n", cu);
    cpu_dump_display(c);

    exit (0);
}