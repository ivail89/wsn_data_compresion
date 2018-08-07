/*---------------------------------------------------------
* Алгоритм LZW. Демонстрационная программа.
*---------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ctime>
#include <stdint.h>

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int  uint;

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
   BFILE *bfile;

   bfile = (BFILE *) calloc( 1, sizeof( BFILE ) );
   bfile->file = fopen( name, "wb" );
   bfile->rack = 0;
   bfile->mask = 0x80;
   bfile->pacifier_counter = 0;
   return bfile;
}

/*-----------------------------------------------------------
   Открытие файла для побитового чтения
*/

BFILE *OpenInputBFile( const char *name )
{
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
    fclose ( bfile->file );
    free ( (char *) bfile );
}

/*-----------------------------------------------------------
   Вывод одного бита
*/

void WriteBit ( BFILE *bfile, int bit )
{
   if ( bit )
      bfile->rack |= bfile->mask;
   bfile->mask >>= 1;
   if ( bfile->mask == 0 )
   {
      putc( bfile->rack, bfile->file );
      if ( ( bfile->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
         putc( '.', stdout );
      bfile->rack = 0;
      bfile->mask = 0x80;
   }
}

/*-----------------------------------------------------------
   Вывод нескольких битов
*/

void WriteBits( BFILE *bfile, ulong code, int count )
{
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
  if ( ( bfile->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
            putc( '.', stdout );
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
   int value;

   if ( bfile->mask == 0x80 )
   {
      bfile->rack = getc( bfile->file );
      if ( bfile->rack == EOF )
         fatal_error( "Error in function ReadBit!\n" );
      if ( ( bfile->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
         putc( '.', stdout );
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
  if ( ( bfile->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
            putc( '.', stdout );
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

/* Стек для декодирования */

char decode_stack[TABLE_SIZE];

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
   Процедура декодирования сжатого файла
*/

void ExpandFile ( BFILE *input, FILE *output )
{
   uint next_code, new_code, old_code;
   int character;
   uint count;

   next_code = FIRST_CODE;
   old_code = (uint) ReadBits( input, BITS );
   if ( old_code == END_OF_STREAM )
      return;
   character = old_code;

   putc ( old_code, output );

   while ( ( new_code = (uint) ReadBits( input, BITS ) )
             != END_OF_STREAM )
   {
      /* Обработка возможной исключительной ситуации */
      if ( new_code >= next_code )
      {
         decode_stack[ 0 ] = (char) character;
         count = decode_string( 1, old_code );
      }
      else
         count = decode_string( 0, new_code );

      character = decode_stack[ count - 1 ];
      /* Выдача раскодированной строки */
      while ( count > 0 )
         putc( decode_stack[--count], output );
      /* Обновление словаря */
      if ( next_code <= MAX_CODE )
      {
         dict[next_code].prefix_code = old_code;
         dict[next_code].character = (char) character;
         next_code++;
      }
      old_code = new_code;
   }
}

/*-----------------------------------------------------------
   Процедура поиска в словаре указанной пары <код фразы,
   символ>. Для ускорения поиска используется хеш, получаемый
   из параметров.
*/

uint find_dictionary_match ( int prefix_code, int character )
{
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

/*-----------------------------------------------------------
   Процедура декодирования строки. Размещает символы в стеке,
   возвращает их количество.
*/

uint decode_string ( uint count, uint code )
{
   while ( code > 255 ) /* Пока не встретится код символа */
   {
      decode_stack[count++] = dict[code].character;
      code = dict[code].prefix_code;
   }
   decode_stack[count++] = (char) code;
   return count;
}


void call_compress( const char* infile_name, const char* outfile_name ) {

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
  printf( "\nКомпрессия %s в %s\n", infile_name, outfile_name );
  
  // вызов процедуры компрессии
  CompressFile( input, output );

  // закрытие файлов
  CloseOutputBFile( output );
  fclose( input );

  printf( "\nCompression complete." );
}

void call_expand( const char* infile_name, const char* outfile_name ) {
  BFILE *input;
  FILE *output;

  // открытие входного файла для чтения
  input = OpenInputBFile( infile_name );
  if ( input == NULL )
   fatal_error( "Error on open %s for read\n", infile_name );
  
  // открытие выходного файла для записи
  output = fopen( outfile_name, "wb" );
  if ( output == NULL )
   fatal_error( "Error on open %s for write\n", outfile_name );

  printf( "\nDecompression %s into %s\n", infile_name, outfile_name );
  
  // вызов процедуры декомпрессии
  ExpandFile(input, output );

  // закрытие файлов
  CloseInputBFile( input );
  fclose( output );

  printf( "\nDecompression complete." );
}


//------------------------------------------------------------
// Главная процедура
int main(int argc, char* argv[])
{
	// 	в микросекундах
	clock_t start_mcs = clock();
	// такты
	unsigned long start_ticks = rdtsc();
	
	setbuf(stdout, NULL);

	// Компрессия:
	call_compress("data.dat", "out.dat");
	
	// Декомпрессия:
	//call_expand("out1.txt", "out_data.txt");
	
	// такты
	unsigned long diff_ticks = rdtsc() - start_ticks;
	
	// микросекунды
	clock_t diff_mcs = clock() - start_mcs;
	
	printf("\n****************************");
	printf("\nTime Difference: %lu ticks.", diff_ticks);
	printf("\nAnd %f seconds.\n", ((float)diff_mcs) / CLOCKS_PER_SEC);

 return 0;

}
