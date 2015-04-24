int main(void)
{
    __asm__ volatile (
            "movl $0, %eax \n"
            "int $0x80  \n"
            );
    for(;;) {
        __asm__ volatile ("movl $16, %ecx");
    }

    return 0;
}
