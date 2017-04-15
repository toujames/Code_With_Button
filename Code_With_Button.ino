/*
 * Reads in FFT Max data when button pressed
 * Outputs the speed when button is released
 * Author: James Touthang
*/

// Definitions
#define LOG_OUT 1         // use the log output function
#define FHT_N 256         // set to 256 point fht

// Liraries
#include <FHT.h>          // include the library
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

Adafruit_LiquidCrystal lcd(6, 2, 4);

// Optimizing Variables for better reading
int max_mag = 180;                        // magnitude of bin threshold 
int bin_multiplier = 146.05;              // samplerate/fht. bin multiplier
double highest_freq = 0;                             // Storing the highest Frequency

int buttonState = HIGH;                  // button state is initialized as "pressed"

// Arrow Character
byte arrow1[8] = { B10000, B11000, B11100, 
                   B11110, B11100, B11000, 
                   B10000,B00000 };

double toMilesPerHour(double dopplerFreq){
  /*Doppler Formula
  // Transmitting Frequency = 24.125 GHz
  // Speed of light = 299,792,458 m /s
  // 3600 sec in 1 hour
  // 1609.34 meters in 1 mile
  */
  return (dopplerFreq * 299792458.0 * 3600.0 ) / (2.0 * 24125000000.0 * 1609.34);
}

// method to get max of two values.
int maxValue(int x, int y){
  if(x >= y) return x;
  return y;
}

// method to account for the bouncing during the button press.
boolean debounceButton(boolean state){
  boolean stateNow = digitalRead(9);
  if(state!=stateNow){
    delay(10);                                    // waits 10 ms to to account for the bouncing
    stateNow = digitalRead(9);
  }
  return stateNow;
}

void setup() {
  Serial.begin(115200); // use the serial port
  pinMode(9,INPUT);                 // Button
  lcd.begin(16, 2);                 // LCD Initialization       
  lcd.setBacklight(HIGH);           // LCD Backlight
  lcd.createChar(1, arrow1);
  TIMSK0 = 0;                       // turn off timer0 for lower jitter
  ADCSRA = 0xe5;                    // set the adc to free running mode
  ADMUX = 0x40;                     // use adc0
  DIDR0 = 0x01;                     // turn off the digital input for adc0
}

void loop() {
 while(1) {                         // reduces jitter
    cli();                          // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
      while(!(ADCSRA & 0x10) );     // wait for adc to be ready
      ADCSRA = 0xf5;                // restart adc
      byte m = ADCL;                // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m;         // form into an int
      k -= 0x0200;                  // form into a signed int
      k <<= 6;                      // form into a 16b signed int
      fht_input[i] = k;             // put real data into bins
    }

    /*
     * fht_log_out[i], 
     *    i is the frequency bin
     *    fht_log_out[i] is the magnitied
     * ffrequency: f(i) = i * sample_rate / FHT_N
     * sample_rate = 16 Mhz / some prescaler 
     */
    fht_window();       // window the data for better frequency response
    fht_reorder();      // reorder the data before doing the fht
    fht_run();          // process the data in the fht
    fht_mag_log();      // take the output of the fht
    sei();              // sets back interrupt

    // Writes to Serial to shue graphic spetrum analyzer
    Serial.write(255);                      // send a start byte
    Serial.write(fht_log_out, FHT_N/2);     // send out the data

    
    int largest_index = 0;
    int last = 0;
    for (byte i = 2; i < FHT_N/2; i++) {    // iterates through each bin and if is the largest it stores it. 
      if( fht_log_out[i] >= max_mag) {
        largest_index = i;
      }
    }
    last = largest_index;                   // sets the last largest_index

    // takes the two values and gets max and multiplies by a multiplier to get freq
    double cur_freq = maxValue(last,largest_index) * bin_multiplier; 

    // sets the highest_freq during reading to get the maximum value
    if(cur_freq > highest_freq ) {
      highest_freq = cur_freq;
    }

    // Code for button pressed. Sets the highest_freq to 0 and display words
    if(debounceButton(buttonState)==HIGH && buttonState == LOW){
      highest_freq = 0;
      lcd.setCursor(0,0); lcd.print("Collecting       ");
      lcd.setCursor(0,1); lcd.write(1);lcd.write(1);lcd.write(1);lcd.write(1); lcd.write(1);lcd.write(1);lcd.print("       ");
      //lcd.setCursor(0,1); lcd.print(" Rec               ");
      buttonState = HIGH;
    } 
    
    // Code for when button released. prints the data to lcd
    else if ( debounceButton(buttonState)==LOW && buttonState == HIGH){
      lcd.setCursor(0,0); lcd.print(highest_freq); lcd.print(" Hz        ");
      lcd.setCursor(0,1); lcd.print(toMilesPerHour(highest_freq)); lcd.print(" MPH        ");
      //lcd.setCursor(0,1); lcd.print(highest_freq); lcd.print(" Hz        ");
      buttonState = LOW;
    }
 }// end of while
}
