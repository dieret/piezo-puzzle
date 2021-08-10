#include <avr/io.h>

// See https://stackoverflow.com/questions/30422367
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>


#define PORT_(port) PORT ## port
#define DDR_(port)  DDR  ## port
#define PIN_(port)  PIN  ## port

#define PORT(port) PORT_(port)
#define DDR(port)  DDR_(port)
#define PIN(port)  PIN_(port)

// FIXME
#define SPEAKER_PORT D
#define SPEAKER_PIN 6

#define F_CPU 1000000UL

void beep(float freq, float duration);

void initialize_ports(void) {
    DDR(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
}

int main(void) {
    initialize_ports();
}

// Logic from https://www.petervis.com/C/pizo%20speaker/pizo%20speaker.html
void beep(float freq, float duration)
{

    float wavelength=(1/freq)*1000;
    uint32_t cycles=duration/wavelength;
    float half_period = wavelength/2;

    for (uint32_t i=0; i<cycles; i++)
    {
        _delay_ms(half_period);
        PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
        _delay_ms(half_period);
        PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);
    }

    return;
}