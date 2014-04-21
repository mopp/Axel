/** @file keyboard.h
 * @brief keyboard header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H


#include <state_code.h>
#include <stdint.h>

struct keyboard {
    uint8_t enable_calps_lock;
    uint8_t enable_num_lock;
    uint8_t enable_scroll_lock;
    // Queue8 buffer;
};
typedef struct keyboard  Keyboard;


enum keyboard_constants {
    /*
     * Read: Status register
     * Write: Command register
     */
    KEYBOARD_CONTROL_PORT   = 0x64,
    /*
     * Read: Output from keyboard register
     * Write: input to keyboard register
     */
    KEYBOARD_DATA_PORT      = 0x60,

    /* keyboard buffer size */
    KEYBOARD_BUFFER_SIZE    = 1024,

    /* アウトプットバッファが空ならば0 */
    KEYBOARD_STATUS_OBF     = 0x01,
    /* インプットプットバッファが空ならば0 */
    KEYBOARD_STATUS_IBF     = 0x02,
    /*
     * システムフラグ
     * 制御コマンドの書き込みに応じて値が変化する
     */
    KEYBOARD_STATUS_F0      = 0x04,
    /*
     * コマンド/データフラグ
     * データを書き込んだ際は0, コマンドを書き込んだ際は1
     */
    KEYBOARD_STATUS_F1      = 0x08,
    /*
     * 禁止フラグ
     * キーボードの禁止キーが押されている場合1となる
     */
    KEYBOARD_STATUS_ST4     = 0x10,
    /*
     * 書き込みタイムアウトフラグ
     * PS/2 ビット0の値が1であれば
     *  0:ポートアドレス0x60から読み込むデータはキーボードエンコーダからのデータ
     *  1:ポートアドレス0x60から読み込むデータはマウスからのデータ
     * AT
     *  0:キーボードコントローラからの書き込みが完了した状態
     *  1:キーボードコントローラからの書き込みが完了していない状態。または書き込みエラーが起こった状態。
     *      ソフトウェアで一定時間タイムアウトを監視する
     */
    KEYBOARD_STATUS_ST5     = 0x20,
    /*
     * 読み込みタイムアウトフラグ
     * 0:キーボードコントローラからの読み込みが完了した状態
     * 1:キーボードコントローラからの読み込みが完了していない状態。または読み込みエラーが起こった状態。
     */
    KEYBOARD_STATUS_ST6     = 0x40,
    /*
     * パリティエラー
     * 0:キーボードコントローラから読み込んだ最後のバイトは奇数パリティ
     * 1：キーボードコントローラから読み込み込んだ最後のバイトは偶数パリティ
     * キーボードコントローラは通常奇数パリティを送信する
     */
    KEYBOARD_STATUS_ST7     = 0x80,

    /* Set LED for scroll, num, caps lock */
    KEYBOARD_DATA_COMMAND_LED           = 0xED,
    KEYBOARD_LED_NUM                    = 0x01,
    KEYBOARD_LED_SCROLL                 = 0x02,
    KEYBOARD_LED_CAPS                   = 0x04,
    /* return writting data(0xEE) to 0x60 */
    KEYBOARD_DATA_COMMAND_ECHO          = 0xEE,
    /* get or set scan code */
    KEYBOARD_DATA_COMMAND_SCANCODE      = 0xF0,
    /* get keyboard id, after writting this command, it takes at least 10ms. */
    KEYBOARD_DATA_COMMAND_KEYBOARD_ID   = 0xF2,

    /* write keyboard controller command */
    KEYBOARD_CONTROL_COMMAND_WRITE_MODE = 0x60,
    /* do self test -> 0x55 is OK result, 0xFC is NG. */
    KEYBOARD_CONTROL_COMMAND_SELF_TEST  = 0xAA,
    /* read data from input port, result is in 0x64. */
    KEYBOARD_CONTROL_COMMAND_READ_INPUT = 0xC0,

    /* switch keyboard interruput. */
    KEYBOARD_CONTROL_COMMAND_BYTE_KIE   = 0x01,
    /* switch mouse interruput. */
    KEYBOARD_CONTROL_COMMAND_BYTE_MIE   = 0x02,
    /* system flag. */
    KEYBOARD_CONTROL_COMMAND_BYTE_SYSF  = 0x04,
    /* ignore keyboard lock. */
    KEYBOARD_CONTROL_COMMAND_BYTE_IGNLK = 0x08,
    /* enable keyboard. */
    KEYBOARD_CONTROL_COMMAND_BYTE_KE    = 0x10,
    /* enable mouse. */
    KEYBOARD_CONTROL_COMMAND_BYTE_ME    = 0x20,
    /* enable scan code translation. */
    KEYBOARD_CONTROL_COMMAND_BYTE_XLATE = 0x40,
};


extern Axel_state_code init_keyboard(void);
extern Axel_state_code set_keyboard_led(uint8_t);


#endif
