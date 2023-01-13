#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "h_bridge.h"
#include "h_bridge.c"
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
int StepBack = 0;

char DisplayState = 'None';

void init (void)
{
    init_h_bridge();                        //MAMMA MIA I MAKE-A DA MOTORS WORKE
    DDRB = ~((PB7 | PB6 | PB5 | PB4) );     //REVOLUTIONARY NEW WAY OF DECLARING THE PORT BAYBEE
    PORTB |= ~((PB7 | PB6 | PB5 | PB4) );
    DDRF &= ~(1<< (PF1 | PF2 | PF3) );      //Config Btn

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
	reset();    //Moves everything back in case it stopped prematurely last time.
}

void reset (void)
{
/*
Big gumby's testing grounds
*/
    h_bridge_set_percentage(0); //Mario's freakin' dead.

    bool Lock = true;       //These lads make the waiting for the end switch work. Please treat them with care.
    bool LockTsu = true;
    bool LockSun = true;

    PORTB = ((PB7 | PB6 | PB5 | PB4) ); //als het klaar is vervang dit met turn all relais off.
    PORTB = ~(1 << PB4);//Make magnet un-magnetise.
    PORTB = ~(1 << PB6);//Switch relais naar Z-as.
    h_bridge_set_percentage(100); //Zet hbrug aan


    while (Lock == true) //Movement back to origin for z-as (Op en neer)
    {
        if (/*just check if the end switch got pressed this time*/)
            h_bridge_set_percentage(0);
            Lock = false;
    }
    PORTB = (1 << PB6);//Switch Relais off of Z-as.


    PORTB = ~(1 << PB5);//Switch relais naar Y-as.
    while (Lock == true) //Movement back to origin for y-as (Voorwaarts en actherwaards)
    {
        if (/*just check if the end switch got pressed this time*/)
            h_bridge_set_percentage(0);
            Lock = false;
    }
    PORTB = (1 << PB5);//Switch Relais off of Y-as.


    PORTB = ~(1 << PB7);//Switch relais naar X-as.
    while (LockTsu == true) //Movement back to origin for x-as (Links en rechts)
    {
        if (/*just check if the end switch got pressed this time*/)
            h_bridge_set_percentage(0);
            LockTsu = false;
    }
    PORTB = (1 << PB7);//Switch Relais off of X-as.


    MenuLayer = 0; //Just a hard-reset for the menus.
    StepCounterX = 0;
    StepCounterY = 0;
    n = 0;
    knop2_ingedrukt = 1;//Doet dit omdat het anders de knop weer als ingedrukt ziet.
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
        display(0xEF, 0);
        display(0xEF, 1);
    break;

    case 'Y':
        display(0xF3, 0);
        display(0xFF, 1);
    break;

    case 'None'
        display(0xFF, 0);
        display(0XFF, 1);
    break;
}

		display(display_set[o], 3);
//		display(display_set[t], 2); TOTALLY not needed right now. We only count from 0 to 9 after all!.. Maybe i should also cut the 0...

}



//BIG BREAKTHROUGH: THose _delay_() mfers made the display flicker / go blank for a sec. So they're now as small as possible without being innacurate.

int main(void)
{
	init();

	while (1) {
	switch(MenuLayer) {

case 0: //Basic coordinate selection (X-axis)
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


                if (n<9) //Just here to make sure we do [] go into the [pos]itives, since those [] work.
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




case 1: //Basic coordinate selection (Y-axis)
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


/*
    case 2: {
PORTB = ~(1 << PB4);//Turn Magsafe on (Magnet stops working like magnet now.)
//use servo to move arm down for a set amount of time.
PORTB = (1 << PB4);//Turn Magsafe off (Magnet just works again, also saves on energy.)
//Move up until it hits the bounding switch.
            }
    break;
*/


case 2://Movement X-AXIS.
//Also here the displays do the X and Y Axis Graphics for the GUI. NOTE TO SELF:
//                                                                              GET THIS BLINKING SO IT LOOKS GUI-ISH.

    DisplayState = 'X';
    PORTB = ~(1 << PB7); //When the code and stuff for the MOTOR is known, just replace this with TURN MOTOR ON.
    //Switch relais to X-as (PA0 / pin 22)
    h_bridge_set_percentage(100); //Oh hey mario
    if ((PINF & (1 << PF1)) == 0)
        {
            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
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

    if ((PINF & (1 << PF3)) == 0)
        {
            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
                  StepCounterX -=  1;
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



     if ((PINF & (1 << PF2)) == 0)
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                reset();//Guess what this does.
            }
        }

    if(StepCounterX == 0)
    {
        PORTB = (1 << PB7);         //Also just replace this one with turning the motor off. Just turn that same switch off.
        h_bridge_set_percentage(0); //Mario's freakin' dead.
    //Switch relais off of X-as (PA0 / pin 22)
        MenuLayer += 1;

        n=StepCounterY;// here so that it doesn't accidenttally show the previous value from [REMOVE]ing the [X]-axis, but instead the real selected value for [Y]
    }
        n=StepCounterX;// Here for display purposes. Handy Debug Tool.
        Refresh();
    break;



case 3://Movement Y-AXIS.
//Also here the displays do the X and Y Axis Graphics for the GUI. NOTE TO SELF:
//                                                                              GET THIS BLINKING SO IT LOOKS GUI-ISH.
    DisplayState = 'Y';

    PORTB = ~(1 << PB6); //Just imagine this is that really relais switch 2.
    h_bridge_set_percentage(100); //Oh hey mario
    //Turn LED 2 on




    if ((PINF & (1 << PF1)) == 0)
        {
            if (knop1_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
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

    if ((PINF & (1 << PF3)) == 0)
        {
            if (knop3_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
                  StepCounterY -=  1;
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



     if ((PINF & (1 << PF2)) == 0)
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(30);
                MenuLayer = 0; //Just a hard-reset in general.
                StepCounterY = 0;
                StepCounterX = 0;
                n = 0;
                PORTB = (1 << PB6);         //Relay 2 off.
                h_bridge_set_percentage(0); //Mario's freakin' dead.
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

    if(StepCounterY == 0)
    {
        MenuLayer += 1;
        PORTB = (1 << PB6);         //Also just replace this one with turning the motor off.
        h_bridge_set_percentage(0); //Mario's freakin' dead.
    }
        n=StepCounterY;// Here for display purposes. Handy Debug Tool.
        Refresh();
    break;


    case 4: {MenuLayer = 0;} break;
/*
    case 4: {
//use servo to move arm down
PORTB = ~(1 << PB4);//Turn Magsafe on (Magnet stops working like magnet now.)
//Move up
PORTB = (1 << PB4);//Turn Magsafe off (Magnet just works again, also saves on energy.)
            }
    break;
*/

/*
case 5://Movement back to reset position X. In actual make it run this bit first and THEN reset
    DisplayState = 'X';
        h_bridge_set_percentage(-75);   //Hello! It's a me, Bitch!
        PORTB = ~(1 << PB7);//switch to motor x
        if ((PINF & (1 << PF1)) == 0) //Replace this in the finished product with a simple switch so it knows when it's back. Also a smidge of "hey we gotta move back a bit" code.
        {
                _delay_ms(50);
                MenuLayer = 0;
                h_bridge_set_percentage(0); //Mario's freakin' dead.
                PORTB = (1 << PB7);//switch off of motor x
        }
    break;
*/

/*
case 6://Movement back to reset position Y. In actual make it run this bit first and THEN reset
    DisplayState = 'Y';
        h_bridge_set_percentage(-75);   //Hello! It's a me, Bitch!
        PORTB = ~(1 << PB6);//switch to motor y
        if ((PINF & (1 << PF1)) == 0) //Replace this in the finished product with a simple switch so it knows when it's back. Also a smidge of "hey we gotta move back a bit" code.
        {
                _delay_ms(50);
                MenuLayer = 0;
                h_bridge_set_percentage(0); //Mario's freakin' dead.
                PORTB = (1 << PB6);//switch off of motor y
        }
    break;
*/



/*
case 7: //Just reset everything to 0.
                MenuLayer = 0; //Just a hard-reset in general.
                StepCounterY = 0;
                StepCounterX = 0;
                n = 0;
    break;
*/
        }

	}
}

ISR(TIMER4_COMPA_vect)
{

    {
    o=n%10;
	//t=(n/10)%10;
    }


}
