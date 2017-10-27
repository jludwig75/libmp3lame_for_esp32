#include "esp32-hal-spi.h"
#include "esp32-hal-gpio.h"


int8_t _spi_num;
spi_t * _spi;
int8_t _sck;
int8_t _miso;
int8_t _mosi;
int8_t _ss;
uint32_t _div;
uint32_t _freq;
int8_t _cs;

void mcp3201_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, int8_t cs)
{
    if(_spi) {
        return;
    }

    if(!_div) {
        _div = spiFrequencyToClockDiv(_freq);
    }

    _spi = spiStartBus(_spi_num, _div, SPI_MODE0, SPI_MSBFIRST);
    if(!_spi) {
        return;
    }

    _sck = sck;
    _miso = miso;
    _mosi = mosi;
    _ss = ss;
    _cs = cs;

    spiAttachSCK(_spi, _sck);
    spiAttachMISO(_spi, _miso);
    spiAttachMOSI(_spi, _mosi);


    spiSetBitOrder(_spi, SPI_MSBFIRST /*MSBFIRST*/);

    _div = SPI_CLOCK_DIV4;
     spiSetClockDiv(_spi, _div);
}

uint16_t mcp3201_get_value()
{
	uint16_t result;
	uint8_t inByte;

	digitalWrite(_cs, LOW);
	result = spiTransferByte(_spi, 0x00);
	result = result << 8;
	inByte = spiTransferByte(_spi, 0x00);
	result = result | inByte;
	digitalWrite(_cs, HIGH);
	result = result >> 1;
	result = result & 0b0000111111111111;

	return result;
}
