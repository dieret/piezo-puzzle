#define F_CPU 1000000UL
#include <avr/io.h>

// See https://stackoverflow.com/questions/30422367
#define __DELAY_BACKWARD_COMPATIBLE__
// By default, _delay_ms and _delay_us always round up if the delay time isn't
// a multiple of clock cycles. We don't want that, so
#define __DELAY_ROUND_CLOSEST__

#include <util/delay.h>

// Port definitions
// -----------------------------------------------------------------------------

// A bit of preprocessor magic to make defining ports/pins easier
#define PORT_(port) PORT##port
#define DDR_(port) DDR##port
#define PIN_(port) PIN##port
#define PORT(port) PORT_(port)
#define DDR(port) DDR_(port)
#define PIN(port) PIN_(port)

#define SPEAKER_PORT B
#define SPEAKER_PIN 1

// Note that the implementation only works if all wheel pins are
// "next to each other"
#define WHEEL_PORT D
#define WHEEL_MASK 0xF0
#define WHEEL_BIT_SHIFT_RIGHT 4

// Audio parameters
// -----------------------------------------------------------------------------

/**
 * Calibration for sleep time in sound generation. Measured on output for one
 * specific implementation. See implementation of `beep`
 */
#define DELAY_OVERHEAD_US 1320

// Frequencies are in Hz
// Durations in ms

#define FAIL_SOUND_FREQ 100
#define FAIL_SOUND_DUR 300

#define BOOT_SOUND_FREQ 300
#define BOOT_SOUND_DUR 250

#define MORSE_FREQ 400
#define MORSE_DOT_DUR 100
#define MORSE_DASH_DUR 3 * MORSE_DOT_DUR
#define MORSE_SHORT_GAP 3 * MORSE_DOT_DUR
#define MORSE_MEDIUM_GAP 7 * MORSE_DOT_DUR

// A quick beep, whenever a wheel position gets locked in (i.e. triggers
// something)
#define USE_LOCKED_IN_BEEPS
#define LOCKED_IN_FREQ 600
#define LOCKED_IN_DUR 50
#define LOCKED_IN_BREAK 300

// Debug configuration
// -----------------------------------------------------------------------------

// [DEBUG] Beep full history when one of the song positions is locked in but
// the combination is wrong. Uncomment next line to activate
// #define BEEP_HISTORY_ON_FAILURE

/**
 * A long beep when beeping a number corresponds to this number. Used for
 * beep_number in debugging.
 */
#define BEEP_NUMBER_LONG_NUMBER 4

// Puzzle configuration
// -----------------------------------------------------------------------------

// Position of the wheel and its meanings.
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

// Assuming that everything >= this number is a song position
#define FIRST_SONG_POSITION 10

/**
 * Number of wheel positions kept in history
 */
#define HISTORY_LENGTH 6

/**
 * Riddle messages as a 10 (message number) x 3 (number of chars in message)
 * array.
 */
const __flash char RIDDLE_MESSAGES[10][3] = {"WHI", "",    "TEN", "",    "SMI",
                                             "",    "CAP", "",    "ECC", ""};

/**
 * The solution to the riddle in reverse (!) order
 */
const __flash uint8_t SOLUTION[5] = {WHI, CAP, SMI, TEN, ECC};
#define SOLUTION_LENGTH 5

/**
 * A combination to play a hint in reverse (!) order
 */
const __flash uint8_t HINT_COMBINATION[5] = {TEN, SMI, WHI, TEN, CAP};
#define HINT_LENGTH 5

/**
 * Minimal time that the wheel has to stay in one position for us to act
 * on it
 */
#define MIN_HOVER_TIME 1000

// MISC configuration
// -----------------------------------------------------------------------------

/**
 * How many ms to wait before checking whether sleep time should be interrupted
 */
#define INTERRUPTING_DELAY_POLL_TIME 50.0

// Constants that you probably don't have to touch
// -----------------------------------------------------------------------------

/**
 * Used in implementation of morse code
 */
char *alphabet = "**ETIANMSURWDKGOHVF*L*PJBXCYZQ**";
#define ALPHABET_LENGTH 32

// PROTOTYPES
// =============================================================================

/**
 * Return position of the wheel. Will be between 0 and 15
 */
uint8_t get_wheel_pos(void);

/**
 * Sleep for this many ms. When the wheel position changes to on_position, the
 * sleep is interrupted.
 */
void interrupting_delay(float ms, int8_t on_position);

/**
 * Beep with frequency in infinite loop.
 */
void beep_forever(float freq);

/**
 * Beep with specified frequency and duration. When wheel position changes,
 * interrupt.
 */
void beep(float freq, float duration, int8_t on_position);

/**
 * Sound played after boot
 */
void play_boot_sound(void);

/**
 * Sound played when player tries to access song positions
 * with a wrong code entered before.
 */
void play_fail_sound(int8_t on_position);

/**
 * Play song given as 2d array length x 2. The two values correspond to
 * frequency and duration. A negative duration marks the end of the song.
 */
void play_song(const __flash float song[][2], int8_t on_position);

// Pre defined songs
void play_hint(int8_t on_position);
void play_song0(int8_t on_position);
void play_song1(int8_t on_position);
void play_song2(int8_t on_position);

/**
 * Play audio based on the history of wheel positions
 */
void play_audio(enum position *history);

/**
 * Morse pre-defined message
 */
void morse_message(uint8_t message_id, int8_t on_position);

/**
 * Helper function for the implementation of morse_char
 */
void _morse_char(uint8_t decimal, int8_t on_position);

/**
 * Morse single char
 */
void morse_char(char c, int8_t on_position);

/**
 * Push a wheel position to the history of wheel positions. The new position
 * will be at the start of the array.
 */
void push_history(enum position *history, uint8_t value);

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
 * Used for debugging: Transmit number with beeps. Number is then given
 * by number of long beeps * BEEP_NUMBER_LONG_NUMBER + number of short beeps.
 */
void beep_number(uint8_t number);

/**
 * Used for debugging: Transmit history with beeps.
 */
void beep_history(enum position *history);

// MAIN
// =============================================================================

uint8_t main(void) {
  initialize_ports();
  play_boot_sound();

  uint8_t last_position = get_wheel_pos();
  enum position history[HISTORY_LENGTH] = {0, 0, 0, 0, 0, 0};

  // Wait for first wheel change to start riddle
  while (get_wheel_pos() == last_position) {
  }

  uint8_t current_pos;
  uint16_t hover_ms_count;
  while (1) {
    current_pos = get_wheel_pos();

    if (current_pos != last_position) {
      // this is done unconditionally and for all wheel positions
      // to allow replaying hints by moving back and forth quickly
      last_position = current_pos;

      // only register even positions
      if (current_pos % 2 == 0) {
        // wait before registering the new position
        hover_ms_count = 0;
        for (; hover_ms_count < MIN_HOVER_TIME; hover_ms_count++) {
          _delay_ms(1.0);
          // cancel if we change wheel while waiting
          if (get_wheel_pos() != current_pos) {
            break;
          }
        }
        // register new position and give audio feedback
        if (hover_ms_count == MIN_HOVER_TIME) {
#ifdef USE_LOCKED_IN_BEEPS
          // We will get either a song (correct solution) or a fail sound
          // (otherwise) so we save ourselves the lock in beep to not take away
          // from the songs.
          if (current_pos < FIRST_SONG_POSITION) {
            beep(LOCKED_IN_FREQ, LOCKED_IN_DUR, current_pos);
            interrupting_delay(LOCKED_IN_BREAK, current_pos);
          }
#endif
          // allow playing different songs once the riddle
          // has been solved: If we move from a song position
          // to a song position, the (potentially solved)
          // history is preserved.
          if (history[0] >= FIRST_SONG_POSITION &&
              current_pos >= FIRST_SONG_POSITION)
            history[0] = current_pos;
          else
            push_history(history, current_pos);

          play_audio(history);
        }
      }
    }
  }
  return 0;
}

// IMPLEMENTATIONS
// =============================================================================

void initialize_ports(void) {
  // Speaker pin is output
  DDR(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
  // Wheel pins are input
  DDR(WHEEL_PORT) &= ~WHEEL_MASK;
  // Wheel pins use internal pull up
  PORT(WHEEL_PORT) |= WHEEL_MASK;
}

void beep_number(uint8_t number) {
  uint8_t n_beeps = number / BEEP_NUMBER_LONG_NUMBER;
  uint8_t i;
  for (i = 0; i < n_beeps; i++) {
    beep(MORSE_FREQ, MORSE_DASH_DUR, -1);
    _delay_ms(MORSE_SHORT_GAP);
  }
  n_beeps = number % BEEP_NUMBER_LONG_NUMBER;
  for (i = 0; i < n_beeps; i++) {
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
// WARNING: When you change this implementation you might have recalibrate
// DELAY_OVEREAD_US.
void beep(float freq, float duration, int8_t on_position) {

  // low frequency beeps are seen as silence
  if (freq < 1) {
    interrupting_delay(duration, on_position);
    return;
  }

  float wavelength = (1 / freq) * 1000;
  uint32_t cycles = (uint32_t)duration / wavelength;
  double half_period_us = (double)1000 * wavelength / 2;
  // Compensate for the time that it takes to toggle pins, load variables, etc.
  double compensated_waiting_time = half_period_us - DELAY_OVERHEAD_US / 2;

  if (compensated_waiting_time < 0)
    compensated_waiting_time = 0;

  for (uint32_t i = 0; i < cycles; i++) {
    _delay_us(compensated_waiting_time);
    PORT(SPEAKER_PORT) |= (1 << SPEAKER_PIN);
    _delay_us(compensated_waiting_time);
    PORT(SPEAKER_PORT) &= ~(1 << SPEAKER_PIN);

    // abort if wheel position has changed
    // we only check this every 100 times because loading variables and
    // executing functions might be expensive
    if (i % 2000 == 0 && on_position >= 0 && get_wheel_pos() != on_position)
      return;
  }

  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void _morse_char(uint8_t decimal, int8_t on_position) {
  if (decimal) {
    if (on_position != get_wheel_pos())
      return;
    _morse_char(decimal / 2, on_position);
    interrupting_delay(MORSE_DOT_DUR, on_position);
    if (decimal != 1) {
      if (decimal % 2)
        beep(MORSE_FREQ, MORSE_DASH_DUR, on_position);
      else
        beep(MORSE_FREQ, MORSE_DOT_DUR, on_position);
    }
    interrupting_delay(MORSE_DOT_DUR, on_position);
  }
}

void interrupting_delay(float duration, int8_t on_position) {
  // Only check whether to abort every 50 ms to avoid distortion of time due to
  // time it takes to check variables.
  while (duration > INTERRUPTING_DELAY_POLL_TIME) {
    _delay_ms(INTERRUPTING_DELAY_POLL_TIME);
    duration -= INTERRUPTING_DELAY_POLL_TIME;
    if (on_position >= 0 && get_wheel_pos() != on_position)
      return;
  }
  // wait leftover duration
  _delay_ms(duration);
  return;
}

// Logic from https://www.pocketmagic.net/morse-encoder/
void morse_char(char c, int8_t on_position) {
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
  _morse_char(i, on_position);
}

void play_boot_sound(void) { beep(BOOT_SOUND_FREQ, BOOT_SOUND_DUR, -1); }

void play_fail_sound(int8_t on_position) {
  beep(FAIL_SOUND_FREQ, FAIL_SOUND_DUR, on_position);
}

void play_audio(enum position *history) {
  // check if combination for hint was entered
  for (uint8_t k = 0; k < HINT_LENGTH; k++) {
    if (history[k] != HINT_COMBINATION[k])
      break;

    if (k == HINT_LENGTH - 1) {
      play_hint(history[0]);
      return;
    }
  }

  // are we on a "riddle" position? (hardcoded)
  if (history[0] < FIRST_SONG_POSITION) {
    morse_message(history[0], history[0]);
    return;
  }

  // If we are at this point we are >= FIRST_SONG_POSITION
  // check if solution was entered before this last position
  for (uint8_t k = 1; k < SOLUTION_LENGTH + 1; k++) {
    if (history[k] != SOLUTION[k - 1]) {
      play_fail_sound(history[0]);
#ifdef BEEP_HISTORY_ON_FAILURE
      beep_history(history);
#endif
      return;
    }
    if (k == SOLUTION_LENGTH) {
      // Correct solution was entered. Now check whether we should play a song
      switch (history[0]) {
      case SONG0:
        play_song0(history[0]);
        break;
      case SONG1:
        play_song1(history[0]);
        break;
      case SONG2:
        play_song2(history[0]);
        break;
      default:
        play_fail_sound(history[0]);
        interrupting_delay(100, history[0]);
        play_fail_sound(history[0]);
      }
    }
  }
}

void push_history(enum position *history, uint8_t value) {
  for (uint8_t k = HISTORY_LENGTH - 1; k > 0; k--)
    history[k] = history[k - 1];

  history[0] = value;
}

void beep_history(enum position *history) {
  for (uint8_t k = 0; k < HISTORY_LENGTH; k++) {
    beep_number(history[k]);
    _delay_ms(MORSE_MEDIUM_GAP);
  }
}

void play_song(const __flash float song[][2], int8_t on_position) {
  for (uint8_t k = 0; song[k][1] >= 0; k++) {
    if (on_position >= 0 && get_wheel_pos() != on_position) {
      return;
    }
    beep(song[k][0], song[k][1], on_position);
  }
}

void morse_message(uint8_t message_id, int8_t on_position) {
  for (uint8_t m = 0; RIDDLE_MESSAGES[message_id][m]; m++) {
    if (on_position != get_wheel_pos())
      return;
    morse_char(RIDDLE_MESSAGES[message_id][m], on_position);
    interrupting_delay(MORSE_SHORT_GAP, on_position);
    if (on_position >= 0 && get_wheel_pos() != on_position) {
      return;
    }
  }
}

uint8_t get_wheel_pos(void) {
  return (~PIN(WHEEL_PORT) & WHEEL_MASK) >> WHEEL_BIT_SHIFT_RIGHT;
}

// Functions playing hardcoded songs.
// The song arrays correspond to the csv files in data/
// The tuples correspond to frequency in Hz, duration in ms
// Zero frequency marks pauses.
// A negative duration marks the end of the song.

void play_hint(int8_t on_position) {
  static const __flash float hint[37][2] = {
      {261.624, 250},  {261.624, 20},   {280.111, 20},   {299.897, 20},
      {321.086, 20},   {343.769, 20},   {368.057, 20},   {394.057, 20},
      {421.899, 20},   {451.707, 20},   {483.618, 20},   {517.786, 20},
      {554.364, 20},   {554.364, 500},  {523.253, 3000}, {622.254, 250},
      {622.254, 20},   {569.112, 20},   {520.509, 20},   {476.059, 20},
      {435.402, 20},   {398.220, 20},   {364.212, 20},   {333.106, 20},
      {304.659, 20},   {278.644, 20},   {254.843, 20},   {233.082, 20},
      {233.082, 500},  {261.624, 4000}, {523.253, 750},  {415.304, 250},
      {261.624, 1000}, {311.127, 750},  {277.182, 250},  {261.624, 1000},
      {0.0, -1.0}};
  play_song(hint, on_position);
}

void play_song0(int8_t on_position) {
  static const __flash float song0[51][2] = {
      {311.129, 190}, {0.000, 10}, {311.129, 190}, {0.000, 10},
      {349.229, 390}, {0.000, 10}, {311.129, 390}, {0.000, 10},
      {415.305, 390}, {0.000, 10}, {391.996, 790}, {0.000, 10},
      {311.129, 190}, {0.000, 10}, {311.129, 190}, {0.000, 10},
      {349.229, 390}, {0.000, 10}, {311.129, 390}, {0.000, 10},
      {466.165, 390}, {0.000, 10}, {415.305, 790}, {0.000, 10},
      {311.129, 190}, {0.000, 10}, {311.129, 190}, {0.000, 10},
      {622.254, 390}, {0.000, 10}, {523.251, 390}, {0.000, 10},
      {415.305, 390}, {0.000, 10}, {391.996, 390}, {0.000, 10},
      {349.229, 390}, {0.000, 10}, {554.366, 190}, {0.000, 10},
      {554.366, 190}, {0.000, 10}, {523.251, 390}, {0.000, 10},
      {415.305, 390}, {0.000, 10}, {466.165, 390}, {0.000, 10},
      {415.305, 790}, {0.000, 10}, {0.0, -1.0}};

  play_song(song0, on_position);
}
void play_song1(int8_t on_position) {
  static const __flash float song1[51][2] = {
      {440.000, 390}, {0.000, 10}, {587.330, 390}, {0.000, 10},
      {554.370, 390}, {0.000, 10}, {493.880, 190}, {0.000, 10},
      {554.370, 190}, {0.000, 10}, {587.330, 390}, {0.000, 10},
      {440.000, 390}, {0.000, 10}, {440.000, 390}, {0.000, 10},
      {493.880, 390}, {0.000, 10}, {440.000, 390}, {0.000, 10},
      {392.000, 390}, {0.000, 10}, {369.990, 390}, {0.000, 10},
      {440.000, 390}, {0.000, 10}, {369.990, 390}, {0.000, 10},
      {392.000, 390}, {0.000, 10}, {440.000, 390}, {0.000, 10},
      {493.880, 390}, {0.000, 10}, {440.000, 390}, {0.000, 10},
      {369.990, 390}, {0.000, 10}, {440.000, 390}, {0.000, 10},
      {392.000, 190}, {0.000, 10}, {369.990, 190}, {0.000, 10},
      {329.630, 390}, {0.000, 10}, {329.630, 390}, {0.000, 10},
      {293.660, 790}, {0.000, 10}, {0.0, -1.0}};
  play_song(song1, on_position);
}

void play_song2(int8_t on_position) {
  static const __flash float song2[93][2] = {
      {369.994, 190}, {0.000, 10}, {369.994, 190},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {466.168, 390},  {0.000, 10},
      {466.168, 190}, {0.000, 10}, {466.168, 190},  {0.000, 10},
      {466.168, 190}, {0.000, 10}, {369.994, 190},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {466.168, 190},  {0.000, 10},
      {493.884, 590}, {0.000, 10}, {493.884, 190},  {0.000, 10},
      {493.884, 190}, {0.000, 10}, {415.302, 190},  {0.000, 10},
      {466.168, 190}, {0.000, 10}, {493.884, 190},  {0.000, 10},
      {554.369, 390}, {0.000, 10}, {554.369, 190},  {0.000, 10},
      {554.369, 190}, {0.000, 10}, {554.369, 190},  {0.000, 10},
      {554.369, 190}, {0.000, 10}, {554.369, 190},  {0.000, 10},
      {622.255, 190}, {0.000, 10}, {493.884, 390},  {0.000, 10},
      {493.884, 190}, {0.000, 10}, {493.884, 190},  {0.000, 10},
      {493.884, 190}, {0.000, 10}, {466.168, 190},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {369.994, 190},  {0.000, 10},
      {466.168, 390}, {0.000, 10}, {466.168, 190},  {0.000, 10},
      {466.168, 190}, {0.000, 10}, {493.884, 190},  {0.000, 10},
      {466.168, 190}, {0.000, 10}, {415.302, 190},  {0.000, 10},
      {369.994, 190}, {0.000, 10}, {415.302, 390},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {415.302, 190},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {415.302, 190},  {0.000, 10},
      {415.302, 190}, {0.000, 10}, {466.168, 190},  {0.000, 10},
      {415.302, 390}, {0.000, 10}, {369.994, 1190}, {0.000, 10},
      {0.0, -1.0}};
  play_song(song2, on_position);
}
