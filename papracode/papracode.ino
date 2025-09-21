/*
  papracode
    - Reads battery voltage and lights up LEDs to mimic M12 battery fuel gauge
    - Reads user pot to control PWM of fan

  PCB is compatible with all 0/1/2-Series 14 pin ATtiny chips
  For chips with 2K flash (ATtiny204 & ATtiny404), the Preprocessor directive must be uncommented
*/
//#define TWOKFLASHCHIP //uncomment line when programming ATtiny204 or ATtiny214
  
// Compile with Arduino 1.8.13 or later.
// Install MegaTinyCore via the Boards Manager
// MegaTinyCore 2.3.2
// Note:bootloader not used, so many options below will not be relevent
//  Board: ATtiny 3224/1624/1614/1604/814/804/424/414/404/214/204
//  Chip or Board: Select your chip
//  Clock: 20MHz internal
//  millis()/micros() TCB0 (breaks tone() and Servo)
//  Startup Time: 8ms
//  BOD Voltage level: Any, BOD not used without bootloader
//  BOD Mode when Active/Sleeping: Any, BOD not used without bootloader
//  Save EEPROM: Any, not used without bootloader
//  UPDI/Reset Pin Function: Any, not used without bootloader 
//  Voltage Baud Correction: Closer to 5V

#ifdef MILLIS_USE_TIMERA0
#error "This sketch takes over TCA0 - please use a different timer for millis"
#endif

#ifndef MILLIS_USE_TIMERB0
#error "This sketch is written for use with TCB0 as the millis timing source"
#endif

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// BME280 I2C address is 0x76 or 0x77. Using 0x76 as an example.
#define BME280_I2C_ADDRESS 0x76

Adafruit_BME280 bme; // I2C

// Pin connections per PAPR V0.5 PCB
// PA0 - UPDI/RESET
// PA1 - ADC - BATTERY
// PA2 - ADC = POTENTIOMETER
// PA3 - TACHOMETER INPUT
// PA4 - LED4 
// PA5 - PWM - FAN (was LED1)
// PA6 - LED2
// PA7 - LED3
// PB0 - I2C SCL (was PWM)
// PB1 - I2C SDA (was Buzzer)
// PB2 - UART TX
// PB3 - UART RX

int analogBatt  = PIN_A1;  // AIN1 - 10 bit resolution on ADC 
int analogPot   = PIN_A2;  // AIN2 - Fan speed control POT with on/off switch
int tachPin     = PIN_PA3;
int led4        = PIN_PA4; //v0.5 change
//int led1        = PIN_PA5; // Sacrificed for PWM fan output
int led2        = PIN_PA6;
int led3        = PIN_PA7;
int PWMPin      = PIN_PA5; // W00 8 bit resolution with Arduino AnalogWrite (v0.3 change)
//int buzzerPin   = PIN_PB1; // Sacrificed for I2C SDA

// Battery Voltage to Fuel Gauge
// > 4.12V/Cell = 100% - 78% Battery  = 4 LEDs         => 899 > adc > 864
// > 3.95V/Cell =  77% - 55% Battery  = 3 LEDs         => 864 > adc > 816
// > 3.70V/Cell =  54% - 33% Battery  = 2 LEDs         => 816 > adc > 781
// > 3.54V/Cell =  32% - 10% Battery  = 1 LEDs         => 781 > adc > 712
// > 3.25V/Cell =  10% - 0%  Battery  = LEDS FLASHING  => 712 > adc > 618
// > 2.80V/Cell =  0%        Battery  = Shut Down

const int battADCMax  =  1023;
const int battADCFull =   899;
const int battADC78p  =   864;
const int battADC55p  =   816;
const int battADC33p  =   781;
const int battADC10p  =   712;
const int battADC0p   =   618;
const int battADCMin  =     0;


//Limits the min pot value     Voltage   Min PWM Percentage
const int minPWMfull =  50;  //11.85V      69     >77%
const int minPWM75p  =  55;  //11.10V      72     >54%
const int minPWM50p  =  61;  //10.62V      77     >33%
const int minPWM25p  =  70;  //9.75V       88     >10%
const int minPWM10p  = 126;  //8.40V      110     >0%
//These values were determined emperically by adjusting the input voltage and dialing the PWM down until motor stall

//State Machine states
const int batteryCheck = 7;
const int batteryFull  = 6;
const int battery75p   = 5;
const int battery50p   = 4;
const int battery25p   = 3;
const int battery10p   = 2;
const int batteryDead  = 1;

int batteryState = batteryCheck;

int offTime = 5; //ms
int onTime = 95;   //ms
int loopDelay = 25; //ms
int blinkCounter = 0;
const uint32_t numBatterySamples = 10;
uint32_t battery = 1023;
const uint32_t maxPot = 1023;
const uint32_t minPot = 0;
uint32_t fanPWM = 0;
uint32_t minPWM = minPWM10p;
uint32_t maxPWM = 255;
uint32_t rawADC = 0;

// Tachometer measurement
volatile unsigned long lastTachPulseTime = 0;
volatile unsigned long tachPulseInterval = 0; // Time in microseconds for one revolution
uint32_t fanRPM = 0;

// PID Controller
double pid_setpoint = 50.0; // Target pressure
double Kp = 2.0, Ki = 0.5, Kd = 1.0;
double pid_integral = 0, pid_derivative = 0, pid_last_error = 0;
unsigned long pid_last_time = 0;

// Breath Model
enum BreathState { STATE_INHALE, STATE_EXHALE };
BreathState currentBreathState = STATE_INHALE;
double ipap_pressure = 60.0; // Inhalation Positive Airway Pressure
double epap_pressure = 30.0; // Exhalation Positive Airway Pressure
double initial_pressure = 0.0; // Store initial pressure to get gauge pressure
double pressure_history[10]; // Short history of pressure readings
int pressure_history_index = 0;

// Serial Command Buffer
String serialCommand;


const int LEDFlashLoop  = 25; //decrease for 10% battery LED to blink faster

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  pinMode(tachPin, INPUT_PULLUP);
  //pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  //pinMode(buzzerPin, OUTPUT);
  pinMode(PWMPin, OUTPUT);  //PA5 - TCB0 WO

  // Configure TCB0 for PWM output on PA5
  TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;

  // Route TWI0 to alternative pins (PB0/SCL, PB1/SDA)
  // IMPORTANT: This requires hardware modification if the PCB is not designed for it.
  PORTMUX.TWISPIROUTEA |= PORTMUX_TWI0_ALT1_gc;
  Wire.begin();

  attachInterrupt(digitalPinToInterrupt(tachPin), tach_isr, FALLING);

  pid_last_time = millis();

  //digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led4, HIGH);
  //digitalWrite(buzzerPin,LOW);
  digitalWrite(PWMPin, maxPWM); //Turn on fan 100%
#ifdef TWOKFLASHCHIP
  delay(1000);
#else 
  Serial.begin(115200);
  Serial.println("Starting up");
  Serial.println("PAPRA 15MAY2021");
  Serial.println("PCB v0.5");
  Serial.println("AtTiny 0/1/2-Series 14 pin");
  Serial.println("Tetra Bio Distributed 2021");
  Serial.println("MODIFIED: I2C Pressure sensor support");

  // Initialize BME280 sensor
  if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10); // Halt
  }

  // Get initial pressure reading to use as a baseline for gauge pressure
  initial_pressure = bme.readPressure();

  for (int i = 0; i <= 4; i++) { //startup knightrider 
    //digitalWrite(led1, LOW);  delay(onTime);
    //digitalWrite(led1, HIGH);
    digitalWrite(led2, LOW);  delay(onTime);
    digitalWrite(led2, HIGH); 
    digitalWrite(led3, LOW);  delay(onTime);
    digitalWrite(led3, HIGH); 
    digitalWrite(led4, LOW);  delay(onTime); delay(onTime);
    digitalWrite(led4, HIGH); 
    digitalWrite(led3, LOW);  delay(onTime);
    digitalWrite(led3, HIGH); 
    digitalWrite(led2, LOW);  delay(onTime);
    digitalWrite(led2, HIGH); 
    //digitalWrite(led1, LOW);  delay(onTime);
    //digitalWrite(led1, HIGH); delay(offTime);
  }
#endif
}

// the loop routine runs over and over again forever:
void loop() {
  delay(25);
  checkSerialCommands();

  // --- Tachometer RPM Calculation ---
  fanRPM = calculateRPM();

  // --- Bi-level Fan Control ---

  // 1. Get the current pressure
  float pressure = getPressure();

  // 2. Update the breath model to determine the current phase of breathing
  updateBreathModel(pressure);

  // 3. Set the IPAP and EPAP pressures. Potentiometer controls IPAP.
  rawADC = analogRead(analogPot);
  // Map pot from 0-1023 to a pressure range of 40.0 to 90.0 for IPAP
  ipap_pressure = 40.0 + ( (double)rawADC / 1023.0 ) * (50.0);
  epap_pressure = 40.0; // Fixed EPAP for now

  // 4. Set the PID setpoint based on the breathing state
  if (currentBreathState == STATE_INHALE) {
    pid_setpoint = ipap_pressure;
  } else {
    pid_setpoint = epap_pressure;
  }

  // 5. Compute the new fan PWM using the PID controller
  fanPWM = computePID(pressure);

  // 6. Set the fan speed
  TCB0.CCMPH = fanPWM; // Direct register write for TCB0 PWM
  battery = battery = ( ( battery * ( numBatterySamples - 1 ) ) + analogRead(analogBatt) ) / numBatterySamples;
  switch (battery) {
    case battADC78p ... battADCMax: // Full = 78% - 100%
      if (batteryState > batteryFull) {
        batteryState = batteryFull;
        //digitalWrite(led1, LOW);  //All LEDs ON (led1 sacrificed)
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, LOW);
        minPWM = minPWMfull;
      }
      break;
    case battADC55p ... ( battADC78p - 1 ): // 75% = 55% - 77%
      if (batteryState > battery75p) {
        batteryState = battery75p;
        //digitalWrite(led1, LOW);  //3 LEDs ON (led1 sacrificed)
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, HIGH);
        minPWM = minPWM75p;
      }
      break;
    case battADC33p ... ( battADC55p - 1 ): // 50% = 33% - 54%
      if (batteryState > battery50p) {
        batteryState = battery50p;
        //digitalWrite(led1, LOW);  //2 LEDs ON (led1 sacrificed)
        digitalWrite(led2, LOW);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM50p;
      }
      break;
    case battADC10p ... ( battADC33p -1 ): // 25% = 10% - 32%
      if (batteryState > battery25p) {
        batteryState = battery25p;
        //digitalWrite(led1, LOW);  //1 LED on (led1 sacrificed)
        digitalWrite(led2, LOW); // Solid LED2
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM25p;
      }
      break;
    case battADC0p ... ( battADC10p - 1) : // 10% - Need to blink LED
      if (batteryState > battery10p) {
        batteryState = battery10p;
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        //digitalWrite(led1, LOW); // Sacrificed
        //digitalWrite(buzzerPin, HIGH); // Sacrificed
        minPWM = minPWM10p;
      }
      break;
    case battADCMin ... (battADC0p - 1 ): // Shutdown
      if (batteryState > batteryDead) {
        batteryState = batteryDead;
        //digitalWrite(led1, HIGH); //All LEDs off
        digitalWrite(led2, HIGH);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        maxPWM = 0; // turn off fan - overrides the measuremnt from above
      }
      break;
  }
  // For battery state of 0-10%, need to blink LED2 to indicate almost dead battery
  if (batteryState == battery10p){
    if (blinkCounter++ > LEDFlashLoop) {
      digitalWrite(led2, !digitalRead(led2));
      blinkCounter = 0;
    }
  }  
#ifdef TWOKFLASHCHIP

#else
  Serial.print(" Battery = ");       Serial.print(battery);
  Serial.print(" rawADC = ");        Serial.print(rawADC);
  // Can't print floats on this platform by default, so printing pressure as int
  Serial.print(" pressure = ");      Serial.print((int)pressure);
  Serial.print(" setpoint = ");      Serial.print((int)pid_setpoint);
  Serial.print(" state = ");         Serial.print(currentBreathState == STATE_INHALE ? "Inhale" : "Exhale");
  Serial.print(" pwm = ");           Serial.print(fanPWM);
  Serial.print(" rpm = ");           Serial.print(fanRPM);
  Serial.print(" Battery State = "); Serial.println(batteryState);
#endif
}

/**
 * @brief Reads the gauge pressure from the BME280 I2C sensor.
 *
 * @return float The gauge pressure in Pascals (Pa), relative to the
 *               pressure at startup.
 */
float getPressure() {
  // Return the current pressure minus the initial pressure to get gauge pressure.
  // This allows the system to measure pressure changes from ambient.
  // The PID controller and breath model will now work with small positive/negative
  // pressure values, which is much easier to tune.
  return bme.readPressure() - initial_pressure;
}

/**
 * @brief Interrupt Service Routine for the fan tachometer.
 *
 * This ISR is called on each pulse from the fan's tachometer.
 * It measures the time interval between pulses.
 * NOTE: Fans often produce 2 pulses per revolution.
 */
uint32_t calculateRPM() {
  unsigned long interval;
  unsigned long last_pulse_time;
  // Safely read the volatile variables
  noInterrupts();
  interval = tachPulseInterval;
  last_pulse_time = lastTachPulseTime;
  interrupts();

  // Check if the fan is stalled (no pulse for a long time)
  // Check for interval > 0 to avoid division by zero
  if ( (micros() - last_pulse_time > 1000000) || (interval == 0) ) {
    return 0;
  } else {
    // RPM = (1 / (interval_in_us * 1e-6)) * (1 rev / 2 pulses) * (60 s / 1 min)
    // RPM = 30,000,000 / interval
    return 30000000UL / interval;
  }
}

void tach_isr() {
  unsigned long now = micros();
  tachPulseInterval = now - lastTachPulseTime;
  lastTachPulseTime = now;
}

/**
 * @brief Computes the PID output for fan speed control.
 *
 * @param current_pressure The measured pressure from the sensor.
 * @return int The calculated PWM value for the fan.
 */
void runCalibration() {
  Serial.println(F("Entering calibration mode..."));
  Serial.println(F("PWM,RPM,Pressure"));

  // Turn off PID control during calibration
  // by setting fan speed directly.

  for (int pwm = minPWM10p; pwm <= 255; pwm += 10) {
    TCB0.CCMPH = pwm; // Direct register write for TCB0 PWM
    delay(2000); // Wait 2 seconds for system to stabilize

    uint32_t currentRPM = calculateRPM();
    float currentPressure = getPressure();

    Serial.print(pwm);
    Serial.print(F(","));
    Serial.print(currentRPM);
    Serial.print(F(","));
    Serial.println(currentPressure);
  }

  // Calibration finished, return to normal operation
  analogWrite(PWMPin, 0); // Turn off fan before resuming PID
  Serial.println(F("Calibration finished."));
}

void updateBreathModel(double current_pressure) {
  // Store current pressure in history (circular buffer)
  pressure_history[pressure_history_index] = current_pressure;

  // Get the oldest sample's index
  int oldest_index = (pressure_history_index + 1) % 10;

  // Calculate pressure slope. This is a very simple approximation.
  double slope = pressure_history[pressure_history_index] - pressure_history[oldest_index];

#if 0
  Serial.print("Slope: "); Serial.println(slope);
#endif

  pressure_history_index = (pressure_history_index + 1) % 10;

  // Define slope thresholds for switching states. These will need tuning.
  double inhale_trigger_slope = 1.5;
  double exhale_trigger_slope = -1.5;

  if (currentBreathState == STATE_INHALE) {
    // If we are inhaling, look for a negative slope to switch to exhale
    if (slope < exhale_trigger_slope) {
      currentBreathState = STATE_EXHALE;
    }
  } else { // STATE_EXHALE
    // If we are exhaling, look for a positive slope to switch to inhale
    if (slope > inhale_trigger_slope) {
      currentBreathState = STATE_INHALE;
    }
  }
}

void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialCommand.trim();
      if (serialCommand.equalsIgnoreCase("calibrate")) {
        runCalibration();
      }
      serialCommand = "";
    } else {
      serialCommand += c;
    }
  }
}

int computePID(double current_pressure) {
  unsigned long now = millis();
  double time_change = (double)(now - pid_last_time);

  // This PID implementation is time-dependent.
  // We only re-calculate if a certain amount of time has passed.
  if (time_change < 20) { // Sample time of 20ms
    return fanPWM; // Return last computed value
  }

  double error = pid_setpoint - current_pressure;

  // Integral term with anti-windup
  pid_integral += error * time_change;
  if (pid_integral > maxPWM) pid_integral = maxPWM;
  if (pid_integral < minPWM) pid_integral = minPWM;

  pid_derivative = (error - pid_last_error) / time_change;

  double output = Kp * error + Ki * pid_integral + Kd * pid_derivative;

  pid_last_error = error;
  pid_last_time = now;

  // Clamp the output to the valid PWM range
  if (output > maxPWM) output = maxPWM;
  if (output < minPWM) output = minPWM;

  return (int)output;
}
