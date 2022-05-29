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
#define SIGNAL_FLOOR 10
#define INFO_PADDING 60

#define SAMPLING_TIME_US           20

// Defines the bounds of how many times the peak of
// the signal has to be louder than the sample average
const uint16_t CLAP_AVERAGEING_SAMLPES = 50;
const uint16_t CLAP_SEQUENCE_MIN_SAMPLES_INTERVAL = 1;
const uint16_t CLAP_SEQUENCE_MAX_SAMPLES_INTERVAL = HORIZONTAL_RESOLUTION;
const uint16_t SINGLE_CLAP_SAMPLES = 5;


uint16_t oldSignal[SIGNAL_LENGTH];
uint16_t adcBuffer[SIGNAL_LENGTH];
uint16_t oldBaseAmplitude;

uint32_t nextTime = 0;

int clapCooldown = 0;
int clapSequenceSamples = 0;
int claps = 0;
int clapSequenceSamplesCurrent = 0;

void UpdateDurationBar() 
{
    M5.Lcd.fillRect(0, VERTICAL_RESOLUTION-SIGNAL_FLOOR, HORIZONTAL_RESOLUTION, SIGNAL_FLOOR, BLUE);
    M5.Lcd.fillRect(0, VERTICAL_RESOLUTION-SIGNAL_FLOOR, clapSequenceSamples, SIGNAL_FLOOR, CYAN);
}
void setup()
{
  dacWrite(M5STACKFIRE_SPEAKER_PIN, 0); // make sure that the speaker is quiet
  M5.Lcd.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.setTextColor(GREEN);  //M5.Lcd.setTextSize(3);
  M5.Lcd.setTextSize(1);

  UpdateDurationBar();

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
    y = map(adcBuffer[n], 0, 4096, VERTICAL_RESOLUTION-SIGNAL_FLOOR, INFO_PADDING);

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

  for (int n = SIGNAL_LENGTH; n > 0; n--)
  {
    adcBuffer[n] = adcBuffer[n-1];
    if(n < CLAP_AVERAGEING_SAMLPES) 
    {
      sampleSum += adcBuffer[n];
    }
  }
  adcBuffer[0] = abs(analogRead(M5STACKFIRE_MICROPHONE_PIN) - 4096/2)*2;

  uint16_t sequenceLength = map(analogRead(M5STACKFIRE_ANGLE_SENSOR_PIN), 0, 4096, CLAP_SEQUENCE_MIN_SAMPLES_INTERVAL, CLAP_SEQUENCE_MAX_SAMPLES_INTERVAL);
  if(sequenceLength != clapSequenceSamples) 
  {
    clapSequenceSamples = sequenceLength;
    UpdateDurationBar();
  }

  uint32_t average = sampleSum / CLAP_AVERAGEING_SAMLPES * 2;

  uint16_t clapThreshold = average + 1024;
  M5.Lcd.drawLine(0,oldBaseAmplitude,HORIZONTAL_RESOLUTION, oldBaseAmplitude, BLACK);
  oldBaseAmplitude = map(clapThreshold, 0, 4096, VERTICAL_RESOLUTION-SIGNAL_FLOOR, INFO_PADDING);
  M5.Lcd.drawLine(0,oldBaseAmplitude,HORIZONTAL_RESOLUTION,oldBaseAmplitude, YELLOW);

  if(clapCooldown <= 0) 
  {
    if(adcBuffer[0] > clapThreshold) 
    {
      clapCooldown = SINGLE_CLAP_SAMPLES;
      if(clapSequenceSamplesCurrent == 0) 
      {
        M5.Lcd.fillRect(0, 0, HORIZONTAL_RESOLUTION, INFO_PADDING, BLACK);
        M5.Lcd.setCursor(20,20);

        clapSequenceSamplesCurrent = clapSequenceSamples;
        M5.Lcd.print("Starting sequence ");
        M5.Lcd.print(clapSequenceSamples);
        M5.Lcd.setCursor(20,40);
      }
      M5.Lcd.print("Clap ");
      claps++;
    }
  }
  else 
  {
    clapCooldown--;
  }

  if(clapSequenceSamplesCurrent > 0) 
  {
    clapSequenceSamplesCurrent--;
    if(clapSequenceSamplesCurrent == 0) 
    {
      M5.Lcd.fillRect(0, 0, HORIZONTAL_RESOLUTION, INFO_PADDING, BLACK);
      M5.Lcd.setCursor(20,20);
      
      M5.Lcd.print("Sequence ended ");
      M5.Lcd.print(claps);
      M5.Lcd.print(" claps total.");


      claps = 0;
    }
  }

  showSignal();
}