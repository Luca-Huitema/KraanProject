#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "h_bridge.h"
#include "h_bridge.c"

//#include <stdbool.h> //I can't fucking believe that after all this time FREAKIN' BOOLEANS are not included by default in C...
// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
const char display_set[] = { 0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x19};
// 0xE3, 0xF3, 0xEF, 0xFF
// X en Y samen (L), Alleen Y-as (|), Alleen X-as (-), Empty ( ).
int n = 0;

char o = 0;
char t = 0;
char h = 0;
char th = 0;


//NOTICE: DE X EN Y HIERBIJ ZIJN ZOALS GEZIEN VANAF DE VOORKANT (WAAR DE OOGJES ZITTEN) VAN HET WERK. OFTERWEL, LEGAAL IS DE X Y EN DE Y X.

#define RelaisX PA0 //Pin 22
#define RelaisY PA2 //Pin 24
#define RelaisZ PA4 //Pin 26
#define RelaisMagnet PA1 //23

#define Xend PC6 //Pin 31
#define Yend PC4 //Pin 33
#define Zend PC2 //Pin 35
#define Xdetect PC3 //Pin 32
#define Ydetect PC5 //Pin 34

#define min PL0 //Pin 49
#define plus PL2 //Pin 47
#define swatch PL4 //Pin 45
#define confirm PL3 //pin 46

#define ReSwitch PD0 //pin SCL 21


int knop1_ingedrukt = 0;
int knop2_ingedrukt = 0;
int knop3_ingedrukt = 0;
int knop4_ingedrukt = 0;

int MenuLayer = 10;
int StepCounterY = 0;
int StepCounterX = 0;

char DisplayState = 'None';


void init (void)
{
    init_h_bridge();                        //MAMMA MIA I MAKE-A DA MOTORS WORKE
    DDRB &=  ~(1<<PB7)&~(1<<PB6)&~(1<<PB5)&~(1<<PB4); // Config the 4 LEDs
    PORTB |=  (1<<PB7)|(1<<PB6)|(1<<PB5)|(1<<PB4);//turn em off

    DDRA |= (1<< RelaisX) | (1<<RelaisY) | (1<<RelaisZ) | (1<<RelaisMagnet);    // Config the Relais ports (X,Y,Z, Magnet.)
    PORTA |= (1<< RelaisX) | (1<<RelaisY) | (1<<RelaisZ) | (1<<RelaisMagnet);   //Zet ze standaard uit?


    DDRL |= ~(1<< (min | plus | swatch | confirm) ); //Config CUSTOM buttons
    DDRD |= (1<< (ReSwitch) ); //Noodstop Reset Ding (Damn You Z-As!)
    DDRC |= ~(1<< (Xdetect | Ydetect | Xend | Yend | Zend) ); //Declare de pins voor de microswitches




	// Initialiseer de pinnen voor datain (D8=PH5), shiftclk (D7=PH4) en latchclk (D4=PG5) als output
	DDRH |= (1 << PH5) | (1 << PH4);
	DDRG |= (1 << PG5);

	// Maak shiftclk en latchclk laag
	PORTH &= ~(1 << PH4);
	PORTG &= ~(1 << PG5);

	TCCR4A = 0;								// set entire TCCR1A register to 0
	TCCR4B = 0;								// same for TCCR1B
	TCNT4 = 0;								// initialize counter value to 0

	OCR4A = 1500;							// = (16*10^6) / (1*1024) - 1 / 2 (must be <65536) set compare match register for 2hz increments
	TCCR4B |= (1 << WGM12);					// turn on CTC mode
	TCCR4B |= (1 << CS12) | (1 << CS10);	// Set CS12 and CS10 bits for 1024 prescaler
	TIMSK4 |= (1 << OCIE4A);				// enable timer compare interrupt

	sei();      // allow interrupts
}

void send_data(char data)
{
	char data_new = data;
	// Herhaal voor alle bits in een char
	for (char i = 0; i < 8; i++)
	{
		// Bepaal de waarde van de bit die je naar het schuifregister
		// wil sturen
		char mask = data_new & 1;

		// Maak de juiste pin hoog of laag op basis van de bepaalde waarde
		// van het bit
		if (mask > 0) {
			//send 1
			PORTH |= (1 << PH5);
		} else {
			//send 0
			PORTH &= ~(1 << PH5);
		}

		// Toggle shiftclk (hoeveel tijd moest het signaal minimaal hoog zijn?)
		PORTH |= (1 << PH4);
		PORTH &= ~(1 << PH4);

		data_new >>= 1;
	}
}

void send_enable(char num)
{
	send_data(0x80 >> num);
}

void display(char data, char num)
{
	send_data(data);
	send_enable(num);

	// Toggle latchclk (hoeveel tijd moest het signaal minimaal hoog zijn?)
	PORTG |= (1 << PG5);
	PORTG &= ~(1 << PG5);

}

void Refresh(void) //Refreshes the screen so new numbers can be shown. Better than calling it each time. TBh.
{
switch (DisplayState){

    case 'All':
        display(0xE3, 0);
        display(0xEF, 1);
    break;

    case 'X':
        if(OCR4A % 10 == 0)
        {
            display(0xEF, 0);
            display(0xEF, 1);
        }
        else
        {
            display(0xE3, 0);
            display(0xEF, 1);
        }
    break;

    case 'Y':
        if(OCR4A % 100 == 0) //Also yeah i had to do this this way since else the compiler would scream at me that there are two duplicate cases.
        {
            display(0xF3, 0);
            display(0xFF, 1);
        }
        else
        {
            display(0xE3, 0);
            display(0xEF, 1);
        }

    break;

    case 'None':
        display(0xFF, 0);
        display(0XFF, 1);
    break;

    case 'Err':
        display(0X00, 0); //Uppercase E
        display(0x00, 1); //Lowercase r
    break;

        }

		display(display_set[o], 3);
//		display(display_set[t], 2); TOTALLY not needed right now. We only count from 0 to 9 after all!.. Maybe i should also cut the 0...

}










int main(void)
{
    init();
    while(1){
        //Pre-switch code
                if (PIND & (1 << ReSwitch))  //Noodstop Ding
                {
                                _delay_ms(30);
                                StepCounterY=n;
                                MenuLayer = 10;
                                DisplayState = 'None';
                }



        	switch(MenuLayer) {

                case 10:{ //Home Z
                n=1; Refresh(); //Voor debug purposes, zet het display op: 1
                PORTA |= (1 << RelaisMagnet);//Magneet Grijpt niet meer
                PORTB ^= (1 << PB4); //Debug Light Magneet


                PORTA &= ~(1 << RelaisZ);//Switch relais naar Z-as.
                PORTB ^= (1 << PB5); //Debug Light Z-as
                h_bridge_set_percentage(10); //Zet h-brug voor Z-As
                    if (PINC & (1 << Zend)) {
                        h_bridge_set_percentage(0); //Als de switch geclicked wordt zet hbrug uit.
                        MenuLayer = 11; //was 11
                        PORTA |= (1 << RelaisZ);//Switch Relais off of Z-as.
                        PORTB ^= (1 << PB5); //Debug Light Z-as
                        }
                }break;

                case 11:{ // Home X
                n=2; Refresh();
                PORTA &= ~(1 << RelaisY);//Switch relais naar Y-as. (Eogenijk X-as)
                PORTB ^= (1 << PB6); //Debug Light Y-as
                h_bridge_set_percentage(-80); //Zou net wat minder dan 12 moeten zijn.

                    if (PINC & (1 << Xend)) {
                        h_bridge_set_percentage(0);
                        MenuLayer = 12;
                        PORTA |= (1 << RelaisY);//Switch Relais off of X-as.
                        PORTB ^= (1 << PB6); //Debug Light Y-as
                        }
                }break;

                case 12:{ //Home Y
                n=3;
                PORTA &= ~(1 << RelaisX);//Switch relais naar X-as. (Eogenijk y-as)
                PORTB ^= (1 << PB7); //Debug Light X-as
                h_bridge_set_percentage(-20); //Zou net wat minder dan 12 moeten zijn.

                    if (PINC & (1 << Yend)) {
                        h_bridge_set_percentage(0);
                        MenuLayer = 20;
                        PORTA |= (1 << RelaisX);//Switch Relais off of X-as.
                        PORTB ^= (1 << PB7); //Debug Light X-as
                        n=0;
                        }

                Refresh();
                }break;


                case 20:{ //Select X
                DisplayState = 'X';

                    if (PINL & (1 << min))
                        {
                            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                if (n>0) //Just here to make sure we don't go into the negatives, since those don't work.
                                    {
                                    n-=1;
                                    }
                                else {  //Wraparound Action baybee!
                                        n=8;
                                     }
                                knop1_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop1_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop1_ingedrukt = 0;
                            }
                        }




                    if (PINL & (1 << plus))
                        {
                            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);


                                if (n<8) //Just here to make sure we do [] go into the [pos]itives, since those [] work.
                                    {
                                    n+=1;
                                    }
                                else {  //Wraparound Action baybee!
                                        n=0;
                                     }
                                knop3_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop3_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop3_ingedrukt = 0;
                            }
                        }



                     if (PINL & (1 << swatch))  //switcher
                        {
                            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                StepCounterX=n;
                                MenuLayer = 25; //Switch to Y-as
                                n=StepCounterY; //New axis starts from 0.
                                knop2_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop2_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop2_ingedrukt = 0;
                            }
                        }



                     if (PINL & (1 << confirm))  //executeer
                        {
                            if (knop4_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                StepCounterX=n;
                                MenuLayer = 30; //Switch to execution of code
                                n=0; //New axis starts from 0.
                                knop4_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop4_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop4_ingedrukt = 0;
                            }
                        }




                Refresh();
                }break;

                case 25:{ //Select Y
                DisplayState = 'Y';
                    if (PINL & (1 << min))                        {
                            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                if (n>0)
                                    {
                                    n-=1;
                                    }
                                else {  //Wraparound Action baybee!
                                        n=9;
                                     }
                                knop1_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop1_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop1_ingedrukt = 0;
                            }
                        }


                    if (PINL & (1 << plus))
                        {
                            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);


                                if (n<9)
                                    {
                                    n+=1;
                                    }
                                else {  //Wraparound Action baybee!
                                        n=0;
                                     }
                                knop3_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop3_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop3_ingedrukt = 0;
                            }
                        }


                     if (PINL & (1 << swatch))  //switcher
                        {
                            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                StepCounterY=n;
                                n=StepCounterX; //New axis starts from 0.
                                MenuLayer = 20;
                                knop2_ingedrukt = 1;

                            }
                        }
                    else
                        {
                        if (knop2_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop2_ingedrukt = 0;
                            }
                        }


                     if (PINL & (1 << confirm))  //executeer
                        {
                            if (knop4_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);
                                StepCounterY=n;
                                MenuLayer = 30; //Switch to execution of code
                                n=0; //New axis starts from 0.
                                knop4_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop4_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop4_ingedrukt = 0;
                            }
                        }
                Refresh();
                }break;


                case 30:{ //Beneden om te pakken
                    PORTA |= (1<<RelaisX)|(1<<RelaisY)|(1<<RelaisZ)|(1<<RelaisMagnet);
                    PORTA &= ~(1 << RelaisZ)&~(1<<RelaisMagnet);//Z-as aan en magneet uit
                    PORTB ^= (1 << PB5); //Debug Light Z-as

                    h_bridge_set_percentage(-20);
                    _delay_ms(1500);//Yeag replace this with a timer instead in the final release.
                    h_bridge_set_percentage(0);

                    PORTA |= (1 << RelaisMagnet);//Magneet kleeft weer
                    PORTB ^= (1 << PB4); //Debug Light Magnet

                    MenuLayer = 35;
                }break;

                case 35:{ //omhoog met cargo
                    h_bridge_set_percentage(20);
                        if (PINC & (1 << Zend)) {
                            h_bridge_set_percentage(0);

                            PORTA |= (1 << RelaisZ);//Switch Relais off of Z-as.
                            PORTB ^= (1 << PB5); //Debug Light Z-as
                            MenuLayer = 40;
                            }
                }break;


                case 40:{ //go Y
                    DisplayState = 'Y';

                    PORTA &= ~(1 << RelaisX);
                    PORTB ^= (1 << PB6); //Debug Light Y-as
                    if(StepCounterY <= 1)
                    {
                        h_bridge_set_percentage(12); //Deze zijn [pos]atief sinds dat van de eindswitch af gaat.
                    }
                    else
                    {
                         h_bridge_set_percentage(25);
                    }


                    if ((PINC & (1 << Ydetect)) == 0)
                        {
                            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);//Fine-tune this also? do these switches need a different debouncing time or???
                                  StepCounterY -=  1;
                                knop1_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop1_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop1_ingedrukt = 0;
                            }
                        }

                    if(StepCounterY == 0)
                    {
                        MenuLayer = 50;
                        h_bridge_set_percentage(0);
                        PORTA |= (1 << RelaisX);
                        PORTB ^= (1 << PB6); //Debug Light Y-as

                    }
                        n=StepCounterY;// Here for display purposes. Handy Debug Tool.
                        Refresh();
                }break;


                case 50:{ //Go X
                    DisplayState = 'X';
                    PORTA &= ~(1 << RelaisY);
                    PORTB ^= (1 << PB7); //Debug Light X-as

                    if(StepCounterX <= 1)
                    {
                        h_bridge_set_percentage(30); //Hier zodat het met grootere accuraatheid bij zijn plek komt.
                    }
                    else
                    {
                         h_bridge_set_percentage(75);
                    }

                    if ((PINC & (1 << Xdetect)) == 0)
                        {
                            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
                            {
                                _delay_ms(30);//Fine-tune this also? do these switches need a different debouncing time or???
                                  StepCounterX -=  1;
                                knop1_ingedrukt = 1;
                            }
                        }
                    else
                        {
                        if (knop1_ingedrukt != 0) // knop is zojuist losgelaten
                            {
                                _delay_ms(30);
                                knop1_ingedrukt = 0;
                            }
                        }

                    if(StepCounterX == 0)
                    {
                        h_bridge_set_percentage(0);
                        PORTA = (1 << RelaisY); //Yet-again first h-bridge and THEN the port. Net zoals je eigenlijk een laptop accu moet ontkoppelen.
                        PORTB ^= (1 << PB7); //Debug Light X-as
                        MenuLayer = 60;
                    }
                        n=StepCounterX;// Here for display purposes. Handy Debug Tool.
                        Refresh();
                }break;


                case 60:{ //Back down
                    PORTA = ~(1 << (RelaisZ));//Z-as aan
                    PORTB ^= (1 << PB5); //Debug Light Z-as

                    h_bridge_set_percentage(20);
                    _delay_ms(1500);//Yeag replace this with a timer instead in the final release.
                    h_bridge_set_percentage(0);

                    PORTA = ~(1 << (RelaisMagnet));//Magneet laat los
                    PORTB ^= (1 << PB4); //Debug Light Magnet

                    MenuLayer = 65;
                }break;


                case 65:{
                    h_bridge_set_percentage(-20);
                        if (PINC & (1 << Zend)) {
                            h_bridge_set_percentage(0);

                            PORTA = (1 << RelaisZ);//Switch Relais off of Z-as.
                            PORTB ^= (1 << PB5); //Debug Light Z-as
                            PORTA = (1 << (RelaisMagnet));//Magneet kleeft weer (energy saving measure)
                            PORTB ^= (1 << PB4); //Debug Light Magnet
                            MenuLayer = 70;
                            }
                }break;


                case 70:{
                    MenuLayer = 10;
                }break;


                default:{
                   DisplayState='Err';
                   Refresh();
                }break;
        	}
    }

    return 0;
}


ISR(TIMER4_COMPA_vect)
{

    {
    o=n%10;
	//t=(n/10)%10;
    }


}

