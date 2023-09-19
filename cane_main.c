//Edgar Hernandez
//1001571290
//Semester project
//3x sr04 sensors


#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "motor.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "uartparse.h"
#include "eeprom.h"

//PE0 PC5 PC7 triggers
#define TRIGGER0     (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 0*4)))
#define TRIGGER1     (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 5*4)))
#define TRIGGER2     (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))

#define FREQ_IN_MASK0 16  //wtimer0 on PC4
#define FREQ_IN_MASK1 64  //wtimer1 on PC6
#define FREQ_IN_MASK2 1   //wtimer2 on PD0
#define TRIGGER0_MASK 1
#define TRIGGER1_MASK 32
#define TRIGGER2_MASK 128

uint32_t distance[3] = {0};
uint32_t channel = 0;        //dictates which trigger is being set off

void enableTimers() //PC4 PC6 PD0 PD2
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER0_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER0_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
                                                     // configure for edge time mode, count up
    WTIMER0_CTL_R = TIMER_CTL_TAEVENT_BOTH;          // measure time from edge to edge
    WTIMER0_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER0_TAV_R = 0;                               // zero counter for first period
    WTIMER0_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter


    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER1_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
                                                     // configure for edge time mode, count up
    WTIMER1_CTL_R = TIMER_CTL_TAEVENT_BOTH;          // measure time from edge to edge
    WTIMER1_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER1_TAV_R = 0;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter


    WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER2_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER2_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
                                                     // configure for edge time mode, count up
    WTIMER2_CTL_R = TIMER_CTL_TAEVENT_BOTH;          // measure time from edge to edge
    WTIMER2_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER2_TAV_R = 0;                               // zero counter for first period
    WTIMER2_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter

    NVIC_EN2_R = 1 << (INT_WTIMER0A-16-64);          //interrupt 110 on (WTIMER0A)
    NVIC_EN3_R = 1 << (INT_WTIMER1A-16-96);          //interrupt 112 on (WTIMER1A)
    NVIC_EN3_R = 1 << (INT_WTIMER2A-16-96);          //interrupt 114 on (WTIMER2A)

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 2000000;                        // set load value to 2e6 for 20 Hz interrupt rate
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts for timeout in timer module
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN0_R = 1 << (INT_TIMER1A-16);              // interrupt 37 on (TIMER1A)
}

void wideTimer0Isr()
{
    if(WTIMER0_TAV_R*.004287 < 3000)                 //only update non-garbage values
    {
        distance[0] = WTIMER0_TAV_R*.004287;         // read counter input
    }
    WTIMER0_TAV_R = 0;                               // zero counter for next edge
    WTIMER0_ICR_R = TIMER_ICR_CAECINT;               // clear interrupt flag
}

void wideTimer1Isr()
{
    if(WTIMER1_TAV_R*.004287 < 4000)                 //only update non-garbage values
    {
        distance[1] = WTIMER1_TAV_R*.004287;         // read counter input
    }
    WTIMER1_TAV_R = 0;                               // zero counter for next edge
    WTIMER1_ICR_R = TIMER_ICR_CAECINT;               // clear interrupt flag
}

void wideTimer2Isr()
{

    if(WTIMER2_TAV_R*.004287 < 5000)                 //only update non-garbage values
    {
        distance[2] = WTIMER2_TAV_R*.004287;         // read counter input
    }
    WTIMER2_TAV_R = 0;                               // zero counter for next edge
    WTIMER2_ICR_R = TIMER_ICR_CAECINT;               // clear interrupt flag
}

void timer1Isr()
{
    if(channel > 2)                                  //rotate which trigger gets turned on
    {                                                //based on value of channel
        channel = 0;
    }

    switch(channel) {
    case 0:
        TRIGGER0 = 1;
        waitMicrosecond(10);
        TRIGGER0 = 0;
        break;
    case 1:
        TRIGGER1 = 1;
        waitMicrosecond(10);
        TRIGGER1 = 0;
        break;
    case 2:
        TRIGGER2 = 1;
        waitMicrosecond(10);
        TRIGGER2 = 0;
        break;
    }
    channel++;

    TIMER1_ICR_R = TIMER_ICR_TATOCINT;              //clear interrupt flag
}

void disableInterrupts()
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;               //turns off interrupts 110, 112, 114 to avoid noise
    NVIC_DIS2_R = 1 << (INT_WTIMER0A-16-64);

    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    NVIC_DIS3_R = 1 << (INT_WTIMER1A-16-96);

    WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;
    NVIC_DIS3_R = 1 << (INT_WTIMER2A-16-96);
}

void initHw()
{
    //enable TIMER1 clock
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;

    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable WTIMER clocks and PC PD PE clocks
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R0 | SYSCTL_RCGCWTIMER_R1 | SYSCTL_RCGCWTIMER_R2;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R3 | SYSCTL_RCGCGPIO_R4;
    _delay_cycles(3);

    // Configure trigger pins
    GPIO_PORTC_DIR_R |= TRIGGER1_MASK | TRIGGER2_MASK;
    GPIO_PORTE_DIR_R |= TRIGGER0_MASK;
    GPIO_PORTC_DEN_R |= TRIGGER1_MASK | TRIGGER2_MASK;
    GPIO_PORTE_DEN_R |= TRIGGER0_MASK;

    // Configure SIGNAL_IN for frequency and time measurements
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK0 | FREQ_IN_MASK1;         // select alternative functions for FREQ_IN pin
    GPIO_PORTD_AFSEL_R |= FREQ_IN_MASK2;
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC4_M;                       // map alt fns to SIGNAL_IN
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD0_M;
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC4_WT0CCP0 | GPIO_PCTL_PC6_WT1CCP0;
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD0_WT2CCP0;
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK0 | FREQ_IN_MASK1;
    GPIO_PORTD_DEN_R |= FREQ_IN_MASK2;
}

void feedback(uint8_t event)                                     //function to execute event patterns
{
    int i;
    char which[60];                                              //string to display activated events and -
    if(event>15 && readEeprom(128 + 6 + (11*(event-16))))        //- the readings that set them off
    {                                                            //format to display compound events 15-19
        for(i=0;i<readEeprom(128 + 8 + (11*(event-16)));i++)     //beats
        {
            snprintf(which, 60,"event %d\ndistance 1: %dmm\ndistance 2: %dmm\n",
                     event, distance[readEeprom(128+ (11*event))],
                     distance[readEeprom(128+ 1+(11*event))]);
            putsUart0(which);
            setMotorSpeed(readEeprom(128 + 7 + (11*(event-16))));        //PWM duty cycle
            waitMicrosecond(readEeprom(128 + 9 + (11*(event-16))) + 1);  //Ton
            setMotorSpeed(0);
            waitMicrosecond(readEeprom(128 + 10 + (11*(event-16))) + 1); //Toff
        }
    }
    else                                                           //format to display single events 0-15
    {
        for(i=0;i<readEeprom(5 + (8*event)); i++)                  //beats
        {
            snprintf(which,60, "event %d\n distance %dmm\n", event, distance[readEeprom(8*event)]);
            putsUart0(which);
            setMotorSpeed(readEeprom(4 + (8*event)));              //PWM duty cycle
            waitMicrosecond(readEeprom(6 + (8*event)) + 1);        //Ton
            setMotorSpeed(0);
            waitMicrosecond(readEeprom(7 + (8*event)) +  1);       //Toff
        }
    }
}

int main(void)
{
    waitMicrosecond(2000000);                       //wait to avoid junk in keyboard
    initHw();
    initUart0();
    initMotor();
    initEeprom();
    enableTimers();
    setUart0BaudRate(115200, 40e6);

    USER_DATA data;                                  //input info

    int i;
    char show_events[7] = "events";                  //comparison strings for show/haptic commands
    char show_patterns[9] = "patterns";
    char on[] = "on";
    char off[] = "off";
    bool pattern;
    bool valid;
    bool compound = false;
    bool haptic;

    uint8_t sensor1;                                  //hold Eeprom values for easier reading in if statements
    uint8_t sensor2;
    uint16_t min1;
    uint16_t max1;
    uint16_t min2;
    uint16_t max2;

    char str[100] = {NULL};

    putsUart0("Done booting.\n");                    //lets user know to begin entering commands

    while(true)
    {
        if(kbhitUart0())                             //keyboard hit
        {
            for(i=0; i< MAX_CHARS;i++)               //clear data buffer
            {
                data.buffer[i] = NULL;
            }
            for(i=0;i<MAX_FIELDS;i++)
            {
                data.fieldType[i] = NULL;
                data.fieldPosition[i] = 0;
            }
            data.fieldCount = 0;

            getsUart0(&data);                         //get user input

            parseFields(&data);                       //parse input

            valid = false;                            //command invalid until match

            if (isCommand(&data, "alert", 1))         //prints a word encapsulated in '!', left from lab 5
            {
                char* str = getFieldString(&data, 1);
                putcUart0('!');
                putsUart0(str);
                putsUart0("!\n");
                valid = true;
            }

            else if(isCommand(&data, "reboot", 0))    //resets board
            {
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }

            else if(isCommand(&data, "erase", 1))      //sets al values of event to 0, including pattern
            {
                valid = true;
                putsUart0("Erasing event...\n");

                if(getFieldInteger(&data, 1) <16)
                {
                    for(i=0 ;i<8; i++)
                    {
                        writeEeprom(i + (8*getFieldInteger(&data, 1)), 0);             //single erase (8 fields)
                    }
                }
                else
                {
                    for(i=0 ;i<11; i++)
                    {
                        writeEeprom(128 + i + (11*(getFieldInteger(&data, 1)-16)), 0);  //compound erase (11 fields)
                    }
                }
            }

            else if(isCommand(&data, "event", 4))                           //creates single event
            {
                valid = true;
                putsUart0("Setting event...\n");
                writeEeprom(0 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 2));  //sensor
                writeEeprom(1 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 3));  //min
                writeEeprom(2 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 4));  //max
            }

            else if(isCommand(&data, "pattern", 5))                         //sets pattern for event
            {
                putsUart0("setting pattern...\n");
                valid = true;
                if(getFieldInteger(&data, 1) < 16)                           //single addressing
                {
                    writeEeprom(4 + (8*getFieldInteger(&data, 1)), (getFieldInteger(&data, 2) * 256)/25); //PWM
                    writeEeprom(5 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 3));            //beats
                    writeEeprom(6 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 4)*1000);       //Ton
                    writeEeprom(7 + (8*getFieldInteger(&data, 1)), getFieldInteger(&data, 5)*1000);       //Toff
                }
                else                                                         //compound addressing
                {
                    writeEeprom(128 + 7 + (11*(getFieldInteger(&data, 1)-16)), (getFieldInteger(&data, 2) * 256)/25);
                    //PWM
                    writeEeprom(128 + 8 + (11*(getFieldInteger(&data, 1)-16)), getFieldInteger(&data, 3));
                    //beats
                    writeEeprom(128 + 9 + (11*(getFieldInteger(&data, 1)-16)), getFieldInteger(&data, 4)*1000);
                    //Ton
                    writeEeprom(128 + 10 + (11*(getFieldInteger(&data, 1)-16)), getFieldInteger(&data, 5)*1000);
                    //Toff
                }
            }

            else if(isCommand(&data, "and", 3))    //creates compound from 2 singles
            {
                valid = true;
                if(getFieldInteger(&data, 1)>15)
                {
                    putsUart0("Setting compound event...\n");
                    writeEeprom(128 + 0 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(0+(8*getFieldInteger(&data, 2))));
                    //sensor event 1
                    writeEeprom(128 + 1 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(1+(8*getFieldInteger(&data, 2))));
                    //min event 1
                    writeEeprom(128 + 2 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(2+(8*getFieldInteger(&data, 2))));
                    //max event 1
                    writeEeprom(128 + 3 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(0+(8*getFieldInteger(&data, 3))));
                    //sensor event 2
                    writeEeprom(128 + 4 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(1+(8*getFieldInteger(&data, 3))));
                    //min event 2
                    writeEeprom(128 + 5 + (11*(getFieldInteger(&data, 1)-16)), readEeprom(2+(8*getFieldInteger(&data, 3))));
                    //max event 2
                }
                else(putsUart0("Invalid event slot. Compound events are 16-19.\n"));
                //doesn't allow compound written to single slot
            }

            else if(isCommand(&data, "haptic", 2))
            {
                valid = true;
                haptic = true;

                for(i = 0;i< 3;i++)
                {
                    if(data.buffer[data.fieldPosition[2]+i] != on[i])  //checks if input is on
                    {
                        haptic = false;
                    }
                }

                if(!haptic)                               //if not on checks if input is off
                {
                    for(i=0;i<4;i++)
                    {
                        if(data.buffer[data.fieldPosition[2]+i] != off[i])
                        {
                            valid = false;
                        }
                    }
                }

                if(haptic)
                {
                    putsUart0("setting haptic...\n");
                    if(getFieldInteger(&data, 1) < 16)
                    {
                        writeEeprom(3 + (8*getFieldInteger(&data, 1)), 1);  //single haptic on
                    }
                    else
                    {
                        writeEeprom(128 + 6 + (11*(getFieldInteger(&data, 1)-16)), 1);
                        //compound haptic on
                    }
                }
                else if (!haptic && valid)
                {
                    putsUart0("setting haptic...\n");
                    if(getFieldInteger(&data, 1) < 16)
                    {
                        writeEeprom(3 + (8*getFieldInteger(&data, 1)), 0); //single haptic off
                    }
                        else
                    {
                        writeEeprom(128 + 6 + (11*(getFieldInteger(&data, 1)-16)), 0);
                        //compound haptic off
                    }
                }
            }

            else if(isCommand(&data, "show", 1))
            {
                pattern = true;
                valid = true;

                for(i=0;i<6;i++)   //check if events
                {
                    if(data.buffer[data.fieldPosition[1]+i] != show_events[i])
                    {
                        valid = false;
                    }
                }

                for(i=0;i<8;i++)   //check if patterns
                {
                    if(data.buffer[data.fieldPosition[1]+i] != show_patterns[i])
                    {
                        pattern = false;
                    }
                }

                if(valid)    //if events
                {
                    putsUart0("\nprinting...\n");
                    for(i=0;i<16;i++)                //single format
                    {
                        snprintf(str,100, "Event: %d\nSensor: %d\nMin: %dmm\nMax:  %dmm\n\n", i, readEeprom(0+(8*i)),
                                readEeprom(1+(8*i)), readEeprom(2+(8*i)));
                        putsUart0(str);
                    }

                    for(i=0;i<4;i++)                  //compound format
                    {
                        snprintf(str,100, "Event: %d\nSensor 1: %d\nSensor 2: %d\nMin 1: %dmm\nMax 1: %dmm\n"
                                "Min 2: %dmm\nMax 2: %dmm\n\n", i + 16,
                                readEeprom(128+0+(11*i)), readEeprom(128+3+(11*i)), readEeprom(128+1+(11*i)),
                                readEeprom(128+2+(11*i)),readEeprom(128+4+(11*i)), readEeprom(128+5+(11*i)));
                        putsUart0(str);
                    }
                }

                else if(pattern)       //if second field patterns
                {
                    valid = true;
                    putsUart0("\nprinting...\n");
                    for(i=0;i<16;i++)      //single format
                    {
                        if(readEeprom(3 + (8*i))) //if haptic on
                            {
                            snprintf(str, 100, "Event: %d\nHaptic: on\nTon: %d\nToff: %d\nBeats: %d\nPWM:  %d%%\n\n",
                                     i,readEeprom(6+(8*i))/1000, readEeprom(7+(8*i))/1000,
                                     readEeprom(5+(8*i)), (readEeprom(4+(8*i))*25)/256);
                            putsUart0(str);
                            }
                        else                      //if haptic off
                        {
                            snprintf(str, 100, "Event: %d\nHaptic: off\nTon: %d\nToff: %d\nBeats: %d\nPWM:  %d%%\n\n",
                                     i,readEeprom(6+(8*i))/1000, readEeprom(7+(8*i))/1000,
                                     readEeprom(5+(8*i)), (readEeprom(4+(8*i))*25)/256);
                            putsUart0(str);
                        }
                    }

                    for(i=0;i<4;i++)               //compound format
                    {
                        if(readEeprom(128+6+(11*i)))        //if haptic on
                        {
                            snprintf(str,100, "Event: %d\nHaptic: on\nTon: %dms\nToff: %dms\nBeats: %d\nPWM:  %d%%\n\n",
                            i + 16, readEeprom(128 + 9+(11*i))/1000, readEeprom(128 + 10+(11*i))/1000,
                            readEeprom(128 + 8+(11*i)), (readEeprom(128 + 7+(11*i))*25)/256);
                            putsUart0(str);
                        }
                        else                                 //if haptic off
                        {
                            snprintf(str,100, "Event: %d\nHaptic: off\nTon: %dms\nToff: %dms\nBeats: %d\nPWM:  %d%%\n\n",
                            i + 16, readEeprom(128 + 9+(11*i))/1000, readEeprom(128 + 10+(11*i))/1000,
                            readEeprom(128 + 8+(11*i)), (readEeprom(128 + 7+(11*i))*25)/256);
                            putsUart0(str);
                        }
                    }
                }
            }

            else if(isCommand(&data, "motor", 1))        // motor command to manually set motor speed
            {
                valid = true;
                setMotorSpeed(getFieldInteger(&data, 1));
            }

            if(!valid)               //if no match on commands
            {
                putsUart0("invalid command.\n");
            }

            for(i=0; i< MAX_CHARS;i++)    //clear data buffer
            {
                data.buffer[i] = NULL;
            }
            for(i=0;i<MAX_FIELDS;i++)
            {
                data.fieldType[i] = NULL;
                data.fieldPosition[i] = 0;
            }
                data.fieldCount = 0;
        }
        //outside kbhit

        for(i=19;i>15;i--) //check compounds
        {
            sensor1 = readEeprom(128 + 0 + (11*(i-16)));
            sensor2 = readEeprom(128 + 3 + (11*(i-16)));
            min1 = readEeprom(128 + 1 + (11*(i-16)));
            min2 = readEeprom(128 + 4 + (11*(i-16)));
            max1 = readEeprom(128 + 2 + (11*(i-16)));
            max2 = readEeprom(128 + 5 + (11*(i-16)));

            if((distance[sensor1] > min1) && (distance[sensor1] < max1)
               && (distance[sensor2] > min2) && (distance[sensor2] < max2)
               && readEeprom(128 + 6 + (11*(i-16))))
            {
                feedback(i);              //if true execute pattern
                waitMicrosecond(1000000); //time off after giving feeback to avoid noise
                i = 15;                   //break loop to avoid checking lower priority while higher is true
                compound = true;          //bool to avoid checking singles while compound is true
            }
        }

        if(!compound)                     //if compound event not currently true
        {
            for(i=15;i>=0;i--)            //check singles
            {
                sensor1 = readEeprom(0 + (8*i));
                min1 = readEeprom(1 + (8*i));
                max1 = readEeprom(2 + (8*i));

                if((distance[sensor1] > min1) && (distance[sensor1] < max1)
                        && readEeprom(3 + (8*i)))
                {
                    feedback(i);              //execute pattern if true
                    waitMicrosecond(1000000); //wait to avoid noise
                    i = -1;                   //break loop to avoid checking lower priority while higher true
                }
            }
        }
        compound = false; //reset compound for next check
    }
}
