/************************************************************
 * File: include/kernel.h
 * Description: kernel header
 *              TODO: Add comment
 ************************************************************/

#ifndef _KERNEL_H_
#define _KERNEL_H_



/* Programmable Interrupt Controller */
enum PIC_constants {
    /* PIC Port Address */
    PIC0_CMD_STATE_PORT = 0x20,
    PIC1_CMD_STATE_PORT = 0xA0,
    PIC0_IMR_DATA_PORT = 0x21,
    PIC1_IMR_DATA_PORT = 0xA1,

    /* Initialization Control Words */
    /* PICの設定 */
    PIC0_ICW1 = 0x11,
    /* IRQから割り込みベクタへの変換されるベースの番号
     * x86では3-7bitを使用する
     * この番号にPICのピン番号が加算された値の例外ハンドラが呼び出される
     */
    PIC0_ICW2 = 0x20,
    /* スレーブと接続されているピン */
    PIC0_ICW3 = 0x04,
    /* 動作モードを指定 */
    PIC0_ICW4 = 0x01,

    PIC1_ICW1 = PIC0_ICW1,
    PIC1_ICW2 = 0x28,
    PIC1_ICW3 = 0x02,
    PIC1_ICW4 = PIC0_ICW4,

    /* Operation Command Words2 */
    PIC_OCW2_EOI = 0x20,

    PIC_IMR_MASK_IRQ0 = 0x01,
    PIC_IMR_MASK_IRQ1 = 0x02,
    PIC_IMR_MASK_IRQ2 = 0x04,
    PIC_IMR_MASK_IRQ3 = 0x08,
    PIC_IMR_MASK_IRQ4 = 0x10,
    PIC_IMR_MASK_IRQ5 = 0x20,
    PIC_IMR_MASK_IRQ6 = 0x40,
    PIC_IMR_MASK_IRQ7 = 0x80,
    PIC_IMR_MASK_IRQ_ALL = 0xFF,
};



#endif
