// HuffAdapt.cpp :

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
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

using namespace std;

//   Константы, используемые в алгоритме кодирования
#define END_OF_STREAM 256 /* Маркер конца потока */
#define ESCAPE 257 /* Маркер начала ESCAPE последовательности */
#define SYMBOL_COUNT 258 /* Максимально возможное количество листьев дерева (256+2 маркера)*/

#define NODE_TABLE_COUNT ((SYMBOL_COUNT * 2) - 1)
#define ROOT_NODE 0
#define MAX_WEIGHT 0x8000 /* Вес корня, при котором начинается
 масштабирование веса */
#define TRUE    1
#define FALSE    0

#define PACIFIER_COUNT 2047 // шаг индикатора выполнения

/* Коды ошибок */
#define NO_ERROR      0
#define BAD_FILE_NAME 1
#define BAD_ARGUMENT  2 

// Эта структура данных нужна для побитового доступа к файлам
typedef struct bit_file
{
	FILE *file;
	unsigned char mask;
	int rack;
	int pacifier_counter;
}
COMPRESSED_FILE;

// структура, описывающая узел дерева
struct node
{
	unsigned int weight; /* Вес узла */
	int parent; /* Номер родителя в массиве узлов */
	int child_is_leaf; /* Флаг листа (TRUE, если лист) */
	int child;
};
/*
   Эта структура данных используется для работы
   с деревом кодирования Хаффмана
   процедурами кодирования и декодирования
 */
typedef struct tree
{
	int leaf[ SYMBOL_COUNT ]; /* Массив листьев дерева */
	int next_free_node; /* Номер следующего
		свободного элемента массива листьев */
	node nodes[ NODE_TABLE_COUNT ]; /* Массив узлов */
}
TREE;

//  Сервисные функции
void usage_exit (void);
void print_ratios (char *input, char *output);
long file_size (char *name);
void fatal_error (char *fmt);

// Функции побитового доступа к файлам
COMPRESSED_FILE	*OpenInputCompressedFile(char *name);
COMPRESSED_FILE	*OpenOutputCompressedFile(char *name);
void OutputBit (COMPRESSED_FILE *, int bit);
void OutputBits (COMPRESSED_FILE *bit_file,
				 unsigned long code, int count);
int InputBit (COMPRESSED_FILE *bit_file);
unsigned long InputBits (COMPRESSED_FILE *bit_file,
						 int bit_count);
void CloseInputCompressedFile (COMPRESSED_FILE *bit_file);
void CloseOutputCompressedFile (COMPRESSED_FILE *bit_file);

// Собственно адаптивное кодирование Хаффмена
void CompressFile (FILE *input, COMPRESSED_FILE *output);
void ExpandFile (COMPRESSED_FILE *input, FILE *output);
void InitializeTree	(TREE *tree);
void EncodeSymbol (TREE *tree, unsigned int c,
				   COMPRESSED_FILE *output);
int  DecodeSymbol (TREE *tree, COMPRESSED_FILE *input);
void UpdateModel (TREE *tree, int c);
void RebuildTree (TREE *tree);
void swap_nodes (TREE *tree, int i, int j);
void add_new_node (TREE *tree, int c);
//=========================================================
//---------------------------------------------------------
// Вывод помощи о пользовании программой 
void help ()
{
  printf("HuffAdapt e(encoding)|d(decoding) input output\n");
}
//---------------------------------------------------------
// Эта функция возвращает размер указанного ей файла
#ifndef SEEK_END
#define SEEK_END 2
#endif
long file_size (char *name)
{
	long eof_ftell;
	FILE *file;

	file = fopen(name, "r");
	if (file == NULL)
		return(0L);
	fseek(file, 0L, SEEK_END);
	eof_ftell = ftell(file);
	fclose(file);
	return(eof_ftell);
}
//---------------------------------------------------------
/*  Эта фунцция выводит результаты сжатия
    после окончания сжатия
 */
void print_ratios (char *input, char *output)
{
	long input_size;
	long output_size;
	int ratio;

	input_size = file_size(input);
	if (input_size == 0)
		input_size = 1;
	output_size = file_size(output);
	ratio = 100 - (int) (output_size * 100L / input_size);
	printf("\nSource filesize:\t%ld\n", input_size);
	printf("Target Filesize:\t%ld\n", output_size);
	if (output_size == 0)
		output_size = 1;
	printf("Compression ratio:\t\t%d%%\n", ratio);
}
//---------------------------------------------------------
// вывод сообщения об ошибке
void fatal_error (char *fmt)
{
	printf("Fatal error: ");
	printf("%s",fmt);
	exit(-1);
};
//---------------------------------------------------------
// открытие файла для побитового вывода
COMPRESSED_FILE *OpenOutputCompressedFile(char *name)
{
	COMPRESSED_FILE *compressed_file;

	compressed_file = (COMPRESSED_FILE *)
		calloc(1, sizeof(COMPRESSED_FILE));
	if (compressed_file == NULL)
		return(compressed_file);
	compressed_file->file = fopen(name, "wb");
	compressed_file->rack = 0;
	compressed_file->mask = 0x80;
	compressed_file->pacifier_counter = 0;
	return(compressed_file);
}
//---------------------------------------------------------
//Открытие файла для побитового ввода
COMPRESSED_FILE *OpenInputCompressedFile (char *name)
{
	COMPRESSED_FILE *compressed_file;

	compressed_file = (COMPRESSED_FILE *)
		calloc(1, sizeof(COMPRESSED_FILE));
	if (compressed_file == NULL)
		return(compressed_file);
	compressed_file->file = fopen(name, "rb");
	compressed_file->rack = 0;
	compressed_file->mask = 0x80;
	compressed_file->pacifier_counter = 0;
	return(compressed_file);
}
//---------------------------------------------------------
//Закрытие файла для побитового вывода
void CloseOutputCompressedFile(COMPRESSED_FILE *compressed_file)
{
	if (compressed_file->mask != 0x80)
		if (putc(compressed_file->rack, compressed_file->file) !=
			compressed_file->rack)
			fatal_error("Error on close compressed file.\n");
	fclose(compressed_file->file);
	free((char *) compressed_file);
}
//---------------------------------------------------------
// Закрытие файла для побитового ввода
void CloseInputCompressedFile(COMPRESSED_FILE *compressed_file)
{
	fclose(compressed_file->file);
	free((char *) compressed_file);
}
//---------------------------------------------------------
// Вывод одного бита
void OutputBit(COMPRESSED_FILE *compressed_file, int bit)
{
	if (bit)
		compressed_file->rack |= compressed_file->mask;
	compressed_file->mask >>= 1;
	if (compressed_file->mask == 0)
	{
		if (putc(compressed_file->rack, compressed_file->file) !=
			compressed_file->rack)
			fatal_error("Error on OutputBit!\n");
		else if ((compressed_file->pacifier_counter++ &
			PACIFIER_COUNT) == 0)
			putc('.', stdout);
		compressed_file->rack = 0;
		compressed_file->mask = 0x80;
    }
}
//---------------------------------------------------------
// Вывод нескольких битов
void OutputBits(COMPRESSED_FILE *compressed_file,
				unsigned long code, int count)
{
	unsigned long mask;

	mask = 1L << (count - 1);
	while (mask != 0)
	{
		if (mask & code)
			compressed_file->rack |= compressed_file->mask;
		compressed_file->mask >>= 1;
		if (compressed_file->mask == 0)
		{
			if (putc(compressed_file->rack, compressed_file->file)!=
				compressed_file->rack)
				fatal_error("Error on OutputBits!\n");
			else if ((compressed_file->pacifier_counter++ &
				PACIFIER_COUNT) == 0)
				putc('.', stdout);
			compressed_file->rack = 0;
			compressed_file->mask = 0x80;
		}
		mask >>= 1;
	}
}
//---------------------------------------------------------
// Ввод одного бита
int InputBit (COMPRESSED_FILE *compressed_file)
{
	int value;

	if (compressed_file->mask == 0x80)
	{
		compressed_file->rack = getc(compressed_file->file);
		if (compressed_file->rack == EOF)
			fatal_error("Error on InputBit!\n");
		if ((compressed_file->pacifier_counter++ &
			PACIFIER_COUNT) == 0)
			putc('.', stdout);
	}
	value = compressed_file->rack & compressed_file->mask;
	compressed_file->mask >>= 1;
	if (compressed_file->mask == 0)
		compressed_file->mask = 0x80;
	return(value ? 1 : 0);
}
//---------------------------------------------------------
// Ввод нескольких битов
unsigned long InputBits (COMPRESSED_FILE *compressed_file,
						 int bit_count)
{
	unsigned long mask;
	unsigned long return_value;

	mask = 1L << (bit_count - 1);
	return_value = 0;
	while (mask != 0)
	{
		if (compressed_file->mask == 0x80)
		{
			compressed_file->rack = getc(compressed_file->file);
			if (compressed_file->rack == EOF)
				fatal_error("Error on InputBits!\n");
			if ((compressed_file->pacifier_counter++ &
				PACIFIER_COUNT) == 0)
				putc('.', stdout);
		}
		if (compressed_file->rack & compressed_file->mask)
			return_value |= mask;
		mask >>= 1;
		compressed_file->mask >>= 1;
		if (compressed_file->mask == 0)
			compressed_file->mask = 0x80;
	}
	return (return_value);
}
//=========================================================
/*  С этого места начинается исходный текст,
 *  реализующий собственно алгоритм
 *  адаптивного кодирования Хаффмана
 */

TREE Tree; //Дерево адаптивного кодирования Хаффмена

// Функция преобразования входного файла в выходной сжатый файл
void CompressFile(FILE *input, COMPRESSED_FILE *output)
{
	int c;

	InitializeTree(&Tree);
	while ((c = getc(input)) != EOF)
	{
		EncodeSymbol(&Tree, c, output);
		UpdateModel(&Tree, c);
	}
	EncodeSymbol(&Tree, END_OF_STREAM, output);
}
//---------------------------------------------------------
// Процедура декомпрессии упакованного файла
void ExpandFile (COMPRESSED_FILE *input, FILE *output)
{
	int c;

	InitializeTree(&Tree);
	while ((c = DecodeSymbol(&Tree, input)) != END_OF_STREAM)
	{
		if (putc(c, output) == EOF)
			fatal_error("Error on output");
		UpdateModel(&Tree, c);
	}
}
//---------------------------------------------------------
/* Функция инициализации дерева
   Перед началом работы алгоритма дерево кодирования
   инициализируется двумя специальными (не ASCII) символами:
   ESCAPE и END_OF_STREAM.
   Также инициализируется корень дерева.
   Все листья инициализируются -1, так как они еще
   не присутствуют в дереве кодирования.
 */
void InitializeTree (TREE *tree)
{
	int i;

	tree->nodes[ ROOT_NODE ].child             = ROOT_NODE + 1;
	tree->nodes[ ROOT_NODE ].child_is_leaf     = FALSE;
	tree->nodes[ ROOT_NODE ].weight            = 2;
	tree->nodes[ ROOT_NODE ].parent            = -1;

	tree->nodes[ ROOT_NODE + 1 ].child         = END_OF_STREAM;
	tree->nodes[ ROOT_NODE + 1 ].child_is_leaf = TRUE;
	tree->nodes[ ROOT_NODE + 1 ].weight        = 1;
	tree->nodes[ ROOT_NODE + 1 ].parent        = ROOT_NODE;
	tree->leaf[ END_OF_STREAM ]                = ROOT_NODE + 1;

	tree->nodes[ ROOT_NODE + 2 ].child         = ESCAPE;
	tree->nodes[ ROOT_NODE + 2 ].child_is_leaf = TRUE;
	tree->nodes[ ROOT_NODE + 2 ].weight        = 1;
	tree->nodes[ ROOT_NODE + 2 ].parent        = ROOT_NODE;
	tree->leaf[ ESCAPE ]                       = ROOT_NODE + 2;

	tree->next_free_node                       = ROOT_NODE + 3;

	for (i = 0 ; i < END_OF_STREAM ; i++)
		tree->leaf[ i ] = -1;
}
//---------------------------------------------------------
/* Эта процедура преобразует входной символ в последовательность
   битов на основе текущего состояния дерева кодирования.
   Некоторое неудобство состоит в том, что, обходя дерево
   от листа к корню, мы получаем последовательность битов
   в обратном порядке, и поэтому необходимо аккумулировать биты
   в INTEGER переменной и выдавать их после того, как обход
   дерева закончен.
 */ 
void EncodeSymbol (TREE *tree, unsigned int c,
				   COMPRESSED_FILE *output)
{
	unsigned long code;
	unsigned long current_bit;
	int code_size;
	int current_node;

	code = 0;
	current_bit = 1;
	code_size = 0;
	current_node = tree->leaf[ c ];
	if (current_node == -1)
		current_node = tree->leaf[ ESCAPE ];
	while (current_node != ROOT_NODE)
	{
		if ((current_node & 1) == 0)
			code |= current_bit;
		current_bit <<= 1;
		code_size++;
		current_node = tree->nodes[ current_node ].parent;
	}
	OutputBits(output, code, code_size);
	if (tree->leaf[ c ] == -1)
	{
		OutputBits(output, (unsigned long) c, 8);
		add_new_node(tree, c);
	}
}
//---------------------------------------------------------
/* Процедура декодирования очень проста. Начиная от корня, мы
   обходим дерево, пока не дойдем до листа. Затем проверяем
   не прочитали ли мы ESCAPE код. Если да, то следующие 8 битов
   соответствуют незакодированному символу, который немедленно
   считывается и добавляется к таблице.
 */
int DecodeSymbol (TREE *tree, COMPRESSED_FILE *input)
{
	int current_node;
	int c;

	current_node = ROOT_NODE;
	while (!tree->nodes[ current_node ].child_is_leaf)
	{
		current_node = tree->nodes[ current_node ].child;
		current_node += InputBit(input);
	}
	c = tree->nodes[ current_node ].child;
	if (c == ESCAPE)
	{
		c = (int) InputBits(input, 8);
		add_new_node(tree, c);
	}
	return(c);
}
//---------------------------------------------------------
/* Процедура обновления модели кодирования для данного символа,
  пожалуй, самое сложное в адаптивном кодировании Хаффмана.
 */
void UpdateModel (TREE *tree, int c)
{
	int current_node;
	int new_node;

	if (tree->nodes[ ROOT_NODE].weight == MAX_WEIGHT)
		RebuildTree(tree);
	current_node = tree->leaf[ c ];
	while (current_node != -1)
	{
		tree->nodes[ current_node ].weight++;
		for (new_node=current_node;new_node>ROOT_NODE;new_node--)
			if (tree->nodes[ new_node - 1 ].weight >=
					tree->nodes[ current_node ].weight)
				break;
		if (current_node != new_node)
		{
			swap_nodes(tree, current_node, new_node);
			current_node = new_node;
		}
		current_node = tree->nodes[ current_node ].parent;
	}
}
//---------------------------------------------------------
/* Процедура перестроения дерева вызывается тогда, когда
   вес корня дерева достигает пороговой величины. Она
   начинается с простого деления весов узлов на 2. Но из-за
   ошибок округления при этом может быть нарушено свойство
   упорядоченности дерева кодирования, и необходимы
   дополнительные усилия, чтобы привести его в корректное
   состояние.
 */
void RebuildTree (TREE *tree)
{
	int i;
	int j;
	int k;
	unsigned int weight;

	printf("R");
	j = tree->next_free_node - 1;
	for (i = j ; i >= ROOT_NODE ; i--)
	{
		if (tree->nodes[ i ].child_is_leaf)
		{
			tree->nodes[ j ] = tree->nodes[ i ];
			tree->nodes[ j ].weight =
				(tree->nodes[ j ].weight + 1) / 2;
			j--;
		}
	}

	for (i = tree->next_free_node-2; j >= ROOT_NODE; i-=2, j--)
	{
		k = i + 1;
		tree->nodes[ j ].weight =
			tree->nodes[ i ].weight + tree->nodes[ k ].weight;
		weight = tree->nodes[ j ].weight;
		tree->nodes[ j ].child_is_leaf = FALSE;
		for (k = j + 1 ; weight < tree->nodes[ k ].weight ; k++)
			;
		k--;
		memmove(&tree->nodes[ j ], &tree->nodes[ j + 1 ],
			(k - j) * sizeof(struct node));
		tree->nodes[ k ].weight = weight;
		tree->nodes[ k ].child = i;
		tree->nodes[ k ].child_is_leaf = FALSE;
	}

	for (i = tree->next_free_node - 1 ; i >= ROOT_NODE ; i--)
	{
		if (tree->nodes[ i ].child_is_leaf)
		{
			k = tree->nodes[ i ].child;
			tree->leaf[ k ] = i;
		}
		else
		{
			k = tree->nodes[ i ].child;
			tree->nodes[ k ].parent =
				tree->nodes[ k + 1 ].parent = i;
		}
	}
}
//---------------------------------------------------------
/* Процедура перестановки узлов дерева вызывается тогда, когда
   очередное увеличение веса узла привело к нарушению свойства
   упорядоченности.
*/
void swap_nodes (TREE *tree, int i, int j)
{
	node temp;

	if (tree->nodes[ i ].child_is_leaf)
		tree->leaf[ tree->nodes[ i ].child ] = j;
	else
	{
		tree->nodes[ tree->nodes[ i ].child ].parent = j;
		tree->nodes[ tree->nodes[ i ].child + 1 ].parent = j;
	}
	if (tree->nodes[ j ].child_is_leaf)
		tree->leaf[ tree->nodes[ j ].child ] = i;
	else
	{
		tree->nodes[ tree->nodes[ j ].child ].parent = i;
		tree->nodes[ tree->nodes[ j ].child + 1 ].parent = i;
	}
	temp = tree->nodes[ i ];
	tree->nodes[ i ] = tree->nodes[ j ];
	tree->nodes[ i ].parent = temp.parent;
	temp.parent = tree->nodes[ j ].parent;
	tree->nodes[ j ] = temp;
}
//---------------------------------------------------------
/* Добавление нового узла в дерево осуществляется достаточно
   просто. Для этого "самый легкий" узел дерева разбивается
   на 2, один из которых и есть тот новый узел. Новому узлу
   присваивается вес 0, котрый будет изменен потом, при
   нормальном процессе обновления дерева.
 */
void add_new_node (TREE *tree, int c)
{
	int lightest_node;
	int new_node;
	int zero_weight_node;

	lightest_node = tree->next_free_node - 1;
	new_node = tree->next_free_node;
	zero_weight_node = tree->next_free_node + 1;
	tree->next_free_node += 2;

	tree->nodes[ new_node ] = tree->nodes[ lightest_node ];
	tree->nodes[ new_node ].parent = lightest_node;
	tree->leaf[ tree->nodes[ new_node ].child ] = new_node;

	tree->nodes[ lightest_node ].child = new_node;
	tree->nodes[ lightest_node ].child_is_leaf = FALSE;

	tree->nodes[ zero_weight_node ].child = c;
	tree->nodes[ zero_weight_node ].child_is_leaf = TRUE;
	tree->nodes[ zero_weight_node ].weight = 0;
	tree->nodes[ zero_weight_node ].parent = lightest_node;
	tree->leaf[ c ] = zero_weight_node;
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

int main(){
   // компрессия
   
    ofstream fRes("data.xls", ios::out);
    unsigned int count_measurement = 0;
    int a[100];
    
    do{
        for (int i=0; i<100; i++){
            COMPRESSED_FILE *output;
            FILE *input;

             createMessage(count_measurement);
            // Компрессия:
            input = fopen("data.dat", "rb");
            output = OpenOutputCompressedFile("out.dat");

				// оценка загрузки процессора в тактах
				unsigned long start_ticks = rdtsc();
                CompressFile(input, output);
				unsigned long diff_ticks = rdtsc() - start_ticks;

        
            CloseOutputCompressedFile(output);
            fclose(input);
    
            //a[i] = filesize("out.dat"); // размер файла после сжатия
            a[i] = diff_ticks;

            cout << count_measurement << " - " << i << endl;
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
