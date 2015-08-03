/* Cytron Technologies SDN. BHD
 * Project: HC-SR04
 * Author: Tony Ng Wei Kang
 *
 * Controller: PIC16F887
 * Complier: MPLABX + XC8
 * Created on November 14, 2013, 12:17 PM
 */

#include <xc.h>

//=============================================================================
//	Configuration Bits
//=============================================================================
#pragma	config FOSC = HS	// HS oscillator
#pragma	config FCMEN = OFF	// Fail-Safe Clock Monitor disabled
#pragma	config IESO = OFF	// Oscillator Switchover mode disabled
#pragma	config PWRTE = OFF	// PWRT disabled
#pragma	config BOREN = OFF	// Brown-out Reset disabled in hardware and software
#pragma	config WDTE = OFF	// WDT disabled (control is placed on the SWDTEN bit)
#pragma	config MCLRE = ON	// MCLR pin enabled; RE3 input pin disabled
#pragma	config LVP = OFF	// Single-Supply ICSP disabled

#define _XTAL_FREQ  20000000

#define rs          PORTBbits.RB4
#define e           PORTBbits.RB5
#define	lcd_data    PORTD

#define	LED1	PORTBbits.RB6
#define	LED2	PORTBbits.RB7

#define Trig    PORTAbits.RA0
#define Echo    PORTBbits.RB0
#define sw2     PORTBbits.RB1

//==========================================================================
//	function prototype
//==========================================================================
void send_config(unsigned char data);
void send_char(unsigned char data);
void lcd_goto(unsigned char data);
void lcd_clr(void);
void send_string(const char *s);
void lcd_bcd(unsigned char uc_digit, unsigned int ui_number);

void interrupt ISRHigh(void);

void delay_ms(unsigned int ui_value);

//==========================================================================
//	main function
//==========================================================================
void main(void)
{
    PORTA = 0;		// Clear PORT
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;

    TRISA = 0b00000000;		// set PORTA AS output
    TRISB = 0b00000011;         // set PORTB AS output
    TRISC = 0b00000000;		// set PORTC AS output
    TRISD = 0b00000000;		// set PORTD AS output
    TRISE = 0b00000000;		// set PORTD AS output

    ANSEL = 0;                  // set PORTA as Digital I/O for PIC16F887
    ANSELH = 0;                 // set PORTB as Digital I/O for PIC16F887

    send_config(0b00000001);    //clear display at lcd
    send_config(0b00000010);    //lcd return to home
    send_config(0b00000110);    //entry mode-cursor increase 1
    send_config(0b00001100);    //display on, cursor off and cursor blink off
    send_config(0b00111000);

// Turn on Timer 1 with 4x prescale as counter
    T1CON = 0b00100000;

// Turn on PORTC and Global interrupt
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.RBIE = 1;
    INTCONbits.RBIF = 0;

// Configuration for Interrupt-On_Change for PORTB0
    IOCBbits.IOCB0 = 1;

    lcd_clr();
    lcd_goto(00);
    send_string("Distance");
    lcd_goto(20);
    send_string("   CM    Inch");
    
    while(1)
    {
        TMR1H = 0;          // Clear Timer1
        TMR1L = 0;

        if(Echo == 0)       // If no signal recieved from Ultrasonic
        {
            Trig = 0;
            __delay_us(2);
            Trig = 1;
            __delay_us(10); // Send LOW-to-HIGH Pulse of 10us to Ultrasonic
            Trig = 0;       
        }
     }
}

//========================================================================
//		Interrupt Subroutine (Interrupt pin is HIGH)
//========================================================================
void interrupt ISRHigh(void)
{
    unsigned long duration = 0;
    unsigned int distance_cm = 0, distance_inc = 0, TMR = 0;

    T1CONbits.TMR1ON = 1;       // ON Counter
    while(Echo == 1);
    T1CONbits.TMR1ON = 0;

    TMR = (unsigned int) TMR1H << 8;
    TMR = TMR + TMR1L;          // Combine 2x counter byte into single integer

    duration = (TMR/10) * 8;  // Duration Formula = TMR * 0.2us(Clock speed) * 4 (Timer Prescale)

    distance_cm = duration / 58 ;   // Refer HC-SR04 Datasheet
    distance_inc = duration / 148;

    if(distance_cm < 400 && distance_inc < 157) // If Ultrasonic sense more than 400cm
    {
        lcd_goto(20);
        lcd_bcd(3,distance_cm);
        lcd_goto(26);
        lcd_bcd(3,distance_inc);
        delay_ms(200);
    }
    else
    {
        lcd_goto(20);
        send_string("***");
        lcd_goto(26);
        send_string("***");
    }
    
    INTCONbits.RBIF = 0;        // Clear PortB Interrupt Flag
}

//========================================================================
//				Delay FUNCTION
//========================================================================
void delay_ms(unsigned int ui_value)
{
    while (ui_value-- > 0)
    {
        __delay_ms(1);	// macro from HI-TECH compiler which will generate 1ms delay base on value of _XTAL_FREQ in system.h
    }
}

//========================================================================
//				LCD FUNCTION
//========================================================================
//send lcd configuration
void send_config(unsigned char data)
{
    rs = 0; //set lcd to configuration mode
    lcd_data = data; //lcd data port = data
    e = 1; //pulse e to confirm the data
    delay_ms(50);
    e = 0;
    delay_ms(50);
}

//send lcd character
void send_char(unsigned char data)
{
    rs = 1;             //set lcd to display mode
    lcd_data = data;    //lcd data port = data
    e = 1;              //pulse e to confirm the data
    delay_ms(10);
    e = 0;
    delay_ms(10);
}

//set the location of the lcd cursor
void lcd_goto(unsigned char data)
{                                   //if the given value is (0-15) the
    if (data < 16)                  //cursor will be at the upper line
    {                               //if the given value is (20-35) the
        send_config(0x80 + data);   //cursor will be at the lower line
    }                               //location of the lcd cursor(2X16):
    else                            // -----------------------------------------------------
    {                               // | |00|01|02|03|04|05|06|07|08|09|10|11|12|13|14|15| |
        data = data - 20;           // | |20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35| |
        send_config(0xc0 + data);   // -----------------------------------------------------
    }
}

//clear the lcd
void lcd_clr(void)
{
    send_config(0x01);
    delay_ms(600);;
}

//send a string to display in the lcd
void send_string(const char *s)
{
    unsigned char i = 0;
    while (s && *s)send_char(*s++);
}

void lcd_bcd(unsigned char uc_digit, unsigned int ui_number)
{
    unsigned int ui_decimal[5] ={ 0 };
//extract 5 single digit from ui_number
    ui_decimal[4] = ui_number/10000;        // obtain the largest single digit, digit4
    ui_decimal[3] = ui_number%10000;        // obtain the remainder
    ui_decimal[2] = ui_decimal[3]%1000;
    ui_decimal[3] = ui_decimal[3]/1000;     // obtain the 2nd largest single digit, digit3
    ui_decimal[1] = ui_decimal[2]%100;
    ui_decimal[2] = ui_decimal[2]/100;      // obtain the 3rd largest single digit, digit2
    ui_decimal[0] = ui_decimal[1]%10;       // obtain the smallest single digit, digit0
    ui_decimal[1] = ui_decimal[1]/10;       // obtain the 4th largest single digit, digit1

    if (uc_digit > 5) uc_digit = 5;         // limit to 5 digits only
    for( ; uc_digit > 0; uc_digit--)
    {
        send_char(ui_decimal[uc_digit - 1] + 0x30);
    }
}