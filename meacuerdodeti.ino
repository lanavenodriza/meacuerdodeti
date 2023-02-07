//https://github.com/marmilicious/FastLED_examples/blob/master/heart_pulse_blood_flowing.ino

#include "config.h"
#include <Bounce2.h>
#include <FastLED.h>
#include "AdafruitIO_WiFi.h"

#define UNIT           2


//---------------------- CONSTANTS ----------------------------------------------------

// BUTTONS
#define BOUNCE_MS     10
#define BUTTON_PIN    13

// LEDS
#define NUM_LEDS      24
#define DATA_PIN      D2
#define COLOR_ORDER  GRB
#define CHIPSET  WS2812B
#define VOLTS          5
#define MAX_MA       500

// FEEDS
#define CONNECT_EVERY 75
#if UNIT == 1
#define FEED_OUT  "meacuerdodeti.button-1"
#define FEED_IN   "meacuerdodeti.button-2"
#define WIFI_SSID WIFI_SSID_1
#define WIFI_PASS WIFI_PASS_1
#else
#define FEED_OUT  "meacuerdodeti.button-2"
#define FEED_IN   "meacuerdodeti.button-1"
#define WIFI_SSID WIFI_SSID_2 
#define WIFI_PASS WIFI_PASS_2
#endif

// INTERACTION
#define MILLIS_PER_MESSAGE  7500L
#define MAX_MILLIS         40000L

// HEARTBEAT
uint8_t pulseHue = 0;  // Blood color [hue from 0-255]
uint8_t pulseSat = 255;  // Blood staturation [0-255]
uint16_t cycleLength = 1300;  // Lover values = continuous flow, higher values = distinct pulses.
uint16_t pulseLength = 120;  // How long the pulse takes to fade out.  Higher value is longer.
uint16_t pulseOffset = 375;  // Delay before second pulse.  Higher value is more delay.



//---------------------- VARIABLES ----------------------------------------------------

// BUTTONS
Bounce2::Button button;
int pulsaciones = 0;


// LEDS
CRGB ring[NUM_LEDS];
CRGB lnnColors[] = {
  CRGB::Black,       // Apagado
  CRGB(  0, 30,  0), // Verde oscuro
  CRGB( 80, 40,  0), // Amarillo
  CRGB(110,  0, 60), // Morado nave
  CRGB(200,  200, 200), // Blanco
};

// FEEDS
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *feed_out = io.feed(FEED_OUT);
AdafruitIO_Feed *feed_in = io.feed(FEED_IN);
int connectCounter = 0;

// INTERACTION
long remainingLight = 0;

// HEARTBEAT
boolean show_heartbeat = false;



//---------------------- CODE ----------------------------------------------------

void setup() {

  initLEDs();
  setAllLEDs(lnnColors[3]);

  initSerial();

  initButton();

  initWiFi();
  setAllLEDs(lnnColors[0]);

}


void loop() {

  unsigned long loopStart = millis();

  refreshConnection();

  processButton();

  refreshLEDs();

  safeShow();
  FastLED.delay(25);

  computeRemainingTime(loopStart);

}


void safeShow() {
  FastLED.show();
  FastLED.show();
}


void handleMessage(AdafruitIO_Data *data) {
  remainingLight += MILLIS_PER_MESSAGE;
  if (remainingLight > MAX_MILLIS) {
    remainingLight = MAX_MILLIS;
  }
  show_heartbeat = true;
  //digitalWrite(LED_BUILTIN, LOW);
  //Serial.println(remainingLight);
}


int sumPulse() {
  int pulse1 = pulseWave8( millis() , cycleLength, pulseLength );
  int pulse2 = pulseWave8( millis() + pulseOffset, cycleLength, pulseLength );
  return qadd8( pulse1, pulse2 );  // Add pulses together without overflow
}


uint8_t pulseWave8(uint32_t ms, uint16_t cycleLength, uint16_t pulseLength) {
  uint16_t T = ms % cycleLength;
  if ( T > pulseLength) return 0;
  uint16_t halfPulse = pulseLength / 2;
  if (T <= halfPulse ) {
    return (T * 255) / halfPulse;  //first half = going up
  } else {
    return ((pulseLength - T) * 255) / halfPulse; //second half = going down
  }
}


void initLEDs() {
  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(ring, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MA);
  FastLED.setBrightness(100);
}


void initSerial() {
  Serial.begin(57600);
  while (! Serial);
}


void initButton() {
  button.attach(BUTTON_PIN, INPUT_PULLUP);
  button.interval(BOUNCE_MS);
  button.setPressedState(LOW);
}


void initWiFi() {
  io.connect();
  feed_in->onMessage(handleMessage);

  boolean led_status = HIGH;
  while (io.status() < AIO_CONNECTED) {
    digitalWrite(LED_BUILTIN, led_status);
    led_status = !led_status;
    Serial.print(".");
    delay(400);
  }
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println(io.statusText());

}


void refreshConnection() {
  if (connectCounter <= 0) {
    connectCounter = CONNECT_EVERY;
    io.run();
  }
  connectCounter--;
}


void processButton() {
  button.update();
  if (button.fell()) {
    pulsaciones++;
    feed_out->save(pulsaciones);
    setAllLEDs(lnnColors[4]);
    //digitalWrite(LED_BUILTIN, LOW);
  }
  if (button.rose()) {
    setAllLEDs(lnnColors[0]);
  }
}


void refreshLEDs() {
  uint8_t pulseBrightness = show_heartbeat ? sumPulse() : 0;
  for (int i = 0; i < NUM_LEDS ; i++) {
    ring[i] = CHSV(pulseHue, pulseSat, pulseBrightness);
  }
}


void computeRemainingTime(unsigned long loopStart) {
  unsigned long loopTime = millis() - loopStart;
  remainingLight -= loopTime;
  if (remainingLight < 0) {
    remainingLight = 0;
    //digitalWrite(LED_BUILTIN, HIGH);
    show_heartbeat = false;
  }
  //Serial.println(remainingLight);

}


void setAllLEDs(CRGB color) {
  for (int i = 0; i < NUM_LEDS ; i++) {
    ring[i] = color;
  }
  safeShow();
}
