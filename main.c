#include <avr/io.h>

// See https://stackoverflow.com/questions/30422367
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#define PORT_(port) PORT##port
#define DDR_(port) DDR##port
#define PIN_(port) PIN##port

#define PORT(port) PORT_(port)
#define DDR(port) DDR_(port)
#define PIN(port) PIN_(port)

// FIXME
#define SPEAKER_PORT D
#define SPEAKER_PIN 6
#define MORSE_FREQUENCY 400
#define MORSE_DURATION_SHORT 500
#define MORSE_DURATION_LONG 2 * MORSE_DURATION_SHORT

#define F_CPU 1000000UL

const char *alphabet = "**ETIANMSURWDKGOHVF*L*PJBXCYZQ**";
#define ALPHABET_LENGTH

// PROTOTYPES
// =============================================================================

void beep(float freq, float duration);
void single_morse_beep(uint8_t decimal);
void morse_char(char c);

void initialize_ports(void) { DDR(SPEAKER_PORT) |= (1 << SPEAKER_PIN); }

// MAIN
// =============================================================================

int main(void) { initialize_ports(); }

// FUNCTIONS
// =============================================================================

// Logic from https://www.petervis.com/C/pizo%20speaker/pizo%20speaker.html
void beep(float freq, float duration) {

  float wavelength = (1 / freq) * 1000;
  uint32_t cycles = duration / wavelength;
  float half_period = wavelength / 2;

  for (uint32_t i = 0; i < cycles; i++) {
    _delay_ms(half_period);
    PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
    _delay_ms(half_period);
    PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);
  }

  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void single_morse_beep(uint8_t decimal) {
  if (decimal) {
    single_morse_beep(decimal / 2);
    if (decimal != 1) {
      if (decimal % 2)
        beep(MORSE_DURATION_LONG, MORSE_FREQUENCY);
      else
        beep(MORSE_DURATION_SHORT, MORSE_FREQUENCY);
    }
  }
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void morse_char(char c) {
  if (c >= 'a' && c <= 'z')
    c -= ALPHABET_LENGTH; // convert to uppercase
  if (c < 'A' || c > 'Z')
    return;
  uint8_t i = 0;
  while (alphabet[++i] != c)
    ;
  single_morse_beep(i);
}
