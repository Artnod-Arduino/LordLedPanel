#include <Wire.h>
#include <RTClib.h>
#include <LordLedPanel.h>

#define LED_PIN 11
#define FAN_PIN 12


RTC_DS1307 RTC;
LordLedPanel light(LED_PIN, FAN_PIN);
int current_temp = 0;

void setup()
{
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) RTC.adjust(DateTime(__DATE__, __TIME__));
  
  light.setValue(LORD_LED_PWM_TIME, 900);
  light.setValue(LORD_LED_TEMP, 60);
  light.setValue(LORD_LED_MARGE, 15);
  light.setValue(LORD_LED_TZ, 2);
  light.setValue(LORD_LED_LAT, (float)(43.70));
  light.setValue(LORD_LED_LON, (float)(7.25));
  light.setLord();
  light.enable();
}

void loop()
{
  DateTime now = RTC.now();
  light.run(now, current_temp);
  int pwm = light.getPwm();
  bool fan = light.getFan();
}