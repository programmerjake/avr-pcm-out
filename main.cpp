#ifdef __AVR__
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_byte(v) (*(v))
#include <fstream>
#endif
#include <stdint.h>

#include "final2.h"

#ifdef __AVR__
static void SetFastClock() // set clock to 8 MHz
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        CLKPR = _BV(CLKPCE);
        CLKPR = 0;
    }
}

static void StartPLL()
{
    PLLCSR = 0x86;
    while((PLLCSR & 1) == 0)
        ;
}

static void StartTimer()
{
    StartPLL();
    TCCR1 = 0x71;
    GTCCR = 0x00;
    OCR1A = 0x80;
    OCR1B = 0xFF;
    OCR1C = 0xFF;
    TIMSK = 0x20;
}

static void WriteToTimer(uint8_t value)
{
    while(1)
    {
        if(TIFR & 0x20)
        {
            TIFR = 0x20;
            OCR1A = value;
            return;
        }
    }
}
#endif

struct Decoder
{
    unsigned index;
    bool gotSecondNibble;
    uint8_t currentByte;
    uint8_t lastSample;
    Decoder()
        : index(0), gotSecondNibble(true)
    {
        lastSample = pgm_read_byte(&sound_data[index++]);
        currentByte = 0;
    }
    uint8_t readNibble()
    {
        if(gotSecondNibble)
        {
            gotSecondNibble = false;
            currentByte = pgm_read_byte(&sound_data[index++]);
            return currentByte & 0xF;
        }
        else
        {
            gotSecondNibble = true;
            return currentByte >> 4;
        }
    }
    uint8_t readDoubleNibble()
    {
        if(gotSecondNibble)
        {
            //currentByte = pgm_read_byte(&sound_data[index++]);
            //return currentByte;
            return pgm_read_byte(&sound_data[index++]); // skip writing to currentByte
        }
        else
        {
            uint8_t retval = currentByte >> 4;
            currentByte = pgm_read_byte(&sound_data[index++]);
            retval |= currentByte << 4;
            return retval;
        }
    }
    uint8_t getFirstSample()
    {
        return lastSample;
    }
    uint8_t getNextSample()
    {
        uint8_t delta = readNibble();
        if(delta == 0x8)
        {
            delta = readDoubleNibble();
        }
        else if(delta & 0x8)
        {
            delta |= -0x10;
        }
        lastSample += delta;
        return lastSample;
    }
};

int main(void)
{
#ifdef __AVR__
    DDRB = _BV(DDB0) | _BV(DDB1) | _BV(DDB2) | _BV(DDB3) | _BV(DDB4) | _BV(DDB5); // all outputs
    PORTB = _BV(PORTB5);
    SetFastClock();
    StartTimer();

    const int scale_shift = 16;
    const double scale_factor = (int32_t)1 << scale_shift;
    const double output_sample_rate = 32e6 / 256.0;
    const double input_sample_rate = 11025;
    const double float_conversion_factor = input_sample_rate / output_sample_rate;
    const uint32_t int_conversion_factor = (int32_t)(float_conversion_factor * scale_factor);

    while(true)
    {
        Decoder decoder;
        uint8_t currentSample = decoder.getFirstSample();
        uint32_t currentIndex = 0;
        for(uint32_t i = 0, index = i >> scale_shift; index < sample_count; i += int_conversion_factor, index = i >> scale_shift)
        {
            if(currentIndex != index)
            {
                currentIndex = index;
                currentSample = decoder.getNextSample();
            }
            WriteToTimer(currentSample);
        }
        WriteToTimer(0x7F);

        _delay_ms(10000);
    }

    return 0;
#else
    std::ofstream os("out.dat", std::ios::binary);
    Decoder decoder;
    os.put((int)decoder.getFirstSample());
    for(unsigned i = 1; i < sample_count; i++)
        os.put((int)decoder.getNextSample());
    return 0;
#endif
}

