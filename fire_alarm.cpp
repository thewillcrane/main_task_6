//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "fire_alarm.h"

#include "siren.h"
#include "strobe_light.h"
#include "user_interface.h"
#include "code.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"

//=====[Declaration of private defines]========================================

#define TEMPERATURE_C_LIMIT_ALARM     30.0
#define STROBE_TIME_GAS               1000
#define STROBE_TIME_OVER_TEMP          500
#define STROBE_TIME_GAS_AND_OVER_TEMP  100

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalIn alarmTestButton(BUTTON1);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

bool gasDetected                  = OFF;
bool overTemperatureDetected      = OFF;
bool gasDetectorState             = OFF;
bool overTemperatureDetectorState = OFF;

//=====[Declarations (prototypes) of private functions]========================

static int fireAlarmStrobeTime();

//=====[Implementations of public functions]===================================

void fireAlarmInit(void)
{
    overTemperatureDetectorState = OFF;
    gasDetectorState = OFF;
    overTemperatureDetected = OFF;
    gasDetected = OFF;

    temperatureSensorInit();
    gasSensorInit();
    sirenInit();
    strobeLightInit();    
    
    alarmTestButton.mode(PullDown); 
}

void fireAlarmUpdate()
{
    fireAlarmActivationUpdate();
    // No siren or strobe control here; handled by user interface
} 

bool gasDetectorStateRead()
{
    return gasDetectorState;
}

bool overTemperatureDetectorStateRead()
{
    return overTemperatureDetectorState;
}

bool gasDetectedRead()
{
    return gasDetected;
}

bool overTemperatureDetectedRead()
{
    return overTemperatureDetected;
}

//=====[Implementations of private functions]==================================

void fireAlarmActivationUpdate()
{
    temperatureSensorUpdate();
    gasSensorUpdate();

    overTemperatureDetectorState = temperatureSensorReadCelsius() > 
                                   TEMPERATURE_C_LIMIT_ALARM;

    if ( overTemperatureDetectorState ) {
        overTemperatureDetected = ON;
    }else {
        overTemperatureDetected = OFF;
    }

    if ( gasSensorRead() ) {
        gasDetected = ON;
        gasDetectorState = ON;
    }else {
        gasDetected = OFF;
        gasDetectorState = OFF;
    }
    
    if( alarmTestButton ) {             
        overTemperatureDetected = ON;
        gasDetected = ON;
    }
}

static int fireAlarmStrobeTime()
{
    if( gasDetectedRead() && overTemperatureDetectedRead() ) {
        return STROBE_TIME_GAS_AND_OVER_TEMP;
    } else if ( gasDetectedRead() ) {
        return STROBE_TIME_GAS;
    } else if ( overTemperatureDetectedRead() ) {
        return STROBE_TIME_OVER_TEMP;
    } else {
        return 0;
    }
}
