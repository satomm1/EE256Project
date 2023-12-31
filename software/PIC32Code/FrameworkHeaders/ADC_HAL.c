/****************************************************************************
 Module
   ADC_HAL.c

 Revision
   1.0.1

 Description
   This implements a hardware abstraction layer for the PIC32 ADC

 Notes

****************************************************************************/
#include "ES_Configure.h"
#include "ES_Framework.h"

/**************************************************************************
  Function
     InitADC

 Parameters
     None

 Returns
     None

 Description
     Initializes the ADC
 Notes
 
 *************************************************************************/
void InitADC(void) {
  /* 
    Step 1: Initialize the ADC calibration values by
    copying them from the factory programmed
    DEVADCx Flash locations starting at 0xBFC45000
    into the ADCxCFG registers starting at 0xBF887D00,
    respectively. 
   */
   ADC0CFG = DEVADC0;        //Load ADC0 Calibration values    
   ADC1CFG = DEVADC1;        //Load ADC1 Calibration values
   ADC7CFG = DEVADC7;        //Load ADC7 Calibration values
   
  /*
   Step 2: The user writes all the essential ADC
    configuration SFRs including the ADC control clock
    and all ADC core clocks setup:
    -ADCCON1, keeping the ON bit = 0
    -ADCCON2, especially paying attention to ADCDIV<6:0> and SAMC<9:0>
    -ADCANCON, keeping all analog enables ANENx bit = 0, WKUPCLKCNT bit = 0xA
    -ADCCON3, keeping all DIGEN5x = 0, especially paying attention to ADCSEL<1:0>, CONCLKDIV <5:0>, and VREFSEL<2:0>
    -ADCxTIME, ADCDIVx<6:0>, and SAMCx<9:0>
    -ADCTRGMODE, ADCIMCONx, ADCTRGSNS, ADCCSSx, ADCGIRQENx, ADCTRGx, ADCBASE
    -Comparators, Filters, etc.
   */
   
   /* Configure ADCCON1 */ 
   ADCCON1 = 0; // No ADCCON1 features are enabled including: Stop-in-Idle, turbo, 
                // CVD mode, Fractional mode and scan trigger source. 
   ADCCON1bits.SELRES = 3; // ADC7 resolution is 12 bits 
   ADCCON1bits.STRGSRC = 1; // Select scan trigger.
   
   /* Configure ADCCON2 */ 
   ADCCON2bits.SAMC = 5; // ADC7 sampling time = 5 * TAD7 
   ADCCON2bits.ADCDIV = 1; // ADC7 clock freq is half of control clock = TAD7
   
   /* Initialize warm up time register */ 
   ADCANCON = 0; 
   ADCANCONbits.WKUPCLKCNT = 5; // Wakeup exponent = 32 * TADx
   
   /* Clock setting */ 
   ADCCON3bits.ADCSEL = 0; // Select input clock source 
   ADCCON3bits.CONCLKDIV = 1; // Control clock frequency is half of input clock 
   ADCCON3bits.VREFSEL = 0; // Select AVDD and AVSS as reference source
   
   ADC0TIMEbits.ADCDIV = 1; // ADC0 clock frequency is half of control clock = TAD0 
   ADC0TIMEbits.SAMC = 5; // ADC0 sampling time = 5 * TAD0 
   ADC0TIMEbits.SELRES = 3; // ADC0 resolution is 12 bits
   
   /* Select analog input for ADC modules, no presync trigger, not sync sampling */ 
   ADCTRGMODEbits.SH0ALT = 0; // ADC0 = AN0
   
   /* Select ADC input mode */ 
   ADCIMCON1bits.SIGN12 = 0; // unsigned data format 
   ADCIMCON1bits.DIFF12 = 0; // Single ended mode 
   ADCIMCON1bits.SIGN13 = 0; // unsigned data format 
   ADCIMCON1bits.DIFF13 = 0; // Single ended mode 
   
   /* Configure ADCGIRQENx */ 
   ADCGIRQEN1 = 0;
   ADCGIRQEN1bits.AGIEN12 = 1; // Enable interrupts
   ADCGIRQEN1bits.AGIEN13 = 1; // Enable interrupts
   ADCGIRQEN2 = 0;
  
   /* Configure ADCCSSx */ 
   ADCCSS1 = 0; // Clear all bits 
   ADCCSS2 = 0; 
   ADCCSS1bits.CSS12 = 1; // AN12 set for scan 
   ADCCSS1bits.CSS13 = 1; // AN13 set for scan 
   
   /* Configure ADCCMPCONx */ 
   ADCCMPCON1 = 0; // No digital comparators are used. Setting the ADCCMPCONx 
   ADCCMPCON2 = 0; // register to '0' ensures that the comparator is disabled. 
   
   /* Configure ADCFLTRx */ 
   ADCFLTR1 = 0; // No oversampling filters are used. 
   ADCFLTR2 = 0; 
   
   /* Set up the trigger sources */ 
   ADCTRG4bits.TRGSRC12 = 3; // Set AN0 (Class 1) to trigger from scan source 
   ADCTRG4bits.TRGSRC13 = 3; // Set AN8 (Class 2) to trigger from scan source
   
   /* Early interrupt */ 
   ADCEIEN1 = 0; // No early interrupt 
   ADCEIEN2 = 0;
   
   /*
    Step 3: The user sets the ANENx bit to ?1? for the ADC 
    SAR Cores needed (which internally in the ADC module 
    enables the control clock to generate by division the 
    core clocks for the desired ADC SAR Cores, which in turn 
    enables the bias circuitry for these ADC SAR Cores).
   */
   
   /* Enable clock to analog circuit */ 
//   ADCANCONbits.ANEN0 = 1; // Enable the clock to analog bias ADC0 
   ADCANCONbits.ANEN7 = 1; // Enable, ADC7
   
   /*
    Step 4: The user sets the ON bit to ?1?, which enables 
    * the ADC control clock.
   */
   
   /* Turn the ADC on */ 
   ADCCON1bits.ON = 1;
   
   /*
    Step 5: The user waits for the interrupt/polls the
    BGVRRDY bit (ADCCON2<31>) and the WKRDYx
    bit (ADCANCON<15,13:8>) = 1, which signals that
    the device analog environment (band gap and VREF)
    is ready.
   */
   
   while(!ADCCON2bits.BGVRRDY); // Wait until the reference voltage is ready 
   while(ADCCON2bits.REFFLT); // Wait if there is a fault with the reference voltage
   
   /* Wait for ADC to be ready */ 
//   while(!ADCANCONbits.WKRDY0); // Wait until ADC0 is ready 
   while(!ADCANCONbits.WKRDY7); // Wait until ADC7 is ready
   
   /*
    Step 6: Set the DIGENx bit (ADCCON3<15,13:8>) to
    ?1?, which enables the digital circuitry to immediately
    begin processing incoming triggers to perform data
    conversions. 
   */
   
   /* Enable the ADC module */ 
//   ADCCON3bits.DIGEN0 = 1; // Enable ADC0 
   ADCCON3bits.DIGEN7 = 1; // Enable ADC7
}

/**************************************************************************
  Function
     ReadADCValues

 Parameters
     uint32_t* Results: a pointer to a vector which the ADC results will be
                        placed

 Returns
     None

 Description
     Places the readings from the ADC into Results
 Notes
 
 *************************************************************************/
void ReadADC(uint16_t *Results){
    
    ADCCON3bits.GSWTRG = 1; // Trigger a conversion
    
    while (ADCDSTAT1bits.ARDY12 == 0); 
    Results[0] = ADCDATA12; // Wait the conversions to complete
    
    while (ADCDSTAT1bits.ARDY13 == 0); 
    Results[1] = ADCDATA13; // fetch the result
}