//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "user_interface.h"

#include "code.h"
#include "siren.h"
#include "smart_home_system.h"
#include "fire_alarm.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"
#include "display.h"

//=====[Declaration of private defines]========================================

#define DISPLAY_REFRESH_TIME_MS 1000
#define KEYPAD_REFRESH_TIME_MS 500

#define ALARM_INTERVAL 10000  //(10 seconds)
#define ALARM_INTERVAL_ON_TIME 3000  //(3 seconds)

#define NUMBER_OF_KEYS                           4
#define TEMPERATURE_C_LIMIT_ALARM 30.0  // From fire_alarm.cpp

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

DigitalOut alarmLed(LED1);   //For part v.

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

static bool incorrectCodeState = OFF;
static bool systemBlockedState = OFF;

static bool codeComplete = false;
static int numberOfCodeChars = 0;

static Timer alarmUpdateTimer; // Timer for periodic alarm state updates

bool tempAlarm = false;
bool gasAlarm = false;

static bool alarmActive = false;  // New variable to track persistent alarm state

char codeSequence[NUMBER_OF_KEYS] = {'2', '0', '0', '5'};  // Correct code
char codeSequenceFromUserInterface[CODE_NUMBER_OF_KEYS];
bool incorrectCode = false;
int numberOfIncorrectCodes = 0;
bool alarmDisplayed = false;

//=====[Declarations (prototypes) of private functions]========================

static void userInterfaceMatrixKeypadUpdate();
static void incorrectCodeIndicatorUpdate();
static void systemBlockedIndicatorUpdate();

static void userInterfaceDisplayInit();
static void userInterfaceDisplayUpdate();

static void sendAlarmState();

static bool checkCodeMatch();  // New function to verify code

//=====[Implementations of public functions]===================================

void userInterfaceInit()
{
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
    alarmLed = OFF;
    matrixKeypadInit( SYSTEM_TIME_INCREMENT_MS );
    userInterfaceDisplayInit();

    // Start the alarm update timer
    alarmUpdateTimer.start();
}

void userInterfaceUpdate()
{
    userInterfaceMatrixKeypadUpdate();
    incorrectCodeIndicatorUpdate();
    systemBlockedIndicatorUpdate();
    userInterfaceDisplayUpdate();
}

bool incorrectCodeStateRead()
{
    return incorrectCodeState;
}

void incorrectCodeStateWrite( bool state )
{
    incorrectCodeState = state;
}

bool systemBlockedStateRead()
{
    return systemBlockedState;
}

void systemBlockedStateWrite( bool state )
{
    systemBlockedState = state;
}

bool userInterfaceCodeCompleteRead()
{
    return codeComplete;
}

void userInterfaceCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void userInterfaceMatrixKeypadUpdate()
{
    static int numberOfHashKeyReleased = 0;
    char keyReleased = matrixKeypadUpdate();

    if( keyReleased != '\0' ) {

        if( alarmActive && !systemBlockedStateRead() ) {
            if( !incorrectCodeStateRead() ) {
                codeSequenceFromUserInterface[numberOfCodeChars] = keyReleased;
                numberOfCodeChars++;
                if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
                    codeComplete = true;
                    numberOfCodeChars = 0;
                    if ( checkCodeMatch()){
                        alarmActive = false;
                        sirenStateWrite(OFF);
                        alarmLed = OFF;
                        alarmDisplayed = false;
                        displayCharPositionWrite(0, 0);
                        displayStringWrite("                    ");
                        displayCharPositionWrite(0, 1);
                        displayStringWrite("                    ");
                        displayCharPositionWrite(0, 2);
                        displayStringWrite("                    ");
                        displayCharPositionWrite(0, 3);
                        displayStringWrite("                    ");
                        userInterfaceDisplayInit();
                    }else{
                        incorrectCodeState = ON;
                        numberOfIncorrectCodes++;
                        if (numberOfIncorrectCodes >= 3) {
                            systemBlockedState = ON;
                        }
                    }
                }
            } else {
                if( keyReleased == '#' ) {
                    numberOfHashKeyReleased++;
                    if( numberOfHashKeyReleased >= 2 ) {
                        numberOfHashKeyReleased = 0;
                        numberOfCodeChars = 0;
                        codeComplete = false;
                        incorrectCodeState = OFF;
                    }
                }
            }
        }
    }
}

static void userInterfaceDisplayInit()
{
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
     
    displayCharPositionWrite ( 0,0 );       //These three display the 
    displayStringWrite( "Temperature:" );   //headings of the various 
                                            //readings for part 3
    displayCharPositionWrite ( 0,1 );
    displayStringWrite( "Gas:" );
    
    displayCharPositionWrite ( 0,2 );
    displayStringWrite( "Alarm:" );
}

static void userInterfaceDisplayUpdate()
{
    static int accumulatedDisplayTime = 0;
    char temperatureString[3] = "";

    
    if( accumulatedDisplayTime >=   
        DISPLAY_REFRESH_TIME_MS ) {  //The three variables update every second
                                     //This shows continous data sent to LCD
    accumulatedDisplayTime = 0;
    if (!alarmActive){
            //Part iii.
        //Show the temperature reading
        sprintf(temperatureString, "%.0f", temperatureSensorReadCelsius());
        displayCharPositionWrite ( 12,0 );
        displayStringWrite( temperatureString );
        displayCharPositionWrite ( 14,0 );
        displayStringWrite( "'C" );
        //Part iv.
        if(temperatureSensorReadCelsius() >= TEMPERATURE_C_LIMIT_ALARM){
            tempAlarm = true;
            if (!alarmActive){
                alarmActive = true;
                sirenStateWrite(ON);
                alarmLed = ON;
            }
        }else{
            tempAlarm = false;
        }

        displayCharPositionWrite ( 4,1 );
        //Show gas state depending on if gas is found
        if ( gasSensorRead() ) {  
            displayStringWrite( "Detected    " );
            //Part iv.
            gasAlarm = true;
            if (!alarmActive){
                alarmActive = true;
                sirenStateWrite(ON);
                alarmLed = ON;
            }
        } else {
            displayStringWrite( "Not Detected" );
            gasAlarm = false;
        }
      
        displayCharPositionWrite ( 6,2 );
        //Show alarm state, if alarm is on or off
        if ( sirenStateRead() ) { 
            displayStringWrite( "ON " );
        } else {
            displayStringWrite( "OFF" );
        }
    }

        //Part v.
     if(alarmActive && !alarmDisplayed){
         displayCharPositionWrite( 0, 0 );
         displayStringWrite("                    ");
         displayCharPositionWrite( 0, 1 );
         displayStringWrite("                    ");
         displayCharPositionWrite( 0, 2 );
         displayStringWrite("                    ");
         displayCharPositionWrite( 0, 0 );
         displayStringWrite("ALARM IS ACTIVATED  ");
         displayCharPositionWrite( 0, 1 );
         if (gasAlarm){
             displayStringWrite("Gas is detected     ");
         }else{
             displayStringWrite("Temp. is too high   ");
         }
         displayCharPositionWrite( 0, 2 );
         displayStringWrite("Enter a code to     ");
         displayCharPositionWrite( 0, 3 );
         displayStringWrite("deactivate alarm:   ");
         alarmDisplayed = true;
     }   
    } else {
        accumulatedDisplayTime =
            accumulatedDisplayTime + SYSTEM_TIME_INCREMENT_MS;        
    }

     // Ensure alarm stays on if conditions clear but code not entered (part v.)
    if (alarmActive && !tempAlarm && !gasAlarm) {
        sirenStateWrite(ON);  // Keep siren on until code is entered
        alarmLed = ON;        // Keep LED on
    }

    /*
    // Part i,
    int i;
    for(i = 0; i < 5; i++){
    char key = matrixKeypadUpdate(); //Get key pressed

    displayCharPositionWrite ( 0,3 ); 
    
    //Check if 2 is pressed
    if (key == '2'){
        if (!gasSensorRead()) {
            displayStringWrite("Gas level OK");
        } else {
            displayStringWrite("Gas Level HIGH");
        }
        i++;
    }

    //Check if 3 is pressed
    if (key == '3'){
        if (!overTemperatureDetectorState) {
            displayStringWrite("Temp: Normal");
        } else {
            displayStringWrite("Temp: Overheat!");
        }
        i++;
    }

    //Clear last line
    if (key == '0'){
        displayStringWrite("                    ");
    }
    }
    
    //Part ii.
    // Update the alarm state every 10 seconds
    if (alarmUpdateTimer.read_ms() >= ALARM_INTERVAL) {
        sendAlarmState();
        ThisThread::sleep_for(ALARM_INTERVAL_ON_TIME); //Turn off at 3 seconds
        displayCharPositionWrite ( 0,3 );
        displayStringWrite("                    ");
        alarmUpdateTimer.reset(); // Reset the timer to start counting again
    }*/  

    //Part iv.
        /*if(tempAlarm){
            displayCharPositionWrite ( 0,3 ); 
            displayStringWrite("WARNING: TEMP       ");
        }else if(gasAlarm){
            displayCharPositionWrite ( 0,3 ); 
            displayStringWrite("WARNING: GAS        ");
        }else{
            displayCharPositionWrite ( 0,3 ); 
            displayStringWrite("                    ");
        }*/
    
}

static void incorrectCodeIndicatorUpdate()
{
    incorrectCodeLed = incorrectCodeStateRead();
}

static void systemBlockedIndicatorUpdate()
{
    systemBlockedLed = systemBlockedState;
}

/*static void sendAlarmState(){  //Pat iii.
    displayCharPositionWrite ( 0,3 ); 
    displayStringWrite( "Alarm is" );

    displayCharPositionWrite ( 9,3 );
    if ( sirenStateRead() ) {
        displayStringWrite( "active     " );
    } else {
        displayStringWrite( "not active " );
    }
}*/

static bool checkCodeMatch(){     //Part v.
    for (int i = 0; i < NUMBER_OF_KEYS; i++){
        if (codeSequenceFromUserInterface[i] != codeSequence[i]){
            return false;
        }
    }
    return true;
}
