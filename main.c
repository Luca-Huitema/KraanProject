
#include <avr/io.h> // voor gebruik DDRx
#include <util/delay.h> // voor delay_ms()

int main(void)
{
    DDRC &= (1<< (PC7 | PC5 | PC6 | PC4 | PC2) ); //X-as detect, Y-as Detect, X,Y en Z as eindstop
    DDRB |= (1<< (PB7 | PB6 | PB5 | PB4) ); // Config the 4 LEDs
    PORTB |= (1<< (PB7 | PB6 | PB5 | PB4) )    ;//put em on??
    // Insert code
    int knop2_ingedrukt = 0;

while (1)
    {


    if ((PINC & (1 << PC7)) == 0)
        {
            if (knop2_ingedrukt == 0) // knop is niet al eerder ingedrukt
            {
                _delay_ms(100);
                PORTB ^= (1 << PB7); //Inverts the boio
                knop2_ingedrukt = 1;
            }
        }

    else
        {
        if (knop2_ingedrukt != 0) // knop is zojuist losgelaten
            {
                _delay_ms(100);
                knop2_ingedrukt = 0;
            }
        }


    }

return 0;
}
