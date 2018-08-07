#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "main.h"

void Init_IO ( void )
{
	DDRB = _BV(DDB1);
}

int main (void)
{
	cli();
	Init_IO();
	sei();
	while (1)
	{
	}
}

INTERRUPT (SIG_OVERFLOW1)
{
	TCNT1=TIME_INIT_MS;
	invert_bit (PORTB,PB1);
	TIFR=0;
}