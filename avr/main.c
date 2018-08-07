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

//флаг сигнализирует о том, что принята команда
volatile char arriveCom = 0;
//флаг истечения времени задержки
//поднимается каждые 2 секунды
//по умолчанию поднят, чтобы при старте программы начать показывать экшен
volatile char timeFlag = 1;

volatile char keyFlag = 0;

volatile char lightFlag = 1;

volatile char notSend = 0;


//значение вычисляется на этапе компиляции
volatile int tcnt = 2 * F_CPU / 1024 / 256;

//указатель на очередной байт в памяти
unsigned char * RAMPointer;

//размер буфера для поступающей команды
#define bufSize 40
//буфер поступающей команды
char buf[bufSize];
//индекс буфера
unsigned short i_buf = 0;

#define mesLen 40
//буфер сообщения для оператора
char message[mesLen];
//индекс сообдения
unsigned short i_mes = 0;


//используемые функции
void sendByte(unsigned char value)
{
	char bufer[1000];
	sprintf(bufer, "adress = %d, value = %d\n\r", RAMPointer, value);
	sendStr(bufer);
};

void sendStr(char *string){
	strcpy(message, string);
	i_mes=0;		// сбрасываем индекс
	UDR1 = message[0];	// отправляем первый байт
	UCSR1B|=(1<<UDRIE1);// разрешаем прерываение UDRE
}

/*
	функции следят за выходом за границу допустимых значений 
	указателя
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

//вектор прерываний

//обработчик прерывания по приходу байта в уарт
ISR (USART1_RX_vect)
{
	//считанный байт
	unsigned char byte;

	//читаем байт
	byte = UDR1;
	//эхо через узел
	UDR1 = byte;

	//если нажата клавиша enter, то ввод команды завершен
	if (byte == 0x0d)
	{
		arriveCom = 1;
	}
	else
	{
		//запоминаем принятый байт в буфере
		if ( i_buf < mesLen )
		{
			buf[i_buf++] = byte;	
		}
	}
}

//прерываение по опустошению буффера USART
ISR (USART1_UDRE_vect)
{
	i_mes++;			
 
 	//вывели весь буфер?
	if( message[i_mes] == '\0' )
	{ 
		// запрещаем прерывание по опустошению - передача закончена
		UCSR1B &=~(1<<UDRIE1);
	}
	else 
	{
		// берем данные из буффера.
		UDR1 = message[i_mes];
	}
}


//прерывание по переполнению таймера
ISR(TIMER0_OVF_vect)
{
	tcnt--;
	if (tcnt == 0)
	{
		//поднимаем флаг истечения 2-х секундного интервала
		timeFlag = 1;
		//вновь увеличиваем счетчик для замера следующих 2-х секунд
		tcnt = 10;
	}
}


int main(void) 
{

	//служебные переменные
	//счетчики для циклов 
	int i, j, k = 0, n = 0;
	
	//значение байта из памяти
	unsigned char value;


	//настраиваем оборудование с которым будем работать 
	//контакты 1,2,3 порта Е для вывода
	DDRE = (1 << PE1) | (1 << PE2) | (1 << PE3);
	//контакты 4,6 порта B для ввода
	//(кнопки)
	DDRB = (0<<PB5) | (0<<PB6);
	//подтягивающие резисторы для порта B
	PORTB = (0<<PB5) | (0<<PB6);

	//конфигурируем таймер
	TCCR0 = 0b00000111;
	TIMSK = (1 << TOIE0);

	//настраиваем уарт на прием/передачу данных
	//устанавливаем скорость передачи
	UBRR1L = LO(bauddivider);
	UBRR1H = HI(bauddivider);
	//регистр статуса уарт
	UCSR1A = 0;
	//разрешаем работу передатчика и приемника
	//прерывания по завершению приема
	//и запрещаем прерывание по завершению передачи
	UCSR1B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE1) | (0 << TXCIE1);
	//асинхронный режим последовательной связи
	//формат данных 8 бит 1 стоп-бит
	UCSR1C = (0<<UMSEL1)|(1<<USBS1)|(1<<UCSZ10)|(1<<UCSZ11);

	//разрешаем прерывания глобально
	sei();

	//настройка режима сна
	set_sleep_mode ( SLEEP_MODE_IDLE );


	//создадим в памяти массив чисел для проверки правильности работы программы
	unsigned char data[256];
	//заполним его числами от 0 до 255
	for (i = 0; i < 256; i++)
	{
		data[i] = i;
	}
	//первоначально ссылаемся на начало этого массива
	RAMPointer = data;


	//главный цикл программы
	for (;;)
	{
		//получаем значение из памяти
		value = *RAMPointer;

		if ( keyFlag )
		{
			//сканировать клавиатуру
			scanKey();
		}
		//если слово еще не отсылалось через уарт
		if ( notSend )
		{
			//отправляем на ПК
			sendByte(value);
			notSend = 0;
		}
		
		if ( notSend == 0 )
		{
			//если прошло 6 секунд
			if ( n == 2 )
			{
				nextAdr();
				n = 0;
				notSend = 1;
			}
			//считаем сколько прождали
			if ( timeFlag )
			{
				if ( lightFlag )
				{
				//отображать слова на индикаторах
					//какую тройку выводить на индикаторы
					switch (k)
					{
						case 0:
							//выводим первые 3 бита
							PORTE = (value & 0b10000000) << PE1 | (value & 0b01000000) << PE2 
										| (value & 0b00100000) << PE3;
							k = 1;
							break;
						case 1:
							//выводим вторые 3 бита
							PORTE = (value & 0b00010000) << PE1 | (value & 0b00001000) << PE2 
										| (value & 0b00000100) << PE3;
							k = 2;
							break;
						case 2:
							//выводим последние 2 бита и бит четности
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

		//если принята команда по уарт
		if ( arriveCom )
		{
			//сбрасываем флаг
			arriveCom = 0;
			//обрабатываем команду
			exeCom();
		}
		//после того как все действия выполнены засыпаем
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
		//проверяем корректность адреса
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

	//вычисляем контрольную сумму
	//отправляем на уарт вычисленную сумму
	
	return;
}


void scanKey()
{
	//считываем значение из порта
	int byte = PINB;

	if (byte & 0b00100000)
	{
		//если нажата кнопка на линии 5 порта В
		nextAdr();
		notSend = 1;
	}
	else if (byte & 0b01000000)
	{
		//если нажата кнопка на линии 6 порта В
		prevAdr();
		notSend = 1;
	}
}




