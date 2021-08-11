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

// Audio parameters
const float FAIL_SOUND_FREQ = 100;
const float FAIL_SOUND_DUR = 300;

const float BOOT_SOUND_FREQ = 200;
const float BOOT_SOUND_DUR = 400;

const float MORSE_FREQUENCY = 400;

const float MORSE_DOT_DURATION = 100;
const float MORSE_DASH_DURATION = 3 * MORSE_DOT_DURATION;
const float MORSE_SHORT_GAP = 3 * MORSE_DOT_DURATION;
const float MORSE_MEDIUM_GAP = 7 * MORSE_DOT_DURATION;

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

const int SOLUTION[5] = {ECC, TEN, SMI, CAP, WHI};
const int SOLUTION_LENGTH = 5;

const int HINT_COMBINATION[4] = {CAP, TEN, WHI, SMI};
const int HINT_LENGTH = 4;

const int MIN_HOVER_TIME = 1000;

#define F_CPU 1000000UL

const char *alphabet = "**ETIANMSURWDKGOHVF*L*PJBXCYZQ**";
#define ALPHABET_LENGTH 32

// PROTOTYPES
// =============================================================================

// TODO! Just returns 0 at the moment
int get_wheel_pos(void);

void interrupting_delay(float ms, int on_position);

void beep(float freq, float duration, int on_position);
void play_boot_sound(void);
void play_fail_sound(int on_position);
void play_song(int song_number, int on_position);
void play_audio(enum position *history);

void morse_message(int k, int on_position);

void single_morse_beep(uint8_t decimal, int on_position);
void morse_char(char c, int on_position);

void push_history(enum position *history, int value);

void initialize_ports(void) { DDR(SPEAKER_PORT) |= (1 << SPEAKER_PIN); }

// MAIN
// =============================================================================
int main(void) {
  initialize_ports();
  int last_position = get_wheel_pos();
  enum position history[6] = {0, 0, 0, 0, 0, 0};

  play_boot_sound();

  // Wait for first wheel change to start riddle
  while (get_wheel_pos() == last_position) {
  }

  while (1) {
    int current_pos = get_wheel_pos();

    if (current_pos != last_position && current_pos % 2 == 0) {

      // wait before registering the new position
      int hover_ms_count = 0;
      for (; hover_ms_count < MIN_HOVER_TIME; hover_ms_count++) {
        _delay_ms(1.0);
        // cancel if we change wheel while waiting
        if (get_wheel_pos() != current_pos) {
          break;
        }
      }
      // register new position
      if (hover_ms_count == MIN_HOVER_TIME) {
        push_history(history, current_pos);
        play_audio(history);
      }
      // this is done unconditionally to allow
      // replaying by moving back and forth quickly
      last_position = current_pos;
    }
  }
}

// FUNCTIONS
// =============================================================================

// Logic from https://www.petervis.com/C/pizo%20speaker/pizo%20speaker.html
void beep(float freq, float duration, int on_position) {

  // low frequency beeps are seen as silence
  if (freq < 1) {
    interrupting_delay(duration, on_position);
    return;
  }

  float wavelength = (1 / freq) * 1000;
  uint32_t cycles = duration / wavelength;
  float half_period = wavelength / 2;

  for (uint32_t i = 0; i < cycles; i++) {
    _delay_ms(half_period);
    PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
    _delay_ms(half_period);
    PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);

    // abort if wheel position has changed
    if (on_position >= 0 && get_wheel_pos() != on_position)
      return;
  }

  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void single_morse_beep(uint8_t decimal, int on_position) {
  if (decimal) {
    single_morse_beep(decimal / 2, on_position);
    interrupting_delay(MORSE_DOT_DURATION, on_position);
    if (decimal != 1) {
      if (decimal % 2)
        beep(MORSE_DASH_DURATION, MORSE_FREQUENCY, on_position);
      else
        beep(MORSE_DOT_DURATION, MORSE_FREQUENCY, on_position);
    }
    interrupting_delay(MORSE_DOT_DURATION, on_position);
  }
}

void interrupting_delay(float duration, int on_position) {
  while (duration > 1) {
    _delay_ms(1.0);
    duration -= 1.0;
    if (get_wheel_pos() != on_position)
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
    if (get_wheel_pos() != on_position) {
      return;
    }
  }
}

int get_wheel_pos(void) { return 0; }
