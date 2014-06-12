/**
 * @file keyboard.h
 * @brief keyboard header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_



#include <state_code.h>
#include <stdint.h>
#include <stdbool.h>
#include <aqueue.h>

struct keyboard {
    bool enable_calps_lock;
    bool enable_num_lock;
    bool enable_scroll_lock;
    Aqueue aqueue;
};
typedef struct keyboard Keyboard;


union keyboard_controle_status {
    struct {
        uint8_t output_buf_full:1;
        uint8_t input_buf_full:1;
        uint8_t system_flag:1;
        uint8_t cmd_data_flag:1;
        uint8_t lock_flag:1;
        uint8_t read_timeout:1;
        uint8_t write_timeout:1;
        uint8_t parity_error:1;
    };
    uint8_t bit_expr;
};
typedef union keyboard_controle_status Keyboard_controle_status;
_Static_assert(sizeof(Keyboard_controle_status) == 1, "Keyboard_controle_status is NOT 1 byte.");


enum keyboard_constants {
    KEYBOARD_CONTROL_PORT   = 0x64, /* Read: Status register,  Write: Command register */
    KEYBOARD_DATA_PORT      = 0x60, /* Read: Output from keyboard register, Write: input to keyboard register */
    KEYBOARD_BUFFER_SIZE    = 1024, /* keyboard buffer size */
    KEYBOARD_STATUS_OBF     = 0x01, /* アウトプットバッファが空かどうか */
    KEYBOARD_STATUS_IBF     = 0x02, /* インプットプットバッファが空かどうか */
    KEYBOARD_STATUS_F0      = 0x04, /*
                                     * システムフラグ
                                     * 制御コマンドの書き込みに応じて値が変化する
                                     */
    KEYBOARD_STATUS_F1      = 0x08, /*
                                     * コマンド/データフラグ
                                     * データを書き込んだ際は0, コマンドを書き込んだ際は1
                                     */
    KEYBOARD_STATUS_ST4     = 0x10, /*
                                     * 禁止フラグ
                                     * キーボードの禁止キーが押されている場合1となる
                                     */
    KEYBOARD_STATUS_ST5     = 0x20, /*
                                     * 書き込みタイムアウトフラグ
                                     * PS/2 ビット0の値が1であれば
                                     *  0:ポートアドレス0x60から読み込むデータはキーボードエンコーダからのデータ
                                     *  1:ポートアドレス0x60から読み込むデータはマウスからのデータ
                                     * AT
                                     *  0:キーボードコントローラからの書き込みが完了した状態
                                     *  1:キーボードコントローラからの書き込みが完了していない状態。または書き込みエラーが起こった状態。
                                     *      ソフトウェアで一定時間タイムアウトを監視する
                                     */
    KEYBOARD_STATUS_ST6     = 0x40, /*
                                     * 読み込みタイムアウトフラグ
                                     * 0:キーボードコントローラからの読み込みが完了した状態
                                     * 1:キーボードコントローラからの読み込みが完了していない状態。または読み込みエラーが起こった状態。
                                     */
    KEYBOARD_STATUS_ST7     = 0x80, /*
                                     * パリティエラー
                                     * 0:キーボードコントローラから読み込んだ最後のバイトは奇数パリティ
                                     * 1：キーボードコントローラから読み込み込んだ最後のバイトは偶数パリティ
                                     * キーボードコントローラは通常奇数パリティを送信する
                                     */

    KEYBOARD_DATA_COMMAND_LED           = 0xED, /* Set LED for scroll, num, caps lock */
    KEYBOARD_LED_NUM                    = 0x01,
    KEYBOARD_LED_SCROLL                 = 0x02,
    KEYBOARD_LED_CAPS                   = 0x04,
    KEYBOARD_DATA_COMMAND_ECHO          = 0xEE, /* return writting data(0xEE) to 0x60 */
    KEYBOARD_DATA_COMMAND_SCANCODE      = 0xF0, /* get or set scan code */
    KEYBOARD_DATA_COMMAND_KEYBOARD_ID   = 0xF2, /* get keyboard id, after writting this command, it takes at least 10ms. */

    KEYBOARD_CONTROL_COMMAND_WRITE_MODE = 0x60, /* write keyboard controller command */
    KEYBOARD_CONTROL_COMMAND_SELF_TEST  = 0xAA, /* do self test -> 0x55 is OK result, 0xFC is NG. */
    KEYBOARD_CONTROL_COMMAND_READ_INPUT = 0xC0, /* read data from input port, result is in 0x64. */

    KEYBOARD_CONTROL_COMMAND_BYTE_KIE   = 0x01, /* switch keyboard interruput. */
    KEYBOARD_CONTROL_COMMAND_BYTE_MIE   = 0x02, /* switch mouse interruput. */
    KEYBOARD_CONTROL_COMMAND_BYTE_SYSF  = 0x04, /* system flag. */
    KEYBOARD_CONTROL_COMMAND_BYTE_IGNLK = 0x08, /* ignore keyboard lock. */
    KEYBOARD_CONTROL_COMMAND_BYTE_KE    = 0x10, /* enable keyboard. */
    KEYBOARD_CONTROL_COMMAND_BYTE_ME    = 0x20, /* enable mouse. */
    KEYBOARD_CONTROL_COMMAND_BYTE_XLATE = 0x40, /* enable scan code translation. */
};


extern Axel_state_code init_keyboard(void);



#endif
