/* 
 * Данная программа создана для оценки нагрузки на процессор в зависимости от
 * входящих данных при работе алгоритма S-LZW
*/

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int  uint;

using namespace std;

/*---------------------------------------------------------
   Побитовый доступ к файлам
*/

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

typedef struct bfile
{
    FILE *file;
    uchar mask;
    int rack;
    int pacifier_counter;
}
BFILE;

#define PACIFIER_COUNT 2047

BFILE *OpenInputBFile ( const char *name );
BFILE *OpenOutputBFile ( const char *name );
void  WriteBit  ( BFILE *bfile, int bit );
void  WriteBits ( BFILE *bfile, ulong code, int count );
int   ReadBit  ( BFILE *bfile );
ulong ReadBits ( BFILE *bfile, int bit_count );
void  CloseInputBFile ( BFILE *bfile );
void  CloseOutputBFile ( BFILE *bfile );

/*---------------------------------------------------------
   Функции высокого уровня
*/

void CompressFile ( FILE *input, BFILE *output );
void ExpandFile   ( BFILE *input, FILE *output );

/*---------------------------------------------------------
   Функции работы с моделью данных для алгоритма LZW
*/

uint find_dictionary_match ( int prefix_code, int character );
uint decode_string ( uint offset, uint code );

/*--------------------------------------------------------
 * Константы для оценки загрузки CPU
*/
ulong amountTacts = 0;
#define OPEN_INPUT_BFILE_MCS          87
#define OPEN_OUTPUT_BFILE_MCS         1
#define WRITE_BIT_MCS                 1
#define WRITE_BITS_MCS                1
#define READ_BIT_MCS                  1
#define READ_BITS_MCS                 1
#define CLOSE_INPUT_BFILE_MCS         1
#define CLOSE_OUTPUT_BFILE_MCS        1
#define FIND_DICTIONARY_MATCH_MCS     1

/*---------------------------------------------------------
   Константы, используемые при работе LZW
*/
/* Количество битов в коде */
#define BITS                       12
/* Максимальное значение кода */
#define MAX_CODE                   ( ( 1 << BITS ) - 1 )
/* Размер словаря в элементах */
#define TABLE_SIZE                 5021
/* Специальный код конца потока */
#define END_OF_STREAM              256
/* Значение кода, которое получает первая добавленная
в словарь фраза */
#define FIRST_CODE                 257
/* Признак свободной ячейки в словаре */
#define UNUSED                     -1

/*-----------------------------------------------------------
   Обработка фатальной ошибки при работе программы.
*/

void fatal_error( const char *str, ... )
{
   printf( "Fatal error: %s\n", str );
   exit(1);
}

/*-----------------------------------------------------------
   Открытие файла для побитовой записи
*/

BFILE *OpenOutputBFile ( const char * name )
{
  amountTacts += OPEN_OUTPUT_BFILE_TACTS; 

//  ulong start_ticks = rdtsc();
  clock_t start_mcs = clock();
   BFILE *bfile;

   bfile = (BFILE *) calloc( 1, sizeof( BFILE ) );
   bfile->file = fopen( name, "wb" );
   bfile->rack = 0;
   bfile->mask = 0x80;
   bfile->pacifier_counter = 0;
  clock_t diff_mcs = clock() - start_mcs;
  cout << diff_mcs << endl;
   return bfile;
}

/*-----------------------------------------------------------
   Открытие файла для побитового чтения
*/

BFILE *OpenInputBFile( const char *name )
{
  amountTacts += OPEN_INPUT_BFILE_TACTS;
   BFILE *bfile;

   bfile = (BFILE *) calloc( 1, sizeof( BFILE ) );
   bfile->file = fopen( name, "rb" );
   bfile->rack = 0;
   bfile->mask = 0x80;
   bfile->pacifier_counter = 0;
   return bfile;
}

/*-----------------------------------------------------------
   Закрытие файла для побитовой записи
*/

void CloseOutputBFile ( BFILE *bfile )
{
  amountTacts += CLOSE_OUTPUT_BFILE_TACTS;

   if ( bfile->mask != 0x80 )
      putc( bfile->rack, bfile->file );
   fclose ( bfile->file );
   free ( (char *) bfile );
}

/*-----------------------------------------------------------
   Закрытие файла для побитового чтения
*/

void CloseInputBFile ( BFILE *bfile )
{
  amountTacts += CLOSE_INPUT_BFILE_TACTS;
    fclose ( bfile->file );
    free ( (char *) bfile );
}

/*-----------------------------------------------------------
   Вывод одного бита
*/

void WriteBit ( BFILE *bfile, int bit )
{
  amountTacts += WRITE_BIT_TACTS;
   if ( bit )
      bfile->rack |= bfile->mask;
   bfile->mask >>= 1;
   if ( bfile->mask == 0 )
   {
      putc( bfile->rack, bfile->file );
      bfile->rack = 0;
      bfile->mask = 0x80;
   }
}

/*-----------------------------------------------------------
   Вывод нескольких битов
*/

void WriteBits( BFILE *bfile, ulong code, int count )
{
  amountTacts += WRITE_BITS_TACTS;
   ulong mask;

   mask = 1L << ( count - 1 );
   while ( mask != 0)
   {
      if ( mask & code )
  bfile->rack |= bfile->mask;
      bfile->mask >>= 1;
      if ( bfile->mask == 0 )
      {
  putc( bfile->rack, bfile->file );
  bfile->rack = 0;
  bfile->mask = 0x80;
      }
      mask >>= 1;
   }
}

/*-----------------------------------------------------------
   Ввод одного бита
*/

int ReadBit( BFILE *bfile )
{
  amountTacts += READ_BIT_TACTS;
   int value;

   if ( bfile->mask == 0x80 )
   {
      bfile->rack = getc( bfile->file );
      if ( bfile->rack == EOF )
         fatal_error( "Error in function ReadBit!\n" );
   }

   value = bfile->rack & bfile->mask;
   bfile->mask >>= 1;
   if ( bfile->mask == 0 )
      bfile->mask = 0x80;
   return ( value ? 1 : 0 );
}

/*-----------------------------------------------------------
   Ввод нескольких битов
*/

ulong ReadBits ( BFILE *bfile, int bit_count )
{
  amountTacts += READ_BITS_TACTS;
   ulong mask;
   ulong return_value;

   mask = 1L << ( bit_count - 1 );
   return_value = 0;
   while ( mask != 0 )
   {
      if ( bfile->mask == 0x80 )
      {
  bfile->rack = getc( bfile->file );
  if ( bfile->rack == EOF )
            fatal_error( "Error in function ReadBits!\n" );
      }
      if ( bfile->rack & bfile->mask )
         return_value |= mask;
      mask >>= 1;
      bfile->mask >>= 1;
      if ( bfile->mask == 0 )
  bfile->mask = 0x80;
   }

   return return_value;
}

/*-----------------------------------------------------------
   Далее начинается исходный текст собственно алгоритма LZW
*/

/* Структура словаря для алгоритма LZW */

struct dictionary
{
   int code_value;
   int prefix_code;
   char character;
}
dict[TABLE_SIZE];

/*-----------------------------------------------------------
   Процедура сжатия файла
*/

void CompressFile ( FILE *input, BFILE *output )
{
   int next_code, character, string_code;
   uint index, i;

   /* Инициализация */
   next_code = FIRST_CODE;
   for ( i = 0 ; i < TABLE_SIZE ; i++ )
       dict[i].code_value = UNUSED;
   /* Считать первый символ */
   if ( ( string_code = getc( input ) ) == EOF )
       string_code = END_OF_STREAM;
   /* Пока не конец сообщения */
   while ( ( character = getc( input ) ) != EOF )
   {
      /* Попытка найти в словаре пару <фраза, символ> */
      index = find_dictionary_match ( string_code, character );
      /* Соответствие найдено */
      if ( dict[index].code_value != -1 )
         string_code = dict[index].code_value;
      /* Такой пары в словаре нет */
      else
      {
         /* Добавление в словарь */
  if ( next_code <= MAX_CODE )
         {
            dict[index].code_value = next_code++;
            dict[index].prefix_code = string_code;
            dict[index].character = (char) character;
         }
         /* Выдача кода */
         WriteBits( output, (ulong) string_code, BITS );
         string_code = character;
      }
   }
   /* Завершение кодирования */
   WriteBits( output, (ulong) string_code, BITS );
   WriteBits( output, (ulong) END_OF_STREAM, BITS );
}

/*-----------------------------------------------------------
   Процедура поиска в словаре указанной пары <код фразы,
   символ>. Для ускорения поиска используется хеш, получаемый
   из параметров.
*/

uint find_dictionary_match ( int prefix_code, int character )
{
  amountTacts += FIND_DICTIONARY_MATCH_TACTS;
   int index;
   int offset;

   /* Собственно получение значения хеш-функции */
   index = ( character << ( BITS - 8 ) ) ^ prefix_code;
   /* Разрешение возможных коллизий */
   if ( index == 0 )
      offset = 1;
   else
      offset = TABLE_SIZE - index;
   for ( ; ; )
   {
      /* Эта ячейка словаря не использована */
      if ( dict[index].code_value == UNUSED )
         return index;
      /* Найдено соответствие */
      if ( dict[index].prefix_code == prefix_code &&
           dict[index].character == (char) character )
         return index;
      /* Коллизия. Подготовка к следующей попытке ее
         разрешения */
      index -= offset;
      if ( index < 0 )
         index += TABLE_SIZE;
   }
}

ulong call_compress( const char* infile_name, const char* outfile_name ) {

  BFILE *output;
  FILE *input;

  // открытие входного файла для чтения
  input = fopen( infile_name, "rb" );
  if ( input == NULL )
   fatal_error( "Ошибка при открытии %s для ввода\n", infile_name );

  // открытие выходного файла для записи
  output = OpenOutputBFile( outfile_name );
  if ( output == NULL )
   fatal_error( "Ошибка при открытии %s для вывода\n", outfile_name );
  
  // вызов процедуры компрессии
  CompressFile( input, output );
  // закрытие файлов
  CloseOutputBFile( output );
  fclose( input );

  // Вывод итогового значения тактов
  return amountTacts;  
}

struct Message {
    short int ultrasonic_distance_sensor[5]; //пять ультразвуковых датчиков расстояния
    short int infrared_distance_sensor[4]; //четыре инфракрасных датчика расстояния
    float accelerometer[3]; //три оси акселерометра
    short int gyroscope[4][3]; //четыре гироскопа
};

void createMessage(int count_measurement) {
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
    unsigned int count_measurement = 30;
    unsigned long a[100];
    
    //do{
        for (int i=0; i<10000; i++){
         // createMessage(count_measurement);
             
          // Компрессия:
				  setbuf( stdout, NULL );
          a[0] = call_compress("data.dat", "out.dat");
         // cout << count_measurement << " - " << i << endl;
        }
       /* float medium =0;
        for (int i=0; i<100; i++){
            medium += a[i];
        }
        medium = medium / 100;*/
        
        //средний размер файла после сжатия (после 100 итереаций)
        //fRes << count_measurement << "\t" << filesize("data.dat") << "\t"
        //      << medium << endl;
        
       // count_measurement += 30;
   // } while (count_measurement < 600);
    
    fRes.close();
    return 0;
}

