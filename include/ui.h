#ifndef UI_H
#define UI_H

int32_t read_gamepad(void);

int32_t read_key(void);

int32_t read_console(void);

void write_console(int32_t c);

void write_sound(uint8_t sample);

void finished(int code);

#endif
