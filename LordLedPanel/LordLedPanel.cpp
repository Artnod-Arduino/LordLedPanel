extern "C" {
	#include <inttypes.h>
	#include <Math.h>
}

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#include "LordLedPanel.h"

TinyLord::TinyLord(){
		latitude=27.0;
		longitude=-82.0;
		timezone=-300;
		DstRules(3,2,11,1, 60); // USA
}
bool TinyLord::TimeZone(int z){
	if(Absolute(z)>720) return false;
	timezone=z;
	return true;
}
bool TinyLord::Position(float lat, float lon){
	if(fabs(lon)>180.0) return false;
	if(fabs(lat)>90.0) return false;
	latitude=lat;
	longitude=lon;
	return true;
}
bool TinyLord::DstRules(uint8_t sm, uint8_t sw, uint8_t em, uint8_t ew, uint8_t adv){
	if(sm==0 || sw==0 || em==0 || ew==0) return false;
	if(sm>12 || sw>4 || em>12 || ew>4) return false;
	dstm1=sm;
	dstw1=sw;
	dstm2=em;
	dstw2=ew;
	dstadv=adv;
	return true;
}
bool TinyLord::SunRise(uint8_t * when){
	return ComputeSun(when,true);
}
bool TinyLord::SunSet(uint8_t * when){
	return ComputeSun(when,false);
}
uint8_t TinyLord::LengthOfMonth(uint8_t * when){
	uint8_t odd, mnth;
	int yr;
	
	yr=when[tl_year]+2000;
	mnth=when[tl_month];
	
	if(mnth==2){
		if(IsLeapYear(yr) ) return 29;
		return 28;
	}
	odd=(mnth & 1) == 1;
	if (mnth > 7) odd  = !odd;
	if (odd) return 31;
	return 30;
}
bool TinyLord::IsLeapYear(int yr){
	return ( (yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0);
}
//====Utility====================
// rather than import yet another library, we define sgn and abs ourselves
char TinyLord::Signum(int n){
	if(n<0) return -1;
	return 1;
}
int TinyLord::Absolute(int n){
	if(n<0) return 0-n;
	return n;
}
void TinyLord::Adjust(uint8_t * when, long offset){
	long tmp, mod, nxt;
	
	// offset is in minutes
	tmp=when[tl_minute]+offset; // minutes
	nxt=tmp/60;				// hours
	mod=Absolute(tmp) % 60;
	mod=mod*Signum(tmp)+60;
	mod %= 60;
	when[tl_minute]=mod;
	
	tmp=nxt+when[tl_hour];
	nxt=tmp/24;					// days
	mod=Absolute(tmp) % 24;
	mod=mod*Signum(tmp)+24;
	mod %= 24;
	when[tl_hour]=mod;
	
	tmp=nxt+when[tl_day];
	mod=LengthOfMonth(when);
	
	if(tmp>mod){
		tmp-=mod;
		when[tl_day]=tmp+1;
		when[tl_month]++;
	}
	if(tmp<1){
		when[tl_month]--;
		mod=LengthOfMonth(when);
		when[tl_day]=tmp+mod;
	}
	
	tmp=when[tl_year];
	if(when[tl_month]==0){
		when[tl_month]=12;
		tmp--;
	}
	if(when[tl_month]>12){
		when[tl_month]=1;
		tmp++;
	}
	tmp+=100;
	tmp %= 100;
	when[tl_year]=tmp;	
	
}
bool TinyLord::ComputeSun(uint8_t * when, bool rs) {
  uint8_t  month, day;
  float y, decl, eqt, ha, lon, lat, z;
  uint8_t a;
  int doy, minutes;
  
  month=when[tl_month]-1;
  day=when[tl_day]-1;
  lon=-longitude/57.295779513082322;
  lat=latitude/57.295779513082322;
  
  
  //approximate hour;
  a=6;
  if(rs) a=18;
  
  // approximate day of year
  y= month * 30.4375 + day  + a/24.0; // 0... 365
 
  // compute fractional year
  y *= 1.718771839885e-02; // 0... 1
  
  // compute equation of time... .43068174
  eqt=229.18 * (0.000075+0.001868*cos(y)  -0.032077*sin(y) -0.014615*cos(y*2) -0.040849*sin(y* 2) );
  
  // compute solar declination... -0.398272
  decl=0.006918-0.399912*cos(y)+0.070257*sin(y)-0.006758*cos(y*2)+0.000907*sin(y*2)-0.002697*cos(y*3)+0.00148*sin(y*3);
  
  //compute hour angle
  ha=(  cos(1.585340737228125) / (cos(lat)*cos(decl)) -tan(lat) * tan(decl)  );
  
  if(fabs(ha)>1.0){// we're in the (ant)arctic and there is no rise(or set) today!
  	return false; 
  }
  
  ha=acos(ha); 
  if(rs==false) ha=-ha;
  
  // compute minutes from midnight
  minutes=720+4*(lon-ha)*57.295779513082322-eqt;
  
  // convert from UTC back to our timezone
  minutes+= timezone;
  
  // adjust the time array by minutes
  when[tl_hour]=0;
  when[tl_minute]=0;
  when[tl_second]=0;
  Adjust(when,minutes);
	return true;
}


LordLedPanel::LordLedPanel(int IO_Led, int IO_Fan, int ee_addr)
{
	_IO_Led		= IO_Led;
	_IO_Fan		= IO_Fan;
	_ee_addr	= ee_addr;
	
	_dataI[LORD_LED_TEMP]	= 1;
	_dataI[LORD_LED_MARGE]	= 0;
	_dataI[LORD_LED_TZ]		= 2;
	_dataF[LORD_LED_LAT - LORD_LED_LAT] = 43.70;
	_dataF[LORD_LED_LON - LORD_LED_LAT] = 7.25;
	_lastDay	= 0;
	_pwm		= 0;
	_fan		= false;
	_isEnable	= false;
	
	pinMode(_IO_Led,OUTPUT);
	analogWrite(_IO_Led, _pwm);
	pinMode(IO_Fan, OUTPUT);
	digitalWrite(IO_Fan, LOW);
}
float LordLedPanel::getValue(int type)
{
	if(type < LORD_LED_LAT) return _dataI[type];
	else if(type == LORD_LED_ON) return _sunrise;
	else if(type == LORD_LED_OFF) return _sunset;	
	else return _dataF[type - LORD_LED_LAT];
}
void LordLedPanel::setValue(int type, int value)
{
	if(type < LORD_LED_LAT) _dataI[type] = value;
	else if(type == LORD_LED_ON) _sunrise = value;
	else if(type == LORD_LED_OFF) _sunset = value;	
}
void LordLedPanel::setValue(int type, float value)
{
	if((type = LORD_LED_LAT) || (type == LORD_LED_LON)) _dataF[type - LORD_LED_LAT] = value;
}
void LordLedPanel::saveValue(int type)
{
	if(type < LORD_LED_LAT)
	{
		int loc = _ee_addr + (sizeof(int) * type);
		EEPROM.put(loc, _dataI[type]);
	}
	else if((type == LORD_LED_LAT) || (type == LORD_LED_LON))
	{
		int loc = _ee_addr + (sizeof(int) * LORD_LED_LAT) + (sizeof(float) * (type - LORD_LED_LAT));
		EEPROM.put(loc, _dataF[type]);
	}
	else if(type == LORD_LED_ENA)
	{
		int loc = _ee_addr + (sizeof(int) * LORD_LED_LAT) + (sizeof(float) * (type - LORD_LED_LAT));
		EEPROM.put(loc, _isEnable);
	}
}
void LordLedPanel::setLord(void)
{
	_myLord.TimeZone(_dataI[LORD_LED_TZ] * 60);
	_myLord.Position(_dataF[LORD_LED_LAT], _dataF[LORD_LED_LON]);
}
void LordLedPanel::run(DateTime now, int temp)
{
	// ARRET D'URGENCE
	if (temp >= _dataI[LORD_LED_TEMP]) enable(false);
	else if ((temp + _dataI[LORD_LED_MARGE]) < _dataI[LORD_LED_TEMP]) enable();
	
	// LIGHT RUN
	if(_isEnable)
	{
		checkSun(now); // check for new Sun Rise/Set
		unsigned long timeSec = (unsigned long)(now.hour()) * 3600 + (now.minute()*60) + now.second(); // time in sec from 00h00:00
		runPwm(timeSec); // change pwm
	}
	else if(_pwm != 0)
	{
		_pwm = 0;
		digitalWrite(_IO_Led, _pwm);
	}
	
	// GESTION VENTILATEUR
	if (temp >= _dataI[LORD_LED_TEMP]) fanStart();
	else if ((temp + _dataI[LORD_LED_MARGE]) < _dataI[LORD_LED_TEMP])
	{
		if(_pwm > 0) fanStart();
		else fanStop();
	}
}
int LordLedPanel::getPwm(void)
{
	return _pwm;
}
bool LordLedPanel::getFan(void)
{
	return _fan;
}
void LordLedPanel::enable(bool value)
{
	if(value != _isEnable) _isEnable = value;
}
bool LordLedPanel::isEnable(void)
{
	return _isEnable;
}
int LordLedPanel::getEEPROM(void)
{
	return _ee_addr;
}
int LordLedPanel::getNextEEPROM(void)
{
	return _ee_addr + LORD_LED_EEPROM_LEN;
}
void LordLedPanel::setEEPROM(int addr)
{
	_ee_addr = addr;
}
void LordLedPanel::loadAll(void)
{
	int loc = _ee_addr;
	EEPROM.get(loc, _dataI[LORD_LED_PWM_TIME]);
	loc += sizeof(int);
	EEPROM.get(loc, _dataI[LORD_LED_TEMP]);
	loc += sizeof(int);
	EEPROM.get(loc, _dataI[LORD_LED_MARGE]);
	loc += sizeof(int);
	EEPROM.get(loc, _dataI[LORD_LED_TZ]);
	loc += sizeof(int);
	EEPROM.get(loc, _dataF[LORD_LED_LAT - LORD_LED_LAT]);
	loc += sizeof(float);
	EEPROM.get(loc, _dataF[LORD_LED_LON - LORD_LED_LAT]);
	loc += sizeof(float);
	EEPROM.get(loc, _isEnable);
	setLord();
}
void LordLedPanel::saveAll(void)
{
	int loc = _ee_addr;
	EEPROM.put(loc, _dataI[LORD_LED_PWM_TIME]);
	loc += sizeof(int);
	EEPROM.put(loc, _dataI[LORD_LED_TEMP]);
	loc += sizeof(int);
	EEPROM.put(loc, _dataI[LORD_LED_MARGE]);
	loc += sizeof(int);
	EEPROM.put(loc, _dataI[LORD_LED_TZ]);
	loc += sizeof(int);
	EEPROM.put(loc, _dataF[LORD_LED_LAT - LORD_LED_LAT]);
	loc += sizeof(float);
	EEPROM.put(loc, _dataF[LORD_LED_LON - LORD_LED_LAT]);
	loc += sizeof(float);
	EEPROM.put(loc, _isEnable);
}
void LordLedPanel::checkSun(DateTime now)
{
	if(now.day() != _lastDay)
	{
		byte sunTime[] = {0, 0, 0, now.day(), now.month(), now.year()};
		_myLord.SunRise(sunTime);
		_sunrise = sunTime[2] * 60 + sunTime[1];
		_myLord.SunSet(sunTime);
		_sunset = sunTime[2] * 60 + sunTime[1];
		_lastDay = now.day();
	}
}
void LordLedPanel::runPwm(unsigned long timeSec)
{
	float nbPerSec = 255.0 / _dataI[LORD_LED_PWM_TIME];
	unsigned long startOn = (unsigned long)(_sunrise) *60;
	unsigned long endOn = startOn + _dataI[LORD_LED_PWM_TIME];
	unsigned long startOff = (unsigned long)(_sunset) *60;
	unsigned long endOff = startOff + _dataI[LORD_LED_PWM_TIME];
	
	if(timeSec < startOn) // off
	{
		if(_pwm != 0)
		{
			_pwm = 0;
			analogWrite(_IO_Led, _pwm);
		}
	}
	else if((timeSec >= startOn)  && (timeSec <= endOn)) // switch on = pwm increment
	{
		float pwmVal = 0 + (nbPerSec * (timeSec - startOn));
		int decimal = 100 * (pwmVal - (int)(pwmVal));
		if(decimal > 50) pwmVal = (int)(pwmVal) + 1;
		else pwmVal = (int)(pwmVal);
		if((int)(pwmVal) != _pwm)
		{
			_pwm = pwmVal;
			analogWrite(_IO_Led, _pwm);
		}
	}
	else if((timeSec > endOn) && (timeSec < startOff)) // on 
	{
		if(_pwm != _dataI[LORD_LED_MARGE])
		{
			_pwm = 255;
			analogWrite(_IO_Led, _pwm);
		}
	}
	else if((timeSec >= startOff) && (timeSec <= endOff)) // switch off = pwm decrement
	{
		float pwmVal = 255 - (nbPerSec * (timeSec - startOff));
		int decimal = 100 * (pwmVal - (int)(pwmVal));
		if(decimal > 50) pwmVal = (int)(pwmVal) + 1;
		else pwmVal = (int)(pwmVal);
		if((int)(pwmVal) != _pwm)
		{
			_pwm = pwmVal;
			analogWrite(_IO_Led, _pwm);
		}
	}
	else if(timeSec > endOff) // off 
	{
		if(_pwm != 0)
		{
			_pwm = 0;
			analogWrite(_IO_Led, _pwm);
		}
	}
}
void LordLedPanel::fanStart(void)
{
	if(_fan == false)
	{
		_fan = true;
		digitalWrite(_IO_Fan, HIGH);
	}
}
void LordLedPanel::fanStop(void)
{
	if(_fan == true)
	{
		_fan = false;
		digitalWrite(_IO_Fan, LOW);
	}
}