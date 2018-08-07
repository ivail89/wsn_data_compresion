/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Mironov
 *
 * Created on 12 июня 2018 г., 14:28
 */

#include <stdio.h>
//#include <process.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdint.h>

static inline uint64_t
rdtsc(void)
{
	uint32_t eax = 0, edx;

	__asm__ __volatile__("cpuid;"
			     "rdtsc;"
				: "+a" (eax), "=d" (edx)
				:
				: "%rcx", "%rbx", "memory");

	__asm__ __volatile__("xorl %%eax, %%eax;"
			     "cpuid;"
				:
				:
				: "%rax", "%rbx", "%rcx", "%rdx", "memory");

	return (((uint64_t)edx << 32) | eax);
}

using namespace std;

// Количество битов в регистре
#define BITS_IN_REGISTER 16

// Максимально возможное значение в регистре
#define TOP_VALUE (((long) 1 << BITS_IN_REGISTER) - 1)

// Диапазоны
#define FIRST_QTR (TOP_VALUE / 4 + 1)
#define HALF      (2 * FIRST_QTR)
#define THIRD_QTR (3 * FIRST_QTR)

// Количество символов алфавита
#define NO_OF_CHARS 256
// Специальный символ КонецФайла
#define EOF_SYMBOL    (NO_OF_CHARS + 1)
// Всего символов в модели
#define NO_OF_SYMBOLS (NO_OF_CHARS + 1)

// Порог частоты для масштабирования
#define MAX_FREQUENCY 16383

// Таблицы перекодировки
unsigned char index_to_char [NO_OF_SYMBOLS];
int	char_to_index [NO_OF_CHARS];

// Таблицы частот
int cum_freq [NO_OF_SYMBOLS + 1];
int	freq [NO_OF_SYMBOLS + 1];

// Регистры границ и кода
long low, high;
long value;

// Поддержка побитлвых операций с файлами
long bits_to_follow;
int buffer;
int bits_to_go;
int garbage_bits;

// Обрабатываемые файлы
FILE *in, *out;

//------------------------------------------------------------
// Инициализация адаптивной модели
void start_model (void)
{
  int i;

  for ( i = 0; i < NO_OF_CHARS; i++)
  {
    char_to_index [i] = i + 1;
    index_to_char [i + 1] = i;
  }
  for ( i = 0; i <= NO_OF_SYMBOLS; i++)
  {
    freq [i] = 1;
    cum_freq [i] = NO_OF_SYMBOLS - i;
  }
  freq [0] = 0;
}

//------------------------------------------------------------
// Обновление модели очередным символом
void update_model ( int symbol)
{
  int i;
  int ch_i, ch_symbol;
  int cum;

  // проверка на переполнение счетчика частоты
  if (cum_freq [0] == MAX_FREQUENCY)
  {
    cum = 0;
    // масштабирование частот при переполнении
    for ( i = NO_OF_SYMBOLS; i >= 0; i--)
    {
      freq [i] = (freq [i] + 1) / 2;
      cum_freq [i] = cum;
      cum += freq [i];
    }
  }

  for ( i = symbol; freq [i] == freq [i - 1]; i--);
  if (i < symbol)
  {
    ch_i                      = index_to_char [i];
    ch_symbol                 = index_to_char [symbol];
    index_to_char [i]         = ch_symbol;
    index_to_char [symbol]    = ch_i;
    char_to_index [ch_i]      = symbol;
    char_to_index [ch_symbol] = i;
  }

  // обновление значений в таблицах частот
  freq [i] += 1;
  while (i > 0)
  {
    i -= 1;
    cum_freq [i] += 1;
  }
}

//------------------------------------------------------------
// Инициализация побитового ввода
void start_inputing_bits (void)
{
  bits_to_go = 0;
  garbage_bits = 0;
}

//------------------------------------------------------------
// Ввод очередного бита сжатой информации
int input_bit (void)
{
  int t;

  if (bits_to_go == 0)
  {
    buffer = getc (in);
    if (buffer == EOF)
    {
      garbage_bits += 1;
      if (garbage_bits > BITS_IN_REGISTER - 2)
      {
	printf ("Ошибка в сжатом файле\n");
	exit (-1);
      }
    }
    bits_to_go = 8;
  }
  t = buffer & 1;
  buffer >>= 1;
  bits_to_go -= 1;
  return t;
}

//------------------------------------------------------------
// Инициализация побитового вывода
void start_outputing_bits (void)
{
  buffer = 0;
  bits_to_go = 8;
}

//------------------------------------------------------------
// Вывод очередного бита сжатой информации
void output_bit ( int bit)
{
  buffer >>= 1;
  if (bit)
    buffer |= 0x80;
  bits_to_go -= 1;
  if (bits_to_go == 0)
  {
    putc ( buffer, out);
    bits_to_go = 8;
  }
}

//------------------------------------------------------------
// Очистка буфера побитового вывода
void done_outputing_bits (void)
{
  putc ( buffer >> bits_to_go, out);
}

//------------------------------------------------------------
// Вывод указанного бита и отложенных ранее
void output_bit_plus_follow ( int bit)
{
  output_bit (bit);
  while (bits_to_follow > 0)
  {
    output_bit (!bit);
    bits_to_follow--;
  }
}

//------------------------------------------------------------
// Инициализация регистров границ и кода перед началом сжатия
void start_encoding (void)
{
  low            = 0l;
  high           = TOP_VALUE;
  bits_to_follow = 0l;
}

//------------------------------------------------------------
// Очистка побитового вывода
void done_encoding (void)
{
  bits_to_follow++;
  if (low < FIRST_QTR)
    output_bit_plus_follow (0);
  else
    output_bit_plus_follow (1);
}

//------------------------------------------------------------
/* Инициализация регистров перед декодированием.
   Загрузка начала сжатого сообщения
*/
void start_decoding (void)
{
  int i;

  value = 0l;
  for ( i = 1; i <= BITS_IN_REGISTER; i++)
    value = 2 * value + input_bit ();
  low = 0l;
  high = TOP_VALUE;
}

//------------------------------------------------------------
// Кодирование очередного символа
void encode_symbol ( int symbol)
{
  long range;

  // пересчет значений границ
  range = (long) (high - low) + 1;
  high = low + (range * cum_freq [symbol - 1]) / cum_freq [0] - 1;
  low = low + (range * cum_freq [symbol]) / cum_freq [0];
  // выдвигание очередных битов
  for (;;)
  {
    if (high < HALF)
      output_bit_plus_follow (0);
    else if (low >= HALF)
    {
      output_bit_plus_follow (1);
      low -= HALF;
      high -= HALF;
    }
    else if (low >= FIRST_QTR && high < THIRD_QTR)
    {
      bits_to_follow += 1;
      low -= FIRST_QTR;
      high -= FIRST_QTR;
    }
    else
      break;
    // сдвиг влево с "втягиванием" очередного бита
    low = 2 * low;
    high = 2 * high + 1;
  }
}

//------------------------------------------------------------
// Декодирование очередного символа
int decode_symbol (void)
{
  long range;
  int cum, symbol;

  // определение текущего масштаба частот
  range = (long) (high - low) + 1;
  // масштабирование значения в регистре кода
  cum = (int)
    ((((long) (value - low) + 1) * cum_freq [0] - 1) / range);
  // поиск соответствующего символа в таблице частот
  for (symbol = 1; cum_freq [symbol] > cum; symbol++);
  // пересчет границ
  high = low + (range * cum_freq [symbol - 1]) / cum_freq [0] - 1;
  low = low + (range * cum_freq [symbol]) / cum_freq [0];
  // удаление очередного символа из входного потока
  for (;;)
  {
    if (high < HALF)
    {
    }
    else if (low >= HALF)
    {
      value -= HALF;
      low -= HALF;
      high -= HALF;
    }
    else if (low >= FIRST_QTR && high < THIRD_QTR)
    {
      value -= FIRST_QTR;
      low -= FIRST_QTR;
      high -= FIRST_QTR;
    }
    else
      break;
    // сдвиг влево с "втягиванием очередного бита
    low = 2 * low;
    high = 2 * high + 1;
    value = 2 * value + input_bit ();
  }
  return symbol;
}

//------------------------------------------------------------
// Собственно адаптивное арифметическое кодирование
void encode ( char *infile, char *outfile)
{
  int ch, symbol;

  in = fopen ( infile, "r+b");
  out = fopen ( outfile, "w+b");
  if (in == NULL || out == NULL)
    return;
  start_model ();
  start_outputing_bits ();
  start_encoding ();
  for (;;)
  {
    ch = getc (in);
    if (ch == EOF)
      break;
    symbol = char_to_index [ch];
    encode_symbol (symbol);
    update_model (symbol);
  }
  encode_symbol (EOF_SYMBOL);
  done_encoding ();
  done_outputing_bits ();
  fclose (in);
  fclose (out);
}

//------------------------------------------------------------
// Собственно адаптивное арифметическое декодирование
void decode ( char *infile, char *outfile)
{
  int ch, symbol;

  in = fopen ( infile, "r+b");
  out = fopen ( outfile, "w+b");
  if (in == NULL || out == NULL)
    return;
  start_model ();
  start_inputing_bits ();
  start_decoding ();
  for (;;)
  {
    symbol = decode_symbol ();
    if (symbol == EOF_SYMBOL)
      break;
    ch = index_to_char [symbol];
    putc ( ch, out);
    update_model (symbol);
  }
  fclose (in);
  fclose (out);
}

struct Message {
    short int ultrasonic_distance_sensor[5]; //пять ультразвуковых датчиков расстояния
    short int infrared_distance_sensor[4]; //четыре инфракрасных датчика расстояния
    float accelerometer[3]; //три оси акселерометра
    short int gyroscope[4][3]; //четыре гироскопа
};

void createMessage(unsigned int count_measurement) {
    Message m1;
    const char *FName="data.dat"; 
    ofstream fout(FName, ios::binary | ios::out);
    if (count_measurement == 0) count_measurement = 1;

    for (int i=0; i<count_measurement; i++){
        // Значения измерений от 15 до 645 см
        m1.ultrasonic_distance_sensor[0] = 15 + rand()%630; 
        m1.ultrasonic_distance_sensor[1] = 15 + rand()%630; 
        m1.ultrasonic_distance_sensor[2] = 15 + rand()%630; 
        m1.ultrasonic_distance_sensor[3] = 15 + rand()%630; 
        m1.ultrasonic_distance_sensor[4] = 15 + rand()%630; 
        
        // значения от 10 до 80 см
        m1.infrared_distance_sensor[0] = 10 + rand()%70; 
        m1.infrared_distance_sensor[1] = 10 + rand()%70; 
        m1.infrared_distance_sensor[2] = 10 + rand()%70; 
        m1.infrared_distance_sensor[3] = 10 + rand()%70; 
        
        // значения от -11g до 11g
        m1.accelerometer[0] = -11 + rand()%22; 
        m1.accelerometer[1] = -11 + rand()%22; 
        m1.accelerometer[2] = -11 + rand()%22; 
        
        // значения от -2000 до 2000 °/сек
        m1.gyroscope[0][0] = -2000 + rand()%4000; 
        m1.gyroscope[0][1] = -2000 + rand()%4000; 
        m1.gyroscope[0][2] = -2000 + rand()%4000; 
        m1.gyroscope[1][0] = -2000 + rand()%4000; 
        m1.gyroscope[1][1] = -2000 + rand()%4000; 
        m1.gyroscope[1][2] = -2000 + rand()%4000; 
        m1.gyroscope[2][0] = -2000 + rand()%4000; 
        m1.gyroscope[2][1] = -2000 + rand()%4000; 
        m1.gyroscope[2][2] = -2000 + rand()%4000; 
        m1.gyroscope[3][0] = -2000 + rand()%4000; 
        m1.gyroscope[3][1] = -2000 + rand()%4000; 
        m1.gyroscope[3][2] = -2000 + rand()%4000; 
        
        fout.write((char*)&m1, sizeof(m1));
    }
    fout.close(); // закрываем файл    
};

ifstream::pos_type filesize(const char* filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg(); 
}

int main(int argc, char** argv) {

    ofstream fRes("data.xls", ios::out);
    unsigned int count_measurement = 0;
    int a[100];
    
    do{
        for (int i=0; i<100; i++){
            createMessage(count_measurement);
            setbuf( stdout, NULL );
            // Компрессия:

				// оценка загрузки процессора в тактах
				unsigned long start_ticks = rdtsc();
                encode ("data.dat", "out.dat");
				unsigned long diff_ticks = rdtsc() - start_ticks;

            //a[i] = filesize("out.dat");// размер файла после сжатия
            a[i] = diff_ticks; // количество тактов
            
            cout << count_measurement << " - " << i << endl;
            // Декомпрессия:
            //call_expand("out1.txt", "out_data.txt");
        }
        float medium =0;
        for (int i=0; i<100; i++){
            medium += a[i];
        }
        medium = medium / 100;
        //средний размер файла после сжатия (после 100 итереаций)
        //fRes << count_measurement << "\t" << filesize("data.dat") << "\t"
          //      << medium << endl;
        
        //среднее количество тактов
        fRes << count_measurement << "\t" << fixed << medium << endl;
        count_measurement += 30;
    } while (count_measurement < 600);
    
    fRes.close();
    return 0;
}
