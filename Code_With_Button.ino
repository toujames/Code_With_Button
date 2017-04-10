/*
  Senior 2: Groupe 2B
*/

// Variable Declarariton
#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft

// Libraries
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"
#include <FFT.h>

/* Connect via SPI with Backpack.
DAT pin is #6
CLK pin is #2
LAT pin is #4
*/
Adafruit_LiquidCrystal lcd(6, 2, 4);

void setup() {
 
  pinMode(9,INPUT);                 // Button
  lcd.begin(16, 2);                 // LCD Initialization       
  lcd.setBacklight(HIGH);           // LCD Backlight
  TIMSK0 = 0;                       // turn off timer0 for lower jitter
  ADCSRA = 0xe5;                    // set the adc to free running mode
  ADMUX = 0x40;                     // use adc0
  DIDR0 = 0x01;                     // turn off the digital input for adc0

  /*sample every 1ms,
  // 1kHz sampling rate
  // 4Hz bin size.
  // takes 256ms to get full set of samples.
  */

}

int last = 0;                       // Storing the Freq
int largest = 0;
int largest_obj_speed  = 0;

int toMilesPerHour(int dopplerFreq){
  /*Doppler Formula
  // Transmitting Frequency = 24.125 GHz
  // Speed of light = 299,792,458 m /s
  // 3600 sec in 1 hour
  // 1609.34 meters in 1 mile
  */
  return (dopplerFreq * 299792458.0 * 3600.0 ) / (2.0 * 24125000000.0 * 1609.0);
}

void(* resetFunc) (void) = 0;             // declare reset function @ address 0

void loop() {
    while(1) {                            // reduces jitter and starts if pin 7 is HIGH
    cli();                                // Clears the the global Interrupt flags ( no interrupts)

    for (int i = 0 ; i < 512 ; i += 2) {  // save 256 samples
      while(!(ADCSRA & 0x10) && (digitalRead(9) == HIGH));           // wait for adc to be ready and Button Press
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Receiving");
      ADCSRA = 0xf5;                      // restart adc
      byte m = ADCL;                      // fetch adc data // puts
      byte j = ADCH;                      // adc high
      int k = (j << 8) | m;               // form into an int
      k -= 0x0200;                        // form into a signed int
      k <<= 6;                            // form into a 16b signed int
      
      fft_input[i] = k;                   // put real data into even bins
      fft_input[i+1] = 0;                 // set odd bins to 0. ( for imaginary
    } // end of for

    fft_window();           // multiplies the input data by a window function to help increase the frequency resolution of the FFT data
    fft_reorder();          //reorders the FFT inputs to get them ready for the special way in which the FFT algorithm processes data
    fft_run();              // This is the main FFT function call
    fft_mag_log();          // gives the magnitude of each bin in the FFT, (square real and i and take square root)
    sei();                  // set global interrupt enable
    // find the largest bin
    int largest_index = 0;
    int largest = 0;
    for (byte i = 2; i < 52; i++) {               // iterates through each byte and if is the largest it stores it. 
      if( fft_log_out[i] > largest ) {
        largest_index = i;
        largest = fft_log_out[i];
      }
    }// end of for loop
    
    int freq = 0;
    if( largest_index == 7 ) freq = 1100;
    else freq = largest_index * 150;
  
    int obj_speed = toMilesPerHour( freq );       // calculates obj speed based on doppler freq


    // Constraints based on we want the data and updates if current obj_speed is greater than largest obj speed.
    if( largest > 150 && ( obj_speed > largest_obj_speed ) && obj_speed > 13.0) {
      last = freq;
      largest_obj_speed = obj_speed;
    }
    
    if( digitalRead(9) == LOW) {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print(last); lcd.print(" Hz");
      lcd.setCursor(0,1); lcd.print( largest_obj_speed ); lcd.print(" MPH");
    }
    largest_obj_speed = 0;
    // 8.7k is good :)

   }// end of while
}

