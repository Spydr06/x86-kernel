#include "console.h"
#include "serial.h"
#include <string.h>
#include "../../lib/log.h"
#include "keyboard.h"

#define FB ((uint8_t*) 0x000B8000)

//uint8_t* fb = (uint8_t*) 0x000B8000;
uint16_t cursor_pos = 0;
uint8_t fg_color = FB_LT_GREY;
uint8_t bg_color = FB_BLACK;

void console_move_cursor(uint16_t pos)
{
    cursor_pos = pos;
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT,    ((pos >> 8) & 0x00ff));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT,    pos & 0x00ff);
}

uint16_t console_cursor_pos(void)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    uint16_t pos = inb(FB_DATA_PORT) << 8;
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    pos |= inb(FB_DATA_PORT);
    return pos;
}

void console_write_cell(uint32_t i, char c, uint8_t bg, uint8_t fg)
{
    FB[i] = c;
    FB[i + 1] = ((fg & 0x0f) << 4) | (bg & 0x0f);
}

void console_scroll(void)
{
    for(int pos = 1; pos < FB_HEIGHT; pos++)
        for(int i = 0; i < FB_WIDTH * 2; i++)
            FB[(pos - 1) * FB_WIDTH * 2 + i] = FB[pos * FB_WIDTH * 2 + i];
}

void console_color(uint8_t fg, uint8_t bg)
{
    fg_color = fg;
    bg_color = bg;
}

void console_delete_last_line(void)
{
    for(int32_t x = 0; x < FB_WIDTH * 2; x++) {
		uint8_t* ptr = FB + (FB_WIDTH * 2) * (FB_HEIGHT - 1) + x;
		*ptr = 0;
	}
}

int console_write(const char* buf, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++) {
        switch(buf[i]) {
        case '\n':
            cursor_pos += FB_WIDTH; // fallthrough
        case '\r':
            cursor_pos = cursor_pos / FB_WIDTH * FB_WIDTH;
            break;
        case '\t':
            cursor_pos = (cursor_pos + FB_TABSIZE) / FB_TABSIZE * FB_TABSIZE;
            break;
        case '\b':
            if(cursor_pos > 0)
                console_write_cell(--cursor_pos * 2, ' ', fg_color, bg_color);
            break;
        case '\e':
            if(buf[i + 1] == '[' && buf[i + 2] == 'A') // up arrow
                cursor_pos -= FB_WIDTH;
            else if(buf[i + 1] == '[' && buf[i + 2] == 'D') // left arrow
                cursor_pos -= 1;
            else if(buf[i + 1] == '[' && buf[i + 2] == 'B') // down arrow
                cursor_pos += FB_WIDTH;
            else if(buf[i + 1] == '[' && buf[i + 2] == 'C') // right arrow
                cursor_pos += 1;
            else
                break;
            i += 2;
            break;
        default:
            console_write_cell(cursor_pos++ * 2, buf[i], fg_color, bg_color);
        }

        if(cursor_pos / FB_WIDTH > FB_HEIGHT - 1) {
            console_scroll();
            console_delete_last_line();
            cursor_pos -= FB_WIDTH;
        }
    }
    console_move_cursor(cursor_pos);
    return len;
}

void console_init(void)
{
    cursor_pos = console_cursor_pos();
    klog(KLOG_OK, "framebuffer initialized");
}

int console_puts(const char* buf)
{
    return console_write(buf, strlen(buf));
}
