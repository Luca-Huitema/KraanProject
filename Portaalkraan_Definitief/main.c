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


int knop1_ingedrukt = 0;
int knop2_ingedrukt = 0;
int knop3_ingedrukt = 0;

int MenuLayer = 0;
int StepCounterY = 255;
int StepCounterX = 255;
int SubMenu = 0;

char DisplayState = 'None';
int Lock = 1;

void init (void)
{
    init_h_bridge();                        //MAMMA MIA I MAKE-A DA MOTORS WORKE
/*    DDRB = ~((PB7 | PB6 | PB5 | PB4) );     //Lampjes declaration
    PORTB |= (1<< (PB7 | PB6 | PB5 | PB4) ); //Turn thenm off? */
    DDRF &= ~(1<< (PF1 | PF2 | PF3) );      //Config Btn
    DDRC |= ~(1<< (PC3 | PC5 | PC6 | PC4 | PC2) ); //Declare de pins voor de microswitches
    DDRA |= ~(1<< (PA0 | PA2 | PA4 | PA1)); // Config the Relais ports (X,Y,Z, Magnet.)
    PORTA |= (1<< (PA0 | PA2 | PA4 | PA1))    ;//Zet ze standaard uit?



	// Initialiseer de pinnen voor datain (D8=PH5), shiftclk (D7=PH4) en latchclk (D4=PG5) als output
	DDRH |= (1 << PH5) | (1 << PH4);
	DDRG |= (1 << PG5);

	// Maak shiftclk en latchclk laag
	PORTH &= ~(1 << PH4);
	PORTG &= ~(1 << PG5);

	TCCR4A = 0;								// set entire TCCR1A register to 0
	TCCR4B = 0;								// same for TCCR1B
	TCNT4 = 0;								// initialize counter value to 0



	//LADS. WE GOT IT. TICKETS FOR SPONGEBOB THE MOVIE- ERR, THE VALUE FOR THE TIMER. IT'S TCNT4!!! ABORT IT'S NOT IT. ABORT.



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
//Bigass timer that [KEEPS [count]ING UP!]... Wait a sec can you just get the value of the timer itself???? THat would be easy to modulo and sheesh..
//do that thing where you get two values for the thing like 1 or 0 for odd and even
//switch case for x, y, and static.
//if divided value is odd, then just like, display the full one. If even then do the blink version.
switch (DisplayState){

    case 'All':
        display(0xE3, 0);
        display(0xEF, 1);
    break;

    case 'X':
        if(OCR4A % 10 == 0) //I severely hope that this does the dimming / blinking thing i hope it does.
                            //UPDATRE BINCH, IT WASN'T THE TIMER VALUE. GET THAT TO WORK OR YAH FIYAHED!
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

        }

		display(display_set[o], 3);
//		display(display_set[t], 2); TOTALLY not needed right now. We only count from 0 to 9 after all!.. Maybe i should also cut the 0...

}





int main(void)
{
	init();
while(1)
{


/*
    if ((PINF & (1 << PF1)) == 0)
        {
            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(60);
                MenuLayer -= 1;
            }
        }

    else
        {
        if (knop1_ingedrukt != 0) // knop is zojuist losgelaten
            {
                _delay_ms(60);
                knop1_ingedrukt = 0;
            }
        }



    if ((PINF & (1 << PF2)) == 0)
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(60);
                MenuLayer = 0;
            }
        }

    else
        {
        if (knop2_ingedrukt != 0) // knop is zojuist losgelaten
            {
                _delay_ms(60);
                knop2_ingedrukt = 0;
            }
        }



    if ((PINF & (1 << PF3)) == 0)
        {
            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(60);
                MenuLayer += 1;
            }
        }

    else
        {
        if (knop3_ingedrukt != 0) // knop is zojuist losgelaten
            {
                _delay_ms(60);
                knop3_ingedrukt = 0;
            }
        }
*///miracle cODE








	switch(MenuLayer) {
	case 0: //Return to origin and suches.
    DisplayState = 'None';

    if ((PINF & (1 << PF3)) == 0)
        {
            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(60);
                MenuLayer += 1;
            }
        }

    else
        {
        if (knop3_ingedrukt != 0) // knop is zojuist losgelaten
            {
                _delay_ms(60);
                knop3_ingedrukt = 0;
            }
        }



        h_bridge_set_percentage(0); //Mario's freakin' dead.
        PORTA = (PA0 | PA2 | PA4 | PA1); //All Relais Off.
        PORTA = ~(1 << PA1);//Make magnet un-magnetise. (Otherwise kan het tonnen nog per ongeluk meenemen.)
                switch(SubMenu){

                case 0:
                    n=1; Refresh();

                    PORTA = ~(1 << PA4);//Switch relais naar Z-as.
                    h_bridge_set_percentage(10); //Zet h-brug voor Z-As
                        if (PINC & (1 << PC2)) {
                            h_bridge_set_percentage(0);
                            SubMenu += 1;
                            PORTA = (1 << PA4);//Switch Relais off of Z-as.
                            }

                break;

                case 1:
                    n=2; Refresh();
                    PORTA = ~(1 << PA0);//Switch relais naar X-as. (eigenlijk Y-as)
                    h_bridge_set_percentage(100); //Zou 12 volt moeten zijn. Check if this works and sheesh.
                        if (PINC & (1 << PC6)) {
                            h_bridge_set_percentage(0);
                            SubMenu += 1;
                            PORTA = (1 << PA0);//Switch Relais off of X-as.
                            }

                break;

                case 2:
                    n=3; Refresh();
                    PORTA = ~(1 << PA2);//Switch relais naar Y-as. (Eogenijk X-as)
                    h_bridge_set_percentage(10); //Zou 6 volt moeten zijn. Check if this works and sheesh.
                        if (PINC & (1 << PC4)) {
                            h_bridge_set_percentage(0);
                            SubMenu += 1;
                            MenuLayer += 1;
                            PORTA = (1 << PA2);//Switch Relais off of Y-as.
                            n = 0;
                            }


                    StepCounterX = 0; //Just, y'know, desinfecting your hands and sanitising before you go!
                    StepCounterY = 0;

                    break;
                    }


                    break;




case 1://Basic coordinate selection (X-axis)
	    DisplayState = 'X';
    if ((PINF & (1 << PF1)) == 0)
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


    if ((PINF & (1 << PF3)) == 0)
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


    if ((PINF & (1 << PF2)) == 0) //Confirm thing is here later in the code since it would lead to less cases of the ammount of steps and steps displayed being mismatched if a +1 or -1 were pressed simultaniously with the confirm button. Order of operations and stuff.
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
                StepCounterX=n;
                MenuLayer += 1;
                n=0; //New axis starts from 0.
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

        Refresh();

    break;

case 2: //Basic coordinate selection (Y-axis)
//Yet again. Once timers are a known concept and you know how the ones already used work make this blink.
    DisplayState = 'Y';
    if ((PINF & (1 << PF1)) == 0)
        {
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


    if ((PINF & (1 << PF3)) == 0)
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


    if ((PINF & (1 << PF2)) == 0) //Confirm thing is here later in the code since it would lead to less cases of the ammount of steps and steps displayed being mismatched if a +1 or -1 were pressed simultaniously with the confirm button. Order of operations and stuff.
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
                StepCounterY=n;
                MenuLayer += 1;
                n=StepCounterX;// here so that it doesn't accidenttally show the previous value from selecting the x-axis, but instead the real selected value for X
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

        Refresh();
    break;


    case 3:
        switch(SubCase2){

        case 0:
        PORTA = ~(1 << (PA1|PA4));//Turn Magsafe on (Magnet stops working like magnet now just to make it not snap and jumpscare ya.) (Oh yeah ook zet het nu de Z as aan.)
        h_bridge_set_percentage(10);
        _delay_ms(1500);//Yeag replace this with a timer instead in the final release.
        h_bridge_set_percentage(0);
        PORTA = (1 << (PA1|PA4));//Turn Magsafe off (Magnet just works again, also saves on energy.)
        SubCase2 += 1
        break;

        case 1:
            PORTA = ~(1 << PA4);//(Oh yeah ook zet het nu de Z as aan.)
            h_bridge_set_percentage(-10);

                if (PINC & (1 << PC2)) {
                    h_bridge_set_percentage(0);
                    Lock = 0;
                    PORTA = (1 << PA4);//Switch Relais off of Z-as.
                    MenuLayer += 1;
                    }
        break;


        }




    break;

    case 4://Movement Y-AXIS.
    DisplayState = 'Y';

    PORTA = ~(1 << PA2);
    if(StepCounterY <= 1)
    {
        h_bridge_set_percentage(-12); //Deze zijn negatief sinds dat van de eindswitch af gaat.
    }
    else
    {
         h_bridge_set_percentage(-25);
    }


    if ((PINC & (1 << PC5)) == 0)
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
        MenuLayer += 1;
        PORTA = (1 << PB6);
        h_bridge_set_percentage(0);
    }
        n=StepCounterY;// Here for display purposes. Handy Debug Tool.
        Refresh();
    break;


    case 5://Movement X-AXIS.

    DisplayState = 'X';
    PORTA = ~(1 << PA0);

    if(StepCounterX <= 1)
    {
        h_bridge_set_percentage(30); //Hier zodat het met grootere accuraatheid bij zijn plek komt.
    }
    else
    {
         h_bridge_set_percentage(75);
    }

    if ((PINC & (1 << PC3)) == 0)
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
        PORTA = (1 << PA0); //Yet-again first h-bridge and THEN the port. Net zoals je eigenlijk een laptop accu moet ontkoppelen.
        MenuLayer += 1;
    }
        n=StepCounterX;// Here for display purposes. Handy Debug Tool.
        Refresh();
    break;

    case 6:

        h_bridge_set_percentage(10);
        _delay_ms(1500);//Yeag replace this with a timer instead in the final release.
        h_bridge_set_percentage(0);
        PORTA = ~(1 << (PA1|PA4));//Turn Magsafe on (Magnet stops working like magnet now just to make it not snap and jumpscare ya.) (Oh yeah ook zet het nu de Z as aan.)


            PORTA = ~(1 << PA4);//(Oh yeah ook zet het nu de Z as aan.)
            h_bridge_set_percentage(-10);
            for (int Lock = 1; Lock == 1; ) //Movement back to origin for z-as (Op en neer)
            {
                if (PINC & (1 << PC2)) {
                    h_bridge_set_percentage(0);
                    Lock = 0;
                    PORTA = (1 << PA4);//Switch Relais off of Z-as.
                    MenuLayer += 0;
                    }
            }
    break;

    default: {MenuLayer = 0;} break;

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
