#ifndef LordLedPanel_h
#define LordLedPanel_h

extern "C" {
	#include <inttypes.h>
	#include <Math.h>
}
#include <RTClib.h>
#include <EEPROM.h>

#define tl_second 0
#define tl_minute 1
#define tl_hour 2
#define tl_day 3
#define tl_month 4
#define tl_year 5

class TinyLord{
	public:
		TinyLord();
		// configuration
		bool Position(float, float);
		bool TimeZone(int);
		bool DstRules(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
				
		//Solar & Astronomical
		bool SunRise(uint8_t *);
		bool SunSet(uint8_t *);
		
		// Utility
		uint8_t LengthOfMonth(uint8_t *);
		bool IsLeapYear(int);
		
	private:
		float latitude, longitude;
		int timezone;
		uint8_t dstm1, dstw1, dstm2, dstw2, dstadv;
		void Adjust(uint8_t *, long);
		bool ComputeSun(uint8_t *, bool);
		char Signum(int);
		int Absolute(int);
};


#define LORD_LED_PWM_TIME	0
#define LORD_LED_TEMP		1
#define LORD_LED_MARGE		2
#define LORD_LED_TZ			3
#define LORD_LED_LAT		4
#define LORD_LED_LON		5
#define LORD_LED_ENA		6

#define LORD_LED_EEPROM_LEN 18

#define LORD_LED_ON		254
#define LORD_LED_OFF	255

class LordLedPanel
{
	public:
		// Déclaration de l'objet
		LordLedPanel(int IO_Led, int IO_Fan, int ee_addr = 0);
		
		// renvoie le paramètre
		float	getValue(int type);
		// permet de modifier les valeurs par catégorie de parametrage
		void	setValue(int type, int value);
		// permet de modifier les valeurs par catégorie de parametrage
		void	setValue(int type, float value);
		// sauvegarde le parametre dans l'EEPROM
		void	saveValue(int type);

		// permet de configurer TinyLord
		void	setLord(void);		
		// lance l'analyse
		void	run(DateTime now, int temp);
		// renvoie un int signifiant le pwm
		int		getPwm(void);
		//renvoie un booléen signifiant l'etat du fan
		bool	getFan(void);
		
		// active ou desactive le timer
		void	enable(bool value = true);		
		//renvoie un entier s'il est activé
		bool	isEnable(void);
		
		// renvoie l'adresse EEPROM
		int		getEEPROM(void);
		// renvoie le prochain octet libre
		int		getNextEEPROM(void);
		// modifier l'adresse EEPROM
		void	setEEPROM(int addr);
		//charge la conf depuis l'EEPROM
		void	loadAll(void);
		//sauvegarde la conf dans l'EEPROM
		void	saveAll(void);
		
	private:
		int		_IO_Led;
		int		_IO_Fan;
		int		_ee_addr;
		TinyLord _myLord;
		int		_dataI[4];
		int		_dataF[2];
		int		_sunrise;
		int		_sunset;
		int		_pwm;
		bool	_fan;
		bool	_isEnable;
		int		_lastDay;
		
		void	checkSun(DateTime now);
		void	runPwm(unsigned long timeSec);
		void	fanStart(void);
		void	fanStop(void);
};
#endif