/*
 * Сравнение алгоритмов LZW, Хаффмана, Арифсетического кодирования
 */

#include <cstdlib>
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <vector>

#include "Arithmetic_coding.h"
#include "Huffman.h"
#include "LZW.h"

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

// Эта структура данных нужна для побитового доступа к файлам
typedef struct bit_file
{
	FILE *file;
	unsigned char mask;
	int rack;
	int pacifier_counter;
}
COMPRESSED_FILE;

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
    ofstream fRes("data.xls", ios::out); // вывод результатов
    unsigned int count_measurement = 0; // начальное количество измерений
    float arithmeticCoding[100]; // размеры файлов после применения архивации Арифсетического кодирования
    unsigned long cpuArithmeticCoding[100]; // количество таквтов на Арифсетическое кодирование
    float Huffman[100]; // размеры файлов после применения метода Хаффмана
    unsigned long cpuHuffman[100]; // количество таквтов метод Хаффмана
    float LZW[100]; // размеры файлов после применения метода Хаффмана
    unsigned long cpuLZW[100]; // количество таквтов метод Хаффмана
    
    fRes << "\t" << "\t" << "Arithmetic coding" 
            << "\t" << "\t" << "LZW" 
            << "\t" << "\t" << "Huffman's Algorithm" << endl;

    fRes << "Count messurement" << "\t" << "Size source file" << "\t"
            << "Compress in %" << "\t" << "Tacts" << "\t"
            << "Compress in %" << "\t" << "Tacts" << "\t"
            << "Compress in %" << "\t" << "Tacts" << endl;
    
    do{
        for (int i=0; i<100; i++){
            createMessage(count_measurement);

            setbuf( stdout, NULL );
            
            /* Арифметическое кодирование */
            unsigned long start_ticks = rdtsc(); // оценка загрузки процессора в тактах
            encode ("data.dat", "out.dat"); // Компрессия
            cpuArithmeticCoding[i] = rdtsc() - start_ticks; // оценка загрузки процессора в тактах после выполнения алгоритма
            arithmeticCoding[i] = 1.0 - float( float(filesize("out.dat")) / float(filesize("data.dat")) );// качество сжатия
            /* ---------------------------- */

            /* метод Хаффмана */
            start_ticks = rdtsc(); // оценка загрузки процессора в тактах
            encode_huffman ("data.dat", "out.dat"); // Компрессия
            cpuHuffman[i] = rdtsc() - start_ticks; // оценка загрузки процессора в тактах после выполнения алгоритма
            Huffman[i] = 1.0 - float( float(filesize("out.dat")) / float(filesize("data.dat")) );// качество сжатия
            /* ---------------------------- */
            
            /* метод LZW */
            start_ticks = rdtsc(); // оценка загрузки процессора в тактах
            call_compress("data.dat", "out.dat"); // Компрессия
            cpuLZW[i] = rdtsc() - start_ticks; // оценка загрузки процессора в тактах после выполнения алгоритма
            LZW[i] = 1.0 - float( float(filesize("out.dat")) / float(filesize("data.dat")) );// качество сжатия
            /* ---------------------------- */
            
            cout << count_measurement << " - " << i << endl;
        }
        
        fRes << count_measurement << "\t" << filesize("out.dat") << "\t";
        
        //средний размер файла после сжатия и среднее количество тактов (после 100 итереаций)
        float mediumArithmeticCoding = 0;
        float mediumCPUArithmeticCoding = 0;
        float mediumHuffman = 0;
        float mediumCPUHuffman = 0;
        float mediumLZW = 0;
        float mediumCPULZW = 0;
        for (int i=0; i<100; i++){
            mediumArithmeticCoding += arithmeticCoding[i];
            mediumCPUArithmeticCoding += cpuArithmeticCoding[i];
            
            mediumLZW += LZW[i];
            mediumCPULZW += cpuLZW[i];

            mediumHuffman += Huffman[i];
            mediumCPUHuffman += cpuHuffman[i];
        }
        mediumArithmeticCoding = mediumArithmeticCoding / 100;
        fRes << mediumArithmeticCoding << "\t";
        mediumCPUArithmeticCoding = mediumCPUArithmeticCoding / 100;
        fRes << fixed << mediumCPUArithmeticCoding << "\t";

        mediumLZW = mediumLZW / 100;
        fRes << mediumLZW << "\t";
        mediumCPULZW = mediumCPULZW / 100;
        fRes << fixed << mediumCPULZW << "\t";

        mediumHuffman = mediumHuffman / 100;
        fRes << mediumHuffman << "\t";
        mediumCPUHuffman = mediumCPUHuffman / 100;
        fRes << fixed << mediumCPUHuffman << "\t";
        
        fRes << endl;
        count_measurement += 30;
    } while (count_measurement < 600);
    
    fRes.close();

    return 0;
}

