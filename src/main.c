/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms. By expressly
 * accepting such terms or by downloading, installing, activating and/or otherwise
 * using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms. If you do not agree to be
 * bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software. The production use license in
 * Section 2.3 is expressly granted for this software.
 */


/* Including necessary configuration files. */
#include "sdk_project_config.h"
#include "interrupt_manager.h"
#include "osif.h"
#include <stdio.h>
#include <string.h>
#include "peripherals_pdb_1.h"


/* ******** DEFINE ****************** */
#define PCC_CLOCK	PCC_PORTD_CLOCK

/* ******** Define LED ports/pins ******* */

/* Led RED - PTD15 */
#define LED_RED_PORT 	PTD
#define LED_RED_PIN  	15

/* Led GREEN - PTD16 */
#define LED_GREEN_PORT 	PTD
#define LED_GREEN_PIN  	16

/* Led BLUE - PTD0 */
#define LED_BLUE_PORT 	PTD
#define LED_BLUE_PIN  	0

/* ******** Define Buttons ports/pins/irq ******* */
#define BTN_GPIO        PTC
#define BTN1_PIN        13U
#define BTN2_PIN        12U
#define BTN_PORT        PORTC
#define BTN_PORT_IRQn   PORTC_IRQn

/* ******** Define ADC - ADC0 SE12 ******* */
#define ADC_INSTANCE    0UL
#define ADC_CHN         ADC_INPUTCHAN_EXT12
#define ADC_VREFH       5.0f
#define ADC_VREFL       0.0f

/******* Define identification ports/pins ***********/
#define PTE0 0

/* ******** Define PDB instance ******* */
#define PDB_INSTANCE    0UL

/* ******** Define DC ports/pins/irq ******* */
/*   IN1   */
#define DRV_IN1_PORT 	PTE
#define DRV_IN1_PIN  	13  //1
/*   IN2   */
#define DRV_IN2_PORT 	PTE //PTD
#define DRV_IN2_PIN  	15  //7
/*   ENA   */
#define DRV_ENA_PORT 	PTE
#define DRV_ENA_PIN  	14

/*   IN1-1   */
#define DRV_IN11_PORT 	PTE
#define DRV_IN11_PIN  	1  //1

/* ******** Define Stepper ports/pins/irq ******* */
/*   STEP   */
#define STP_DRV_STEP_PORT 	PTE
#define STP_DRV_STEP_PIN  	1  //1
/*   EN   */
#define STP_DRV_EN_PORT 	PTE //PTD
#define STP_DRV_EN_PIN  	15  //7
/*   DIRECTION   */
#define STP_DRV_DIR_PORT 	PTE
#define STP_DRV_DIR_PIN  	14


/*   COOLER   */
#define COOLER_PORT 	PTD
#define COOLER_PIN  	7

//#define LED_RED_TOGGLE

/* LPIT channel used */
#define LPIT_CHANNEL        0UL
#define LPIT_Channel_IRQn   LPIT0_Ch0_IRQn


/* Timeout in ms for blocking operations on LPUART */
#define TIMEOUT     500U


/*command for increase/decrease brightness */
typedef enum
{
    LED_BRIGHTNESS_INCREASE_REQUESTED = 0x00U,
	LED_BRIGHTNESS_DECREASE_REQUESTED = 0x01U
} btn_commands_list;

/* Timeout for PDB in microseconds */
#define PDLY_TIMEOUT   10000UL //1000000UL

/* ******** VARIABLES ***************** */
volatile int exit_code = 0;

/* End command for Nextion display. Every command should end with this */
uint8_t Cmd_End[3]={0xFF,0xFF,0xFF};

/* store led request button value */
uint8_t ledRequested = (uint8_t)LED_BRIGHTNESS_INCREASE_REQUESTED;

/* LED PWM duty cycle */
volatile int Led_Red_dutyCycle = 0U;
volatile int Led_Green_dutyCycle = 0U;
volatile int Led_Blue_dutyCycle = 0U;
volatile int Servo_dutyCycle = 0U;
volatile int Gauge_value = 0U;

/* ECU identification */
bool isHMI=false;
/* Flag used to store if an ADC IRQ was executed */
volatile bool adcConvDone;
/* Variable to store value from ADC conversion */
volatile uint16_t adcRawValue;


/*  ***** FUNCTIONS  ******** */

/* @brief: ADC Interrupt Service Routine.
 *        Read the conversion result, store it
 *        into a variable and set a specified flag.
 */
void ADC_IRQHandler(void)
{
    /* Get channel result from ADC channel */
    ADC_DRV_GetChanResult(ADC_INSTANCE, 0U, (uint16_t *)&adcRawValue);
    /* Set ADC conversion complete flag */
    adcConvDone = true;
}

/*!
 * @brief: LPIT interrupt handler.
 *         When an interrupt occurs clear channel flag and toggle LED0
 */
void LPIT_ISR(void)
{
	if (LPIT_DRV_GetInterruptFlagTimerChannels(INST_LPIT_CONFIG_1,(1 << LPIT_CHANNEL)))
	{
		/* Clear LPIT channel flag */
		LPIT_DRV_ClearInterruptFlagTimerChannels(INST_LPIT_CONFIG_1, (1 << LPIT_CHANNEL));


		PINS_DRV_TogglePins(STP_DRV_STEP_PORT, (uint32_t) (1u << STP_DRV_STEP_PIN));
	}
}



/** * Button interrupt handler - used to increase/ decrease duty for BLUE LED */
void buttonISR(void)
{
    /* Check if one of the buttons (SW3/SW2) was pressed */
    uint32_t buttonsPressed = PINS_DRV_GetPortIntFlag(BTN_PORT) &
                                           ((1 << BTN1_PIN) | (1 << BTN2_PIN));


    if(buttonsPressed != 0)
    {
        /* Set LED PWM duty value according to the button pressed */
        switch (buttonsPressed)
        {
            case (1 << BTN1_PIN):
                ledRequested = LED_BRIGHTNESS_INCREASE_REQUESTED;

            	/* start / stop stepper motor */

            	//PINS_DRV_TogglePins(DRV_IN1_PORT, (uint32_t) (1u << DRV_IN1_PIN));
            	PINS_DRV_TogglePins(STP_DRV_EN_PORT, (uint32_t) (1u << STP_DRV_EN_PIN));

#if 0

            	 if (PTE->PDIR & (1<<DRV_IN1_PIN))
            	 {
            		 PINS_DRV_ClearPins(DRV_ENA_PORT, (uint32_t) (1u << DRV_ENA_PIN));
            	 }
            	 else
            	 {
            		 PINS_DRV_SetPins(DRV_ENA_PORT, (uint32_t) (1u << DRV_ENA_PIN));
            	 }

            	//PINS_DRV_TogglePins(STP_DRV_STEP_PORT, (uint32_t) (1u << STP_DRV_STEP_PIN));
#endif


                if(Led_Blue_dutyCycle<4600)
                {
                	Led_Blue_dutyCycle=Led_Blue_dutyCycle+100;

                }
                else
                {
                	Led_Blue_dutyCycle=4700;
                }
                /* Clear interrupt flag */
                PINS_DRV_ClearPinIntFlagCmd(BTN_PORT, BTN1_PIN);
                break;


            case (1 << BTN2_PIN):
                ledRequested = LED_BRIGHTNESS_DECREASE_REQUESTED;

                //PINS_DRV_TogglePins(DRV_IN2_PORT, (uint32_t) (1u << DRV_IN2_PIN));

            	/* set direction for stepper motor */
            	PINS_DRV_TogglePins(STP_DRV_DIR_PORT, (uint32_t) (1u << STP_DRV_DIR_PIN));
#if 0

           	 if (PTE->PDIR & (1<<DRV_IN2_PIN))
           	 {
           		 PINS_DRV_ClearPins(DRV_ENA_PORT, (uint32_t) (1u << DRV_ENA_PIN));
           	 }
           	 else
           	 {
           		 PINS_DRV_SetPins(DRV_ENA_PORT, (uint32_t) (1u << DRV_ENA_PIN));
           	 }
#endif

                if(Led_Blue_dutyCycle>100)
                {
                	Led_Blue_dutyCycle=Led_Blue_dutyCycle-100;

                }
                else
                {
                	Led_Blue_dutyCycle=10;
                }
                /* Clear interrupt flag */
                PINS_DRV_ClearPinIntFlagCmd(BTN_PORT, BTN2_PIN);
                break;
            default:
                //PINS_DRV_ClearPortIntFlagCmd(BTN_PORT);
                break;
        }
    }
}


/* Update the gauge on display with value sent on UART  */
void UpdateGauge(char *obj, uint16_t value)
{
	char buf[30];

	/*minimum value fr position 0 */
	value+=315;

	/*scaled the value upper 0  */
	if(value>=360)
		{
			value=value-360;
		}
	int len=sprintf(buf, "%s=%u", obj,value);

	/*  send value of gauge (z0.value) on UART    */
	LPUART_DRV_SendDataBlocking(INST_LPUART_LPUART_1, (uint8_t *)buf, len, TIMEOUT);
	//delay(10U);
	/* send the end command to display  */
	LPUART_DRV_SendDataBlocking(INST_LPUART_LPUART_1, (uint8_t *)Cmd_End, 3, TIMEOUT);

}

/* @brief: Calculate the values to be used by pdb to generate
 *        a interrupt at a specific timeout.
 * @param pdbConfig: pointer to the PDB configuration struct
 * @param type:      pdb_timer_config_t *
 * @param uSec:      interval for pdb interrupt in microseconds
 * @param type:      uint32_t
 * @param intVal:    pointer to the storage element where to set the calculated value
 * @param type:      uint16_t
 * @return:          Returns true if the interrupt period can be achieved, false if not
 * @return type:     bool
 */
bool calculateIntValue(const pdb_timer_config_t *pdbConfig, uint32_t uSec, uint16_t * intVal)
{
    /* Local variables used to store different parameters
     * such as frequency and prescalers
     */
    uint32_t    intVal_l            = 0;
    uint8_t     pdbPrescaler        = (1 << pdbConfig->clkPreDiv);
    uint8_t     pdbPrescalerMult    = 0;
    uint32_t    pdbFrequency;

    bool resultValid = false;

    /* Get the Prescaler Multiplier from the configuration structure */
    switch (pdbConfig->clkPreMultFactor)
    {
        case PDB_CLK_PREMULT_FACT_AS_1:
            pdbPrescalerMult    =   1U;
            break;
        case PDB_CLK_PREMULT_FACT_AS_10:
            pdbPrescalerMult    =   10U;
            break;
        case PDB_CLK_PREMULT_FACT_AS_20:
            pdbPrescalerMult    =   20U;
            break;
        case PDB_CLK_PREMULT_FACT_AS_40:
            pdbPrescalerMult    =   40U;
            break;
        default:
            /* Defaulting the multiplier to 1 to avoid dividing by 0*/
            pdbPrescalerMult    =   1U;
            break;
    }

    /* Get the frequency of the PDB clock source and scale it
     * so that the result will be in microseconds.
     */
    CLOCK_SYS_GetFreq(CORE_CLOCK, &pdbFrequency);
    pdbFrequency /= 1000000;

    /* Calculate the interrupt value for the prescaler, multiplier, frequency
     * configured and time needed.
     */
    intVal_l = (pdbFrequency * uSec) / (pdbPrescaler * pdbPrescalerMult);

    /* Check if the value belongs to the interval */
    if((intVal_l == 0) || (intVal_l >= (1 << 16)))
    {
        resultValid = false;
        (*intVal) = 0U;
    }
    else
    {
        resultValid = true;
        (*intVal) = (uint16_t)intVal_l;
    }

    return resultValid;
}

/* set delay in cycles */
void delay(volatile int cycles)
{
    /* Delay function - do nothing for a number of cycles */
    while(cycles--);
}

/* main function */
int main(void)
                                   {
	
	uint16_t delayValue = 0;
    //status_t status;
	/* Write your local variable definition here */
    ftm_state_t ftmStateStruct1, ftmStateStruct2;

	/* Write your local variable definition here */

	/* Variables in which we store data from ADC */

	uint16_t adcMax;
	uint16_t adcValue=0;

    IRQn_Type adcIRQ;

    adcConvDone = false;

	/* Initialize clock module */  
	CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT, g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
	CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);

	/* Initialize pins
	 *    -    See PinSettings component for more info
	 */
	PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

	/* Get ADC max value from the resolution */
	adcMax = (uint16_t) (1 << 12);

	/* Configure and calibrate the ADC converter
	 *  -   See ADC component for the configuration details
	 */
	DEV_ASSERT(ADC_0_ChnConfig0.channel == ADC_CHN);

	ADC_DRV_ConfigConverter(ADC_INSTANCE, &ADC_0_ConvConfig0);
	ADC_DRV_AutoCalibration(ADC_INSTANCE);
	ADC_DRV_ConfigChan(ADC_INSTANCE, 0UL, &ADC_0_ChnConfig0);

    /* Initialize LPUART instance */
    LPUART_DRV_Init(INST_LPUART_LPUART_1, &lpUartState1, &lpUartInitConfig1);

    /* Initialize the Enhanced DMA to be used for LPUART transfers
         *  - Setup channel allocation
         *  -   See EDMA component for more info
         */
    EDMA_DRV_Init(&dmaController_State,
                  &dmaController_InitConfig,
                  edmaChnStateArray,
                  edmaChnConfigArray,
                  EDMA_CONFIGURED_CHANNELS_COUNT);

    /* Initialize FTM instance */
    FTM_DRV_Init(INST_FLEXTIMER_PWM_1, &flexTimer_pwm_1_InitConfig, &ftmStateStruct1);
    FTM_DRV_Init(INST_FLEXTIMER_PWM_2, &flexTimer_pwm_2_InitConfig, &ftmStateStruct2);

    /* Initialize FTM PWM */
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM_1, &flexTimer_pwm_1_PwmConfig);
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM_2, &flexTimer_pwm_2_PwmConfig);

    /* Initialize LPIT instance 0
     *  -   Reset and enable peripheral
     */
    LPIT_DRV_Init(INST_LPIT_CONFIG_1, &lpit1_InitConfig);
    /* Initialize LPIT channel 0 and configure it as a periodic counter
     * which is used to generate an interrupt every second.
     */
    /* Initialize LPIT channel 0 and configure it as a periodic counter
        * which is used to generate an interrupt every second.
        */
    LPIT_DRV_InitChannel(INST_LPIT_CONFIG_1, LPIT_CHANNEL, &lpit1_ChnConfig0);

    /* Setup button pins */
    PINS_DRV_SetPinsDirection(BTN_GPIO, ~((1 << BTN1_PIN)|(1 << BTN2_PIN)));

    /* Setup button pins interrupt */
    PINS_DRV_SetPinIntSel(BTN_PORT, BTN1_PIN, PORT_INT_RISING_EDGE);
    PINS_DRV_SetPinIntSel(BTN_PORT, BTN2_PIN, PORT_INT_RISING_EDGE);

    /* Install LPIT_ISR as LPIT interrupt handler */
    INT_SYS_InstallHandler(LPIT_Channel_IRQn, &LPIT_ISR, NULL);

    /* Start LPIT0 channel 0 counter */
    LPIT_DRV_StartTimerChannels(INST_LPIT_CONFIG_1, (1 << LPIT_CHANNEL));

    /* Install buttons ISR */
    INT_SYS_InstallHandler(BTN_PORT_IRQn, &buttonISR, NULL);

    /* Enable buttons interrupt */
    INT_SYS_EnableIRQ(BTN_PORT_IRQn);

    /* Turn on DC motor */
   // PINS_DRV_SetPins(DRV_ENA_PORT, (1 << DRV_ENA_PIN));


    PINS_DRV_SetPins(STP_DRV_EN_PORT, (1 << STP_DRV_EN_PIN));
    PINS_DRV_ClearPins(STP_DRV_EN_PORT, (1 << STP_DRV_EN_PIN));

    PINS_DRV_SetPins(STP_DRV_DIR_PORT, (1 << STP_DRV_DIR_PIN));
    PINS_DRV_ClearPins(STP_DRV_DIR_PORT, (1 << STP_DRV_DIR_PIN));

    /* ADC IRQ set */
       
	switch(ADC_INSTANCE)
    {
    case 0UL:
        adcIRQ = ADC0_IRQn;
        break;
    case 1UL:
        adcIRQ = ADC1_IRQn;
        break;
    default:
        adcIRQ = ADC1_IRQn;
        break;
    }

    INT_SYS_InstallHandler(adcIRQ, &ADC_IRQHandler, (isr_t*) 0);

     /* Calculate the value needed for PDB instance
     * to generate an interrupt at a specified timeout.
     * If the value can not be reached, stop the application flow
     */
	if (!calculateIntValue(&pdb_1_timerConfig0, PDLY_TIMEOUT, &delayValue))
    {
        /* Stop the application flow */
	   while(1);
    }
    /* Setup PDB instance
     *  -   See PDB component for details
     *  Note: Pre multiplier and Prescaler values come from
     *        calculateIntValue function.
     */
    PDB_DRV_Init(PDB_INSTANCE, &pdb_1_timerConfig0);
    PDB_DRV_Enable(PDB_INSTANCE);
    PDB_DRV_ConfigAdcPreTrigger(PDB_INSTANCE, 0UL, &pdb_1_adcTrigConfig0);
    PDB_DRV_SetTimerModulusValue(PDB_INSTANCE, (uint32_t) delayValue);
    PDB_DRV_SetAdcPreTriggerDelayValue(PDB_INSTANCE, 0UL, 0UL, (uint32_t) delayValue);
    PDB_DRV_LoadValuesCmd(PDB_INSTANCE);
    PDB_DRV_SoftTriggerCmd(PDB_INSTANCE);

    /* Enable ADC 1 interrupt */
    INT_SYS_EnableIRQ(adcIRQ);

	/* check if ECU is HMI  */
    if (PTE->PDIR & (1<<PTE0))
    {
       isHMI=true;
    }
    else
    {
		isHMI=false;
    }
    
	/*set brightness to 0 on LED_RED */
	FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_1,
	        flexTimer_pwm_1_IndependentChannelsConfig[0].hwChannelId,
	        FTM_PWM_UPDATE_IN_TICKS, (uint16_t)0, 0U, true);

	FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_2,
	        flexTimer_pwm_2_IndependentChannelsConfig[0].hwChannelId,
	        FTM_PWM_UPDATE_IN_TICKS, (uint16_t)300, 0U, true);

    /* Infinite loop */
	while (1)
	{
		Servo_dutyCycle=(int)(adcValue/2.5);
		if(Servo_dutyCycle<300)
		{
			Servo_dutyCycle=300;
		}
		if(Servo_dutyCycle>1800)
		{
			Servo_dutyCycle=1800;
		}

	FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_2,
		        flexTimer_pwm_2_IndependentChannelsConfig[0].hwChannelId,
		        FTM_PWM_UPDATE_IN_TICKS, (uint16_t)Servo_dutyCycle, 0U, true);

	/* set the LED GREEN ON at mid brightness*/
	FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_1,
	                            flexTimer_pwm_1_IndependentChannelsConfig[1].hwChannelId,
	                            FTM_PWM_UPDATE_IN_TICKS, (uint16_t)Led_Green_dutyCycle,
	                            0U,
	                            true);

	/* set the LED GREEN ON at minimum brightness*/
	FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_1,
	                            flexTimer_pwm_1_IndependentChannelsConfig[2].hwChannelId,
	                            0x01, (uint16_t)Led_Blue_dutyCycle,
	                            0U,
	                            true);


  	

	#ifdef LED_RED_TOGGLE
	/* Increase the brightness of LED RED*/
    for (Led_Red_dutyCycle = 0; Led_Red_dutyCycle < 4799; Led_Red_dutyCycle += 10)
    {
        FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_1,
                                flexTimer_pwm_1_IndependentChannelsConfig[0].hwChannelId,
                                FTM_PWM_UPDATE_IN_TICKS, (uint16_t)Led_Red_dutyCycle,
                                0U,
                                true);
        OSIF_TimeDelay(1);
    }
    OSIF_TimeDelay(10);
	#endif


	/* if ECU is HMI then info to display to be sent - 0..270*/
	if(isHMI)
	{  

			Gauge_value=((int)(adcValue/18.5));
			UpdateGauge("z0.val",Gauge_value);

	}


	#ifdef LED_RED_TOGGLE
	/* Decrease the brightness */
    for (Led_Red_dutyCycle = 4799; Led_Red_dutyCycle > 0; Led_Red_dutyCycle -= 10)
    {
        FTM_DRV_UpdatePwmChannel(INST_FLEXTIMER_PWM_1,
                                    flexTimer_pwm_1_IndependentChannelsConfig[0].hwChannelId,
                                    FTM_PWM_UPDATE_IN_TICKS, (uint16_t)Led_Red_dutyCycle,
                                    0U,
                                    true);
        OSIF_TimeDelay(1);
    }
    OSIF_TimeDelay(10);
	#endif
	/* Process the result to get the value in volts */
	if (adcConvDone == true)
        {
            /* Process the result to get the value in volts */
          //  adcValue = ((float) adcRawValue / adcMax) * (ADC_VREFH - ADC_VREFL);
           	adcValue = (( adcRawValue * 5000) / adcMax) ;// (ADC_VREFH - ADC_VREFL); 
            /* Clear conversion done interrupt flag */
            adcConvDone = false;
            /* Trigger PDB timer */
            PDB_DRV_SoftTriggerCmd(PDB_INSTANCE);
        }

	
	Led_Green_dutyCycle= (int)(adcValue*0.8);

	/* activate / deactivate fan  */

	if(adcValue>2500)
	{

		    PINS_DRV_ClearPins(COOLER_PORT, (1 << COOLER_PIN));
	}
	else
	{
			PINS_DRV_SetPins(COOLER_PORT, (1 << COOLER_PIN));
	}


	}

	return exit_code;	  
}
