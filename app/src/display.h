#pragma once

#include <stdint.h>

void display_init(void);
void epd_clear(void);
void display_refresh(void);
uint8_t *display_get_buf(void);
