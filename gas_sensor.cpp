//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"
#include "gas_sensor.h"

#include "smart_home_system.h"

#include "display.h"

//=====[Declaration of private defines]========================================

#define MQ2_NUMBER_OF_AVG_SAMPLES    10

#define MQ2_GAS_THRESHOLD_PPM       2000  // Threshold for gas detection 

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

AnalogIn mq2(A2);


//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

float mq2PPM = 0.0;
float mq2ReadingsArray[MQ2_NUMBER_OF_AVG_SAMPLES];
static float mq2R0 = 0.0; //Resistance in clean air

static Timer calibrationTimer; // Timer for non-blocking delay in calibration

//=====[Declarations (prototypes) of private functions]========================

float calculatePPM();

//Function to clear the display by overwriting with spaces
void displayClear() {
    //Move to the first position and write 20 spaces
    displayCharPositionWrite(0, 0);
    const char *spaces = "                    "; //20 spaces
    displayStringWrite(spaces);
    //Move back to the starting position for the next message
    displayCharPositionWrite(0, 0);
}

//=====[Implementations of public functions]===================================

void gasSensorInit()
{
    int i;
    
    for( i=0; i<MQ2_NUMBER_OF_AVG_SAMPLES ; i++ ) {
        mq2ReadingsArray[i] = 0;
    }
}

void gasSensorUpdate()
{
    static int mq2SampleIndex = 0;
    float mq2ReadingsSum = 0.0;
    float mq2ReadingsAverage = 0.0;

    int i = 0;

    mq2ReadingsArray[mq2SampleIndex] = mq2.read();
       mq2SampleIndex++;
    if ( mq2SampleIndex >= MQ2_NUMBER_OF_AVG_SAMPLES) {
        mq2SampleIndex = 0;
    }
    
    // Calculate average
    mq2ReadingsSum = 0.0;
    for (i = 0; i < MQ2_NUMBER_OF_AVG_SAMPLES; i++) {
        mq2ReadingsSum += mq2ReadingsArray[i];
    }
    mq2ReadingsAverage = mq2ReadingsSum / MQ2_NUMBER_OF_AVG_SAMPLES;


    mq2PPM = calculatePPM();  

    // Compare the PPM value to the threshold
    if (mq2PPM > MQ2_GAS_THRESHOLD_PPM) {
        printf("Gas detected! PPM exceeds threshold.\n");
    } else {
        printf("Gas level is normal.\n");
    }

    printf("PPM: %.2f\n", mq2PPM);    
}

bool gasSensorRead()
{
    return (mq2PPM > MQ2_GAS_THRESHOLD_PPM);
}

//=====[Implementations of private functions]==================================

float calculatePPM(){
    float sensorValue = mq2.read();
    float voltage = sensorValue * 3.3;

    float Rs = (5.0 - voltage) / voltage; 

    float R0 = 10.0;
    float ratio = Rs / R0;

    float a = 100.0;
    float b = -2.0;  
    float ppm = a * pow(ratio, b);

    return ppm;
}
