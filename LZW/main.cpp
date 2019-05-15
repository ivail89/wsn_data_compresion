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

typedef struct bfile
{
    FILE *file;
    uchar mask;
    int rack;
    int pacifier_counter;
}
BFILE;

#define PACIFIER_COUNT 2047

BFILE *OpenOutputBFile ( const char *name );
void  WriteBits ( BFILE *bfile, ulong code, int count );
void  CloseOutputBFile ( BFILE *bfile );

/*---------------------------------------------------------
   Функции высокого уровня
*/

void CompressFile ( FILE *input, BFILE *output );

/*---------------------------------------------------------
   Функции работы с моделью данных для алгоритма LZW
*/

uint find_dictionary_match ( int prefix_code, int character );

/*--------------------------------------------------------
 * Константы для оценки загрузки CPU
*/
float amountMcs = 0;
#define OPEN_OUTPUT_BFILE_MCS         87
#define WRITE_BITS_NS                 721
#define CLOSE_OUTPUT_BFILE_MCS        92
#define FIND_DICTIONARY_MATCH_NS      601

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
  amountMcs += OPEN_OUTPUT_BFILE_MCS; 

   BFILE *bfile;

   bfile = (BFILE *) calloc( 1, sizeof( BFILE ) );
   bfile->file = fopen( name, "wb" );
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
  amountMcs += CLOSE_OUTPUT_BFILE_MCS;

   if ( bfile->mask != 0x80 )
      putc( bfile->rack, bfile->file );
   fclose ( bfile->file );
   free ( (char *) bfile );
}

/*-----------------------------------------------------------
   Вывод нескольких битов
*/

void WriteBits( BFILE *bfile, ulong code, int count )
{
  amountMcs += 0.001 * WRITE_BITS_NS;

   ulong mask;

   mask = 1L << ( count - 1 );
   ulong mask1 = 1L << ( count - 1 );
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

   int index, count_attempt = 0;
   int offset;
   clock_t start = clock();
   /* Собственно получение значения хеш-функции */

   index = ( character << ( BITS - 8 ) ) ^ prefix_code;
   /* Разрешение возможных коллизий */
   if ( index == 0 )
      offset = 1;
   else
      offset = TABLE_SIZE - index;
   for ( ; ; )
   {
     count_attempt++;
      /* Эта ячейка словаря не использована */
      if ( dict[index].code_value == UNUSED ){
         amountMcs += 0.001*count_attempt*FIND_DICTIONARY_MATCH_NS;
         return index;
      }
      /* Найдено соответствие */
      if ( dict[index].prefix_code == prefix_code &&
           dict[index].character == (char) character ){
         amountMcs += 0.001*count_attempt*FIND_DICTIONARY_MATCH_NS;
         return index;
      }
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

  amountMcs = 0;
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
  return amountMcs;  
}

ifstream::pos_type filesize(const char* filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg(); 
}

// Измерения записываем в структуру
struct Row {
  string date, time;
  float co, c6h6, t, rh, ah;
  int pts1, nmhc,pts2, no, pts3, no2, pts4, o3;
};

int main(int argc, char** argv) {

  ifstream file ("AirQualityUCIsed.csv"); // Входные данные
  ofstream data ("data.dat", ios::binary); // Файл который будем сжимать

  if (!file){
    cout << "File with problem" << endl;
  } else {

	  string header, cell; 
	  Row r;
	  int nRow = 1; // Сколько строк будет записано в один пакет на сжатие

	  getline(file, header); // Заголовок забираем один раз до начала получения данных одной строкой, при необходимости сможем распарсить
	  
	  
    int index = 0;
    while (!file.eof()) {
      for (int i=0; i<nRow; i++){

        // Тестовые данные не требуют обработки
        getline(file, r.date, ';');
        getline(file, r.time, ';');

        // Числовые измерения нужно переводить из строки в число
        // Числа с плавающей точкой на вход должны идти только с точкой, запятую atof не переваривает
        getline(file, cell, ';'); r.co = atof(cell.c_str());
        getline(file, cell, ';'); r.pts1 = atof(cell.c_str());
        getline(file, cell, ';'); r.nmhc = atof(cell.c_str());
        getline(file, cell, ';'); r.c6h6 = atof(cell.c_str());
        getline(file, cell, ';'); r.pts2 = atof(cell.c_str());
        getline(file, cell, ';'); r.no = atof(cell.c_str());
        getline(file, cell, ';'); r.pts3 = atof(cell.c_str());
        getline(file, cell, ';'); r.no2 = atof(cell.c_str());
        getline(file, cell, ';'); r.pts4 = atof(cell.c_str());
        getline(file, cell, ';'); r.o3 = atof(cell.c_str());
        getline(file, cell, ';'); r.t = atof(cell.c_str());
        getline(file, cell, ';'); r.rh = atof(cell.c_str());
        getline(file, cell, ';'); r.ah = atof(cell.c_str());

        getline(file, cell); // The tail of the string
        data.write((char*)&r, sizeof(r)); // Т.к. файл должен быть бинарным
      }
	  
      setbuf( stdout, NULL );
      cout << index  << ";" << call_compress("data.dat", "out.dat") << ";" << filesize("data.dat") << ";" << filesize("out.dat") << endl;
      index++;
    }

	  data.close();
  }
  return 0;
}

