#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"

// Test a very basic brk
int brk1()
{
    void* current_break_0 = sbrk(0); 
    printf("break starts at: %p ( %lu)\n", current_break_0, (size_t) current_break_0);

    void* invalid_address_1 = (void*) ((size_t) current_break_0 - 1);
    void* invalid_address_2 = (void*) ((size_t) current_break_0 - 4096);
    void* invalid_address_3 = (void*) ((size_t) current_break_0 - 819203548645);     // random number, no meaning
    void* invalid_address_4 = 0;

    void* valid_address_5 = current_break_0;
    void* valid_address_6 = (void*) ((size_t) current_break_0 + 4096);
    void* valid_address_7 = (void*) ((size_t) current_break_0 + 1192035);

    int brk_result_1 = brk(invalid_address_1);
    assert(brk_result_1 == -1);
    void* current_break_1 = sbrk(0);
    printf("current break after invalid_address_1: %p ( %lu)\n", current_break_1, (size_t) current_break_1);
    assert(current_break_0 == current_break_1);

    int brk_result_5 = brk(valid_address_5);
    assert(brk_result_5 == 0);
    void* current_break_5 = sbrk(0);
    printf("current break after valid_address_5: %p ( %lu)\n", current_break_5, (size_t) current_break_5);
    assert(current_break_5 == valid_address_5);

    int brk_result_2 = brk(invalid_address_2);
    assert(brk_result_2 == -1);
    void* current_break_2 = sbrk(0);
    printf("current break after invalid_address_2: %p ( %lu)\n", current_break_2, (size_t) current_break_2);
    assert(current_break_5 == current_break_2);

    int brk_result_6 = brk(valid_address_6);
    assert(brk_result_6 == 0);
    void* current_break_6 = sbrk(0);
    printf("current break after valid_address_6: %p ( %lu)\n", current_break_6, (size_t) current_break_6);
    assert(current_break_6 == valid_address_6);

    int brk_result_3 = brk(invalid_address_3);
    assert(brk_result_3 == -1);
    void* current_break_3 = sbrk(0);
    printf("current break after invalid_address_3: %p ( %lu)\n", current_break_3, (size_t) current_break_3);
    assert(current_break_6 == current_break_3);

    int brk_result_7 = brk(valid_address_7);
    assert(brk_result_7 == 0);
    void* current_break_7 = sbrk(0);
    printf("current break after valid_address_7: %p ( %lu)\n", current_break_7, (size_t) current_break_7);
    assert(current_break_7 == valid_address_7);

    int brk_result_4 = brk(invalid_address_4);
    assert(brk_result_4 == -1);
    void* current_break_4 = sbrk(0);
    printf("current break after invalid_address_4: %p ( %lu)\n", current_break_4, (size_t) current_break_4);
    assert(current_break_7 == current_break_4);


    return 0;
}
