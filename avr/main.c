#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define baudrate 9600L
#define F_CPU 8000000L
#define bauddivider (F_CPU/(16 * baudrate) - 1)
#define HI(x) ((x) >> 8)
#define LO(x) ((x)& 0xFF)

//���� ������������� � ���, ��� ������� �������
volatile char arriveCom = 0;
//���� ��������� ������� ��������
//����������� ������ 2 �������
//�� ��������� ������, ����� ��� ������ ��������� ������ ���������� �����
volatile char timeFlag = 1;

volatile char keyFlag = 0;

volatile char lightFlag = 1;

volatile char notSend = 0;


//�������� ����������� �� ����� ����������
volatile int tcnt = 2 * F_CPU / 1024 / 256;

//��������� �� ��������� ���� � ������
unsigned char * RAMPointer;

//������ ������ ��� ����������� �������
#define bufSize 40
//����� ����������� �������
char buf[bufSize];
//������ ������
unsigned short i_buf = 0;

#define mesLen 40
//����� ��������� ��� ���������
char message[mesLen];
//������ ���������
unsigned short i_mes = 0;


//������������ �������
void sendByte(unsigned char value)
{
	char bufer[1000];
	sprintf(bufer, "adress = %d, value = %d\n\r", RAMPointer, value);
	sendStr(bufer);
};

void sendStr(char *string){
	strcpy(message, string);
	i_mes=0;		// ���������� ������
	UDR1 = message[0];	// ���������� ������ ����
	UCSR1B|=(1<<UDRIE1);// ��������� ����������� UDRE
}

/*
	������� ������ �� ������� �� ������� ���������� �������� 
	���������
*/
void nextAdr()
{
	if ( RAMPointer++ > 4096 )
	{
		RAMPointer = 0;
	}
}

void prevAdr()
{
	if ( RAMPointer-- < 0 )
	{
		RAMPointer = 4095;
	}
}

//������ ����������

//���������� ���������� �� ������� ����� � ����
ISR (USART1_RX_vect)
{
	//��������� ����
	unsigned char byte;

	//������ ����
	byte = UDR1;
	//��� ����� ����
	UDR1 = byte;

	//���� ������ ������� enter, �� ���� ������� ��������
	if (byte == 0x0d)
	{
		arriveCom = 1;
	}
	else
	{
		//���������� �������� ���� � ������
		if ( i_buf < mesLen )
		{
			buf[i_buf++] = byte;	
		}
	}
}

//����������� �� ����������� ������� USART
ISR (USART1_UDRE_vect)
{
	i_mes++;			
 
 	//������ ���� �����?
	if( message[i_mes] == '\0' )
	{ 
		// ��������� ���������� �� ����������� - �������� ���������
		UCSR1B &=~(1<<UDRIE1);
	}
	else 
	{
		// ����� ������ �� �������.
		UDR1 = message[i_mes];
	}
}


//���������� �� ������������ �������
ISR(TIMER0_OVF_vect)
{
	tcnt--;
	if (tcnt == 0)
	{
		//��������� ���� ��������� 2-� ���������� ���������
		timeFlag = 1;
		//����� ����������� ������� ��� ������ ��������� 2-� ������
		tcnt = 10;
	}
}


int main(void) 
{

	//��������� ����������
	//�������� ��� ������ 
	int i, j, k = 0, n = 0;
	
	//�������� ����� �� ������
	unsigned char value;


	//����������� ������������ � ������� ����� �������� 
	//�������� 1,2,3 ����� � ��� ������
	DDRE = (1 << PE1) | (1 << PE2) | (1 << PE3);
	//�������� 4,6 ����� B ��� �����
	//(������)
	DDRB = (0<<PB5) | (0<<PB6);
	//������������� ��������� ��� ����� B
	PORTB = (0<<PB5) | (0<<PB6);

	//������������� ������
	TCCR0 = 0b00000111;
	TIMSK = (1 << TOIE0);

	//����������� ���� �� �����/�������� ������
	//������������� �������� ��������
	UBRR1L = LO(bauddivider);
	UBRR1H = HI(bauddivider);
	//������� ������� ����
	UCSR1A = 0;
	//��������� ������ ����������� � ���������
	//���������� �� ���������� ������
	//� ��������� ���������� �� ���������� ��������
	UCSR1B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE1) | (0 << TXCIE1);
	//����������� ����� ���������������� �����
	//������ ������ 8 ��� 1 ����-���
	UCSR1C = (0<<UMSEL1)|(1<<USBS1)|(1<<UCSZ10)|(1<<UCSZ11);

	//��������� ���������� ���������
	sei();

	//��������� ������ ���
	set_sleep_mode ( SLEEP_MODE_IDLE );


	//�������� � ������ ������ ����� ��� �������� ������������ ������ ���������
	unsigned char data[256];
	//�������� ��� ������� �� 0 �� 255
	for (i = 0; i < 256; i++)
	{
		data[i] = i;
	}
	//������������� ��������� �� ������ ����� �������
	RAMPointer = data;


	//������� ���� ���������
	for (;;)
	{
		//�������� �������� �� ������
		value = *RAMPointer;

		if ( keyFlag )
		{
			//����������� ����������
			scanKey();
		}
		//���� ����� ��� �� ���������� ����� ����
		if ( notSend )
		{
			//���������� �� ��
			sendByte(value);
			notSend = 0;
		}
		
		if ( notSend == 0 )
		{
			//���� ������ 6 ������
			if ( n == 2 )
			{
				nextAdr();
				n = 0;
				notSend = 1;
			}
			//������� ������� ��������
			if ( timeFlag )
			{
				if ( lightFlag )
				{
				//���������� ����� �� �����������
					//����� ������ �������� �� ����������
					switch (k)
					{
						case 0:
							//������� ������ 3 ����
							PORTE = (value & 0b10000000) << PE1 | (value & 0b01000000) << PE2 
										| (value & 0b00100000) << PE3;
							k = 1;
							break;
						case 1:
							//������� ������ 3 ����
							PORTE = (value & 0b00010000) << PE1 | (value & 0b00001000) << PE2 
										| (value & 0b00000100) << PE3;
							k = 2;
							break;
						case 2:
							//������� ��������� 2 ���� � ��� ��������
							PORTE = (value & 0b00000010) << PE1 | (value & 0b00000001) << PE2 
										| (value & 0b00100000) << PE3;
							k = 0;
							break;
					}
				}
				timeFlag = 0;
				n++;
			}
		}

		//���� ������� ������� �� ����
		if ( arriveCom )
		{
			//���������� ����
			arriveCom = 0;
			//������������ �������
			exeCom();
		}
		//����� ���� ��� ��� �������� ��������� ��������
		sleep_mode();
		
	}
	return 0;
}



void exeCom()
{
	char bufer[4];
	unsigned char i, adr;
	if ( strncmp(buf, "key_on", 6) == 0 )
	{
		keyFlag = 1;
	}
	else if ( strncmp(buf, "key_off", 7) == 0)
	{
		keyFlag = 0;
	}
	else if ( strncmp(buf, "light_on", 8) == 0)
	{
		lightFlag = 1;
	}
	else if (strncmp(buf, "light_off", 9) == 0)
	{
		lightFlag = 0;
	}
	else if (strncmp(buf, "get", 3) == 0)
	{
		//��������� ������������ ������
		for (i = 0; i < 4; i++)
		{
			bufer[i] = buf[i+4];
		}
		adr = atoi(bufer);
		if ( adr > 0 && adr < 4096)
		{
			RAMPointer = adr;
		}
		else 
		{
			sendStr("bad arddress\n\r");
		}
	}
	else 
	{
		sendStr("undefined command\n\r");
	} 

	//��������� ����������� �����
	//���������� �� ���� ����������� �����
	
	return;
}


void scanKey()
{
	//��������� �������� �� �����
	int byte = PINB;

	if (byte & 0b00100000)
	{
		//���� ������ ������ �� ����� 5 ����� �
		nextAdr();
		notSend = 1;
	}
	else if (byte & 0b01000000)
	{
		//���� ������ ������ �� ����� 6 ����� �
		prevAdr();
		notSend = 1;
	}
}




