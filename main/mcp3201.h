#pragma once

#include <stdint.h>


void mcp3201_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, int8_t cs);

uint16_t mcp3201_get_value();
