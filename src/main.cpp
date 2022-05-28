/************************************************************************

  M5StackFire simple oscilloscope example

  The oscilloscope has a auto trigger function to achive a stable
  visual impression when a repeating signal like a sine wave is applied.

  original version
  STM32F429 Discovery June 2018, ChrisMicro
  this version reworked for
  M5StackFire         September 2018, ChrisMicro

************************************************************************/
#include <M5Stack.h>
#include <stdio.h>

#define M5STACKFIRE_MICROPHONE_PIN 34
#define M5STACKFIRE_ANGLE_SENSOR_PIN 36
#define M5STACKFIRE_SPEAKER_PIN 25 // speaker DAC, only 8 Bit

#define HORIZONTAL_RESOLUTION 320
#define VERTICAL_RESOLUTION   240
#define POSITION_OFFSET_Y      0
#define SIGNAL_LENGTH HORIZONTAL_RESOLUTION

#define SAMPLING_TIME_US           20

// Defines the bounds of how many times the peak of
// the signal has to be louder than the sample average
const uint16_t CLAP_DEVIATION_LOWER_BOUND_PERCENT = 0;
const uint16_t CLAP_DEVIATION_UPPER_BOUND_PERCENT = 100;
const uint16_t CLAP_AVERAGEING_SAMLPES = 50;
const uint16_t CLAP_SWEEP_SAMPLES = 4;
const uint16_t CLAP_LENGTH_SAMPLES = 10;


uint16_t oldSignal[SIGNAL_LENGTH];
uint16_t adcBuffer[SIGNAL_LENGTH];

float audioLevel = 0;
float maxClapInterval = 0;
float audioBaseLevel = 0;
float audioPeak = 0;
float clapDeviationPercentage = 2.0f;

uint32_t nextTime = 0;
uint32_t clapSweep = 0;

uint32_t lastTime = 0;

void setup()
{
  dacWrite(M5STACKFIRE_SPEAKER_PIN, 0); // make sure that the speaker is quiet
  M5.Lcd.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.setTextColor(GREEN);  //M5.Lcd.setTextSize(3);
  M5.Lcd.setTextSize(1);

  for (int n = 0; n < SIGNAL_LENGTH; n++)
  {
    adcBuffer[n] = 0;
  }
  //M5.Lcd.println("analog input MIC");
  //M5.Lcd.print("sampling frequency: "); M5.Lcd.print(1000000 / SAMPLING_TIME_US); M5.Lcd.println(" Hz");
}

void showSignal()
{
  int n;

  int oldx;
  int oldy;
  int oldSig;
  int x, y;

  for (n = 0; n < SIGNAL_LENGTH; n++)
  {
    x = n;
    y = map(adcBuffer[n], 0, 4096, 0, VERTICAL_RESOLUTION);

    if (n > 0)
    {
      // delete old line element
      M5.Lcd.drawLine(oldx , oldSig, x, oldSignal[n], BLACK );
      // draw new line element
      if (n < SIGNAL_LENGTH - 1) // don't draw last element because it would generate artifacts
      {
        M5.Lcd.drawLine(oldx,    oldy, x,            y, GREEN );
      }
    }
    oldx = x;
    oldy = y;
    oldSig = oldSignal[n];
    oldSignal[n] = y;
  }
}

void loop(void)
{
  //record signal
  while (micros() < nextTime);
    nextTime = micros() + SAMPLING_TIME_US;

  uint32_t sampleSum = 0;
  uint16_t peak = 0;

  for (int n = SIGNAL_LENGTH; n > 0; n--)
  {
    adcBuffer[n] = adcBuffer[n-1];
    if(n < CLAP_AVERAGEING_SAMLPES) 
    {
      uint16_t amplitude = abs(adcBuffer[n] - (4096 / 2));
      sampleSum += amplitude;
    }
  }
  adcBuffer[0] = analogRead(M5STACKFIRE_MICROPHONE_PIN);
  peak = abs(adcBuffer[0] - (4096 / 2));


  uint32_t average = sampleSum / CLAP_AVERAGEING_SAMLPES * 2;

  float normalizedValue = 1.0f/4096;
  audioBaseLevel = average * normalizedValue;
  audioPeak = peak * normalizedValue;
  clapDeviationPercentage = map(analogRead(M5STACKFIRE_ANGLE_SENSOR_PIN), 0, 4096, CLAP_DEVIATION_LOWER_BOUND_PERCENT, CLAP_DEVIATION_UPPER_BOUND_PERCENT) / 100.0f;

  // M5.Lcd.fillRect(0, 0, HORIZONTAL_RESOLUTION, 70, BLACK);
  // M5.Lcd.setCursor(20, 20);
  // M5.Lcd.print("Base audio level: ");
  // M5.Lcd.println(audioBaseLevel);
  // M5.Lcd.setCursor(20, 40);
  // M5.Lcd.print("Sample peak: ");
  // M5.Lcd.println(audioPeak);
  // M5.Lcd.setCursor(20, 60);
  // M5.Lcd.print("Clap amplitude difference: ");
  // M5.Lcd.print(clapDeviationPercentage*100);
  // M5.Lcd.println("%");

  M5.Lcd.fillRect(0, 160, HORIZONTAL_RESOLUTION, 200, BLACK);
  M5.Lcd.setCursor(20, 160);
  M5.Lcd.print("Clap sweep: ");
  M5.Lcd.print(clapSweep);
  M5.Lcd.print("/");
  M5.Lcd.println(CLAP_LENGTH_SAMPLES);

  M5.Lcd.setCursor(20, 180);
  M5.Lcd.print("Sampling rate per second: ");
  M5.Lcd.print(1.0 / ((micros()-lastTime) / 1000000.0));
  showSignal();


  // Detect if amplitude is above the baseline by some margin
  // if(audioPeak > audioBaseLevel + clapDeviationPercentage) 
  // {
  //   if(clapSweep > CLAP_SWEEP_SAMPLES) 
  //   {
  //     clapSweep = 0;
  //     M5.Lcd.fillScreen(RED);
  //   }
  //   // Detect a new clap
  //   else if(clapSweep < 1)
  //   {
  //     M5.Lcd.fillScreen(BLUE);
  //     clapSweep = 1;
  //   }
  // }
  // // if current clap has passed the silence test
  // else if(clapSweep > CLAP_LENGTH_SAMPLES)
  // {
  //   clapSweep = 0;
  //   M5.Lcd.fillScreen(GREEN);
  // }
  // // if clap has already started then increase its sample lenght by one
  // else
  // {
  //   if(clapSweep > 0) 
  //   {
  //     clapSweep++;
  //   }
  //   M5.Lcd.fillScreen(BLACK);
  // }

  lastTime = micros();
}