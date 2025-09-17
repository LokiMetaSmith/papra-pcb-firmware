/*
  papracode for PCB v0.7.x and ATtiny1624 12SEPT2025

  - Reads battery voltage and lights up LEDs to mimic an M12 battery fuel gauge
  - Reads a user potentiometer to control the fan speed (PWM)
  - Controls a buzzer for low battery warning

  Hardware: ATtiny1624 (14-pin), MegaTinyCore, 20 MHz internal clock
  Add megaTinyCore to Boards Manager (Tested with megaTinyCore v2.6.10)
  Arduino Settings
    Add to `Additional Boards Manager` http://drazzy.com/package_drazzy.com_index.json
  IMPORTANT: In Arduino>Tools, set the following:
  Board: ATtiny3224/1624/...
  Chip: ATtiny1624
  Clock: 20MHz Internal
  millis()/micros() timer: TCB1
  Any other settings, leave as default
*/

#include <Arduino.h>

#ifdef MILLIS_USE_TIMERA0
#error "This sketch takes over TCA0 - please use a different timer for millis"
#endif


// ---------- Logic levels (for this board, LOW turns LEDs ON) ----------
#define ledOn LOW  // LEDs sink current to turn on
#define ledOff HIGH

#define buzzerOn HIGH
#define buzzerOff LOW

// ---------- Pin assignments (PCB v0.7.x) ----------
const int analogFuseIlm    = PIN_A1;   // (future use)
const int analogPot        = PIN_A2;   // Fan speed potentiometer
const int analogBatt       = PIN_A3;   // Battery voltage divider
const int pinLED1          = PIN_PA4;  // Battery "full" LED
const int pinLED2          = PIN_PA5;  // Battery "3/4" LED
const int pinLED3          = PIN_PA6;  // Battery "1/2" LED
const int pinLED4          = PIN_PA7;  // Battery "1/4" LED
const int pinFanPWM        = PIN_PB0;  // Fan PWM output (TCA0 WO0)
const int pinBuzzer        = PIN_PB1;  // Buzzer output
const int pinFusePowerGood = PIN_PB3;  // (future use)

// ---------- Battery thresholds (10-bit ADC counts) ----------
// These values are based on your voltage divider and battery chemistry
const uint16_t battADCMax  = 1023;  // ADC max value (fully charged)
const uint16_t battADCFull = 899;   // "Full" threshold
const uint16_t battADC78p  = 864;   // 78% threshold
const uint16_t battADC55p  = 816;   // 55% threshold
const uint16_t battADC33p  = 781;   // 33% threshold
const uint16_t battADC10p  = 712;   // 10% threshold
const uint16_t battADC0p   = 618;   // "Empty" threshold
const uint16_t battADCMin  = 0;     // ADC min value

// ---------- Fan PWM (TCA0 Single-slope @ 20 kHz) ----------
// The fan is controlled by a PWM signal at 20 kHz (inaudible to humans)
static constexpr uint16_t PWM_TOP = 999;  // 20 MHz / (1 * (999+1)) = 20 kHz PWM
static constexpr uint16_t PWM_MAX = PWM_TOP;

uint16_t minPWM = 126;      // Minimum allowed PWM (depends on battery state)
uint16_t maxPWM = PWM_MAX;  // Maximum allowed PWM (clamped to 0 if battery is dead)

// ---------- Battery filtering (rolling average for stable readings) ----------
const uint16_t numBatterySamples              = 10;  // Number of samples to average
uint16_t batteryADCSamples[numBatterySamples] = { 0 };
uint32_t batteryADCSum                        = 0;
uint8_t batteryADCSampleIndex                 = 0;
uint16_t batteryADCSampleAverage              = 0;

// ---------- Miscellaneous ----------
const uint8_t lowBatteryLoopCounts = 25;  // Controls blink speed for low battery LED
const uint16_t minADCValue10bit    = 0;
const uint16_t maxADCValue10bit    = 1023;

uint8_t lowBatteryBlinkCounter = 0;
uint16_t rawADC                = 0;  // Raw potentiometer reading
uint16_t fanPWM                = 0;  // Calculated PWM value for the fan

// ---------- Helper: Set all 4 LEDs at once ----------
void setLEDs( bool l1, bool l2, bool l3, bool l4 ) {
  digitalWriteFast( pinLED1, l1 );
  digitalWriteFast( pinLED2, l2 );
  digitalWriteFast( pinLED3, l3 );
  digitalWriteFast( pinLED4, l4 );
}

// ---------- Helper: Turn buzzer on or off ----------
void setBuzzer( bool on ) {
  digitalWriteFast( pinBuzzer, on ? buzzerOn : buzzerOff );
}

// ---------- Helper: Set fan PWM duty cycle (clamped to maxPWM) ----------
inline void setFan( uint16_t duty ) {
  if( duty > maxPWM )
    duty = maxPWM;          // Clamp to maxPWM (may be 0 if battery is dead)
  TCA0.SINGLE.CMP0 = duty;  // Set PWM duty cycle (0..PER)
}

// ---------- LED startup animation (just for fun) ----------
const uint8_t ledDanceOffTime = 5;   // ms (short off between steps)
const uint8_t ledDanceOnTime  = 95;  // ms (how long each LED stays on)
void ledStartupDance() {
  // Simple "chase" animation for all 4 LEDs at startup
  for( int i = 0; i <= 4; i++ ) {
    setLEDs( ledOn, ledOff, ledOff, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOff, ledOn, ledOff, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOff, ledOff, ledOn, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOff, ledOff, ledOff, ledOn );
    delay( ledDanceOnTime * 2 );
    setLEDs( ledOff, ledOff, ledOn, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOff, ledOn, ledOff, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOn, ledOff, ledOff, ledOff );
    delay( ledDanceOnTime );
    setLEDs( ledOff, ledOff, ledOff, ledOff );
    delay( ledDanceOffTime );
  }
}

// ---------- Battery state machine ----------
// This enum tracks which battery "level" we're in
enum batteryStates {
  undefined,    // Not used
  batteryDead,  // Battery is empty (fan off, buzzer on)
  battery10p,   // 10% left
  battery25p,   // 25% left
  battery50p,   // 50% left
  battery75p,   // 75% left
  batteryFull,  // 100% (full)
  batteryCheck  // Initial state (forces update on first run)
};
uint8_t batteryState = batteryCheck;

// ---------- TCA0 setup: 20 kHz PWM on WO0→PB0 (default route) ----------
inline void tca0_init_20kHz_WO0_defaultRoute() {
  // Route TCA0 WO0 output to PB0 (see datasheet Table 16-3)
  PORTMUX.TCAROUTEA = ( PORTMUX.TCAROUTEA & ~0x03 ) | 0x00;

  // Ensure SINGLE mode and disable while configuring
  TCA0.SINGLE.CTRLA = 0;
  TCA0.SPLIT.CTRLD  = 0;  // clear SPLITM → SINGLE mode

  // Single-slope PWM, enable WO0 output
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;

  // Set PWM frequency: 20 kHz at 20 MHz (DIV1, PER=999)
  TCA0.SINGLE.PER  = PWM_TOP;
  TCA0.SINGLE.CMP0 = 0;  // start at 0% (fan off)
  TCA0.SINGLE.CNT  = 0;

  // Enable timer with no prescaler (DIV1)
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}

// ---------- Arduino setup() ----------
void setup() {
  // Set up all GPIO pins
  pinMode( pinLED1, OUTPUT );
  pinMode( pinLED2, OUTPUT );
  pinMode( pinLED3, OUTPUT );
  pinMode( pinLED4, OUTPUT );
  pinMode( pinFanPWM, OUTPUT );
  pinMode( pinBuzzer, OUTPUT );
  pinMode( pinFusePowerGood, INPUT );

  setLEDs( ledOff, ledOff, ledOff, ledOff );  // All LEDs off at startup
  setBuzzer( false );                         // Buzzer off at startup

  // Configure analog inputs
  analogReference( VDD );      // Use VDD as ADC reference
  analogReadResolution( 10 );  // 10-bit ADC (0..1023)
  analogSampleDuration( 64 );  // Sample duration (increase if voltage divider is high impedance)

  // Initialize battery ADC rolling average with first reading
  uint16_t firstReading = analogRead( analogBatt );
  for( uint8_t i = 0; i < numBatterySamples; ++i ) {
    batteryADCSamples[i] = firstReading;
    batteryADCSum += firstReading;
  }
  batteryADCSampleAverage = firstReading;

  // Set up PWM: 20 kHz on PB0 (WO0) using SINGLE mode
  tca0_init_20kHz_WO0_defaultRoute();
  // Startup beep and LED animation
  setBuzzer( true );
  delay( 100 );
  setBuzzer( false );
  setFan( maxPWM );  // Start with fan at max speed
  ledStartupDance();
}

// ---------- Arduino loop() ----------
void loop() {
  delay( 25 );  // Slow down the loop a bit (not critical)

  // --- Read potentiometer and set fan speed ---
  // The potentiometer sets the fan speed between minPWM and maxPWM (which may be 0 if battery is dead)
  rawADC = analogRead( analogPot );
  fanPWM = map( rawADC, minADCValue10bit, maxADCValue10bit, minPWM, maxPWM );
  setFan( fanPWM );

  // --- Rolling average on battery ADC for stable readings ---
  uint16_t newReading = analogRead( analogBatt );
  batteryADCSum -= batteryADCSamples[batteryADCSampleIndex];
  batteryADCSamples[batteryADCSampleIndex] = newReading;
  batteryADCSum += newReading;
  batteryADCSampleIndex   = ( batteryADCSampleIndex + 1 ) % numBatterySamples;
  batteryADCSampleAverage = batteryADCSum / numBatterySamples;

  // --- Buzzer pulse timing (for 10% battery warning) ---
  static unsigned long lastBuzzerMillis = 0;
  static bool buzzerPulseState          = false;

  // --- Battery state machine ---
  // This logic "ratchets" downward: once the battery drops to a lower state, it never goes back up
  switch( batteryADCSampleAverage ) {
    case battADC78p ... battADCMax:  // Full
      if( batteryState > batteryFull || batteryState == batteryCheck ) {
        batteryState = batteryFull;
        setLEDs( ledOn, ledOff, ledOff, ledOff );  // Only LED1 on
        minPWM = 50;                               // Allow lowest fan speed
      }
      setBuzzer( false );
      break;

    case battADC55p ...( battADC78p - 1 ):  // 75%
      if( batteryState > battery75p || batteryState == batteryCheck ) {
        batteryState = battery75p;
        setLEDs( ledOff, ledOn, ledOff, ledOff );  // Only LED2 on
        minPWM = 55;
      }
      setBuzzer( false );
      break;

    case battADC33p ...( battADC55p - 1 ):  // 50%
      if( batteryState > battery50p || batteryState == batteryCheck ) {
        batteryState = battery50p;
        setLEDs( ledOff, ledOff, ledOn, ledOff );  // Only LED3 on
        minPWM = 61;
      }
      setBuzzer( false );
      break;

    case battADC10p ...( battADC33p - 1 ):  // 25%
      if( batteryState > battery25p || batteryState == batteryCheck ) {
        batteryState = battery25p;
        setLEDs( ledOff, ledOff, ledOff, ledOn );  // Only LED4 on
        minPWM = 70;
      }
      setBuzzer( false );
      break;

    case battADC0p ...( battADC10p - 1 ):  // 10% → blink LED4, 1 Hz beep 10% duty
      if( batteryState > battery10p || batteryState == batteryCheck ) {
        batteryState = battery10p;
      }
      {
        // Blink LED4 to indicate low battery
        bool led4State = digitalReadFast( pinLED4 );  // <-- was pinLED1
        if( lowBatteryBlinkCounter++ > lowBatteryLoopCounts ) {
          led4State              = !led4State;
          lowBatteryBlinkCounter = 0;
        }
        setLEDs( ledOff, ledOff, ledOff, led4State );  // Only LED4 blinks
      }
      // Buzzer beeps at 1 Hz, 10% duty cycle
      if( millis() - lastBuzzerMillis >= 1000 ) {
        lastBuzzerMillis = millis();
        buzzerPulseState = true;
        setBuzzer( true );
      }
      else if( buzzerPulseState && millis() - lastBuzzerMillis >= 100 ) {
        buzzerPulseState = false;
        setBuzzer( false );
      }
      minPWM = 126;
      break;

    case battADCMin ...( battADC0p - 1 ):  // Dead → fan off, buzzer solid
      if( batteryState > batteryDead || batteryState == batteryCheck ) {
        batteryState = batteryDead;
        setLEDs( ledOff, ledOff, ledOff, ledOff );  // All LEDs off
        maxPWM = 0;                                 // Clamp fan off
        setFan( 0 );
      }
      setBuzzer( true );  // Solid buzzer
      buzzerPulseState = false;
      break;

    default:
      setBuzzer( false );
      buzzerPulseState = false;
      break;
  }
}
