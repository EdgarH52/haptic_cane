#include <motor.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"

#define MOTOR_MASK 2

void initMotor()
{
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;       //PWM0 clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;     //PD clocks
    _delay_cycles(3);

    GPIO_PORTD_DEN_R |= MOTOR_MASK;              //PD1
    GPIO_PORTD_AFSEL_R |= MOTOR_MASK;
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD1_M;
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD1_M0PWM7;    //configure PD1 to PWM function

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM0_3_CTL_R = 0;                                // turn-off PWM0 generator 3 (out 6 and 7)
    PWM0_3_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO; //on on compare, off on load

    PWM0_3_LOAD_R = 1024;                            // set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz

    PWM0_3_CMPB_R = 0;                               // motor off (0=always low, 1023=always high)

    PWM0_3_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM1 generator 2
    PWM0_ENABLE_R = PWM_ENABLE_PWM7EN;               // enable outputs
}

void setMotorSpeed(uint16_t speed)
{
    PWM0_3_CMPB_R = speed;
}
