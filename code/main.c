#define F_CPU 1000000UL
#include <avr/io.h>

float song0[4][2] = {
    {220.0, 500.0}, {440.0, 500.0}, {220.0, 500.0}, {0.0, -1.0}};

float song1[4][2] = {
    {440.0, 500.0}, {220.0, 500.0}, {440.0, 500.0}, {0.0, -1.0}};

float song2[3][2] = {{440.0, 500.0}, {220.0, 500.0}, {0.0, -1.0}};

float hint[2][2] = {{440.0, 500.0}, {0.0, -1.0}};

float **songs[4] = {(float **)song0, (float **)song1, (float **)song2,
                    (float **)hint};

// See https://stackoverflow.com/questions/30422367
#define __DELAY_BACKWARD_COMPATIBLE__
// By default, _delay_ms and _delay_us always round up if the delay time isn't
// a multiple of clock cycles. We don't want that, so
#define __DELAY_ROUND_CLOSEST__

#include <util/delay.h>

// measured on output for one specific implementation
#define DELAY_OVERHEAD_US 1320

#define PORT_(port) PORT##port
#define DDR_(port) DDR##port
#define PIN_(port) PIN##port

#define PORT(port) PORT_(port)
#define DDR(port) DDR_(port)
#define PIN(port) PIN_(port)

// FIXME
#define SPEAKER_PORT D
#define SPEAKER_PIN 2

#define WHEEL_PORT D
#define WHEEL_MASK ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 7))

// Audio parameters
#define FAIL_SOUND_FREQ 100
#define FAIL_SOUND_DUR 300

#define BOOT_SOUND_FREQ 200
#define BOOT_SOUND_DUR 400

#define MORSE_FREQ 400
#define MORSE_DOT_DUR 100
#define MORSE_DASH_DUR 3 * MORSE_DOT_DUR
#define MORSE_SHORT_GAP 3 * MORSE_DOT_DUR
#define MORSE_MEDIUM_GAP 7 * MORSE_DOT_DUR

// Riddle Parameters
enum position {
  WHI = 0,
  TEN = 2,
  SMI = 4,
  CAP = 6,
  ECC = 8,
  SONG0 = 10,
  SONG1 = 12,
  SONG2 = 14
};

char RIDDLE_MESSAGES[10][3] = {"WHI", "",    "TEN", "",    "SMI",
                               "",    "CAP", "",    "ECC", ""};

const int SOLUTION[5] = {WHI, CAP, SMI, TEN, ECC};
const int SOLUTION_LENGTH = 5;

const int HINT_COMBINATION[5] = {TEN, SMI, WHI, TEN, CAP};
const int HINT_LENGTH = 5;

/**
 * Minimal time that the wheel has to stay in one position for us to act
 * on it
 */
const int MIN_HOVER_TIME = 1000;

/**
 * Used in implementation of morse code
 */
const char *alphabet = "**ETIANMSURWDKGOHVF*L*PJBXCYZQ**";
#define ALPHABET_LENGTH 32

// PROTOTYPES
// =============================================================================

// TODO! Just returns 0 at the moment
uint8_t get_wheel_pos(void);

void interrupting_delay(float ms, int on_position);

void beep_forever(float freq);
void beep(float freq, float duration, int on_position);
void play_boot_sound(void);
void play_fail_sound(int on_position);
void play_song(int song_number, int on_position);
void play_audio(enum position *history);

void morse_message(int k, int on_position);

void single_morse_beep(uint8_t decimal, int on_position);
void morse_char(char c, int on_position);

void push_history(enum position *history, int value);

/**
 * Called once at the beginning to set up ports as inputs/outputs
 */
void initialize_ports(void);

/**
 * Play a series of tunes to check that speaker is working correctly. Also used
 * to calibrated pitch.
 */
void sound_test(void);

/**
 * Used for debugging: As many beeps as the wheel position indicates
 */
void beep_wheel_pos(void);

// MAIN
// =============================================================================
int main(void) {
  initialize_ports();

  play_boot_sound();

  int last_position = get_wheel_pos();
  enum position history[6] = {0, 0, 0, 0, 0, 0};

  // Wait for first wheel change to start riddle
  while (get_wheel_pos() == last_position) {
  }

  while (1) {
    int current_pos = get_wheel_pos();

    if (current_pos != last_position) {
      // this is done unconditionally and for all wheel positions
      // to allow replaying hints by moving back and forth quickly
      last_position = current_pos;

      // only register even positions
      if (current_pos % 2 == 0) {
        // wait before registering the new position
        int hover_ms_count = 0;
        for (; hover_ms_count < MIN_HOVER_TIME; hover_ms_count++) {
          _delay_ms(1.0);
          // cancel if we change wheel while waiting
          if (get_wheel_pos() != current_pos) {
            break;
          }
        }
        // register new position and give audio feedback
        if (hover_ms_count == MIN_HOVER_TIME) {
          push_history(history, current_pos);
          play_audio(history);
        }
      }
    }
  }
}

// FUNCTIONS
// =============================================================================

void initialize_ports(void) {
  // Speaker pin is output
  DDR(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
  // Wheel pins are input
  DDR(WHEEL_PORT) &= ~WHEEL_MASK;
  // Wheel pins use internal pull up
  PORT(WHEEL_PORT) |= WHEEL_MASK;
}

void beep_wheel_pos(void) {
  uint8_t n_beeps = get_wheel_pos();
  for (uint8_t i = 0; i < n_beeps; i++) {
    beep(MORSE_FREQ, MORSE_DOT_DUR, -1);
    _delay_ms(MORSE_SHORT_GAP);
  }
}

void sound_test() {
  beep(293.665, 500, -1);
  beep(329.628, 500, -1);
  beep(369.994, 500, -1);
  beep(391.995, 500, -1);
  beep(440.000, 500, -1);
  beep(493.883, 500, -1);
  beep(554.365, 500, -1);
  beep(587.330, 500, -1);
  beep(622.254, 500, -1);
  beep(659.255, 500, -1);
  beep(739.989, 500, -1);
  beep(783.991, 500, -1);
  beep(880.000, 500, -1);
  beep(987.767, 500, -1);
  beep(1108.731, 500, -1);
  beep(1174.659, 500, -1);
}

void beep_forever(float freq) {
  float wavelength = (1 / freq) * 1000;
  double half_period_us = (double)1000 * wavelength / 2;

  while (1) {
    _delay_us(half_period_us);
    PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
    _delay_us(half_period_us);
    PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);
  }
}

// Logic from https://www.petervis.com/C/pizo%20speaker/pizo%20speaker.html
void beep(float freq, float duration, int on_position) {

  // low frequency beeps are seen as silence
  if (freq < 1) {
    interrupting_delay(duration, on_position);
    return;
  }

  float wavelength = (1 / freq) * 1000;
  uint32_t cycles = (uint32_t)duration / wavelength;
  double half_period_us = (double)1000 * wavelength / 2;
  double compensated_waiting_time = half_period_us - DELAY_OVERHEAD_US / 2

  for (uint32_t i = 0; i < cycles; i++) {
    _delay_us(compensated_waiting_time);
    PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
    _delay_us(compensated_waiting_time);
    PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);

    // abort if wheel position has changed
    // we only check this every 100 times because loading variables and
    // executing functions might be expensive
    if (i % 100 == 0 && on_position >= 0 && get_wheel_pos() != on_position)
      return;
  }

  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void single_morse_beep(uint8_t decimal, int on_position) {
  if (decimal) {
    single_morse_beep(decimal / 2, on_position);
    interrupting_delay(MORSE_DOT_DUR, on_position);
    if (decimal != 1) {
      if (decimal % 2)
        beep(MORSE_DASH_DUR, MORSE_FREQ, on_position);
      else
        beep(MORSE_DOT_DUR, MORSE_FREQ, on_position);
    }
    interrupting_delay(MORSE_DOT_DUR, on_position);
  }
}

void interrupting_delay(float duration, int on_position) {
  while (duration > 50) {
    _delay_ms(50.0);
    duration -= 50.0;
    if (on_position >= 0 && get_wheel_pos() != on_position)
      return;
  }
  _delay_ms(duration);
  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void morse_char(char c, int on_position) {
  if (c == ' ') {
    interrupting_delay(MORSE_MEDIUM_GAP, on_position);
    return;
  }
  if (c >= 'a' && c <= 'z')
    c -= ALPHABET_LENGTH; // convert to uppercase
  if (c < 'A' || c > 'Z')
    return;
  uint8_t i = 0;
  while (alphabet[++i] != c)
    ;
  single_morse_beep(i, on_position);
}

void play_boot_sound(void) { beep(BOOT_SOUND_FREQ, BOOT_SOUND_DUR, -1); }

void play_fail_sound(int on_position) {
  beep(FAIL_SOUND_FREQ, FAIL_SOUND_DUR, on_position);
}

void play_audio(enum position *history) {
  // check if combination for hint was entered
  for (int k = 0; k < HINT_LENGTH; k++) {
    if (history[k] != HINT_COMBINATION[k])
      break;

    if (k == HINT_LENGTH - 1) {
      play_song(3, history[0]);
      return;
    }
  }

  // are we on a "riddle" position? (hardcoded)
  if (history[0] < 10) {
    morse_message(history[0], history[0]);
    return;
  }

  // check if solution was entered
  for (int k = 1; k < SOLUTION_LENGTH + 1; k++) {
    if (history[k] != SOLUTION[k]) {
      play_fail_sound(history[0]);
      return;
    }
    if (k == SOLUTION_LENGTH) {
      play_song(history[0] / 2 - 5, history[0]);
      return;
    }
  }
}

void push_history(enum position *history, int value) {
  for (int k = 0; k < 5; k++)
    history[k + 1] = history[k];

  history[0] = value;
}

void play_song(int song_number, int on_position) {
  float **song = songs[song_number];
  for (int k = 0; song[k][2] >= 0; k++) {
    beep(song[k][0], song[k][1], on_position);
  }
}

void morse_message(int k, int on_position) {
  for (int m = 0; RIDDLE_MESSAGES[k][m]; m++) {
    morse_char(RIDDLE_MESSAGES[k][m], on_position);
    interrupting_delay(MORSE_SHORT_GAP, on_position);
    if (on_position >= 0 && get_wheel_pos() != on_position) {
      return;
    }
  }
}

uint8_t get_wheel_pos(void) { return PIN(WHEEL_PORT) & WHEEL_MASK; }
