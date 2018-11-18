// ArAl.cpp: ������� ���� �������.

#include "stdafx.h"

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <intrin.h> 

using namespace std;

class Message {
private:
    short int ultrasonic_distance_sensor[5]; //���� �������������� �������� ����������
    short int infrared_distance_sensor[4]; //������ ������������ ������� ����������
    float accelerometer[3]; //��� ��� �������������
    short int gyroscope[4][3]; //������ ���������
public:
    Message(){
        // �������� ��������� �� 15 �� 645 ��
        ultrasonic_distance_sensor[0] = 15 + rand()%630; 
        ultrasonic_distance_sensor[1] = 15 + rand()%630; 
        ultrasonic_distance_sensor[2] = 15 + rand()%630; 
        ultrasonic_distance_sensor[3] = 15 + rand()%630; 
        ultrasonic_distance_sensor[4] = 15 + rand()%630; 
        
        // �������� �� 10 �� 80 ��
        infrared_distance_sensor[0] = 10 + rand()%70; 
        infrared_distance_sensor[1] = 10 + rand()%70; 
        infrared_distance_sensor[2] = 10 + rand()%70; 
        infrared_distance_sensor[3] = 10 + rand()%70; 
        
        // �������� �� -11g �� 11g
        accelerometer[0] = -11 + rand()%22; 
        accelerometer[1] = -11 + rand()%22; 
        accelerometer[2] = -11 + rand()%22; 
        
        // �������� �� -2000 �� 2000 �/���
        gyroscope[0][0] = -2000 + rand()%4000; 
        gyroscope[0][1] = -2000 + rand()%4000; 
        gyroscope[0][2] = -2000 + rand()%4000; 
        gyroscope[1][0] = -2000 + rand()%4000; 
        gyroscope[1][1] = -2000 + rand()%4000; 
        gyroscope[1][2] = -2000 + rand()%4000; 
        gyroscope[2][0] = -2000 + rand()%4000; 
        gyroscope[2][1] = -2000 + rand()%4000; 
        gyroscope[2][2] = -2000 + rand()%4000; 
        gyroscope[3][0] = -2000 + rand()%4000; 
        gyroscope[3][1] = -2000 + rand()%4000; 
        gyroscope[3][2] = -2000 + rand()%4000; 
    }
};

void createMessage(unsigned int count_measurement) {
    const char *FName="data.dat"; 
    ofstream fout(FName, ios::binary | ios::out);
    if (count_measurement == 0) count_measurement = 1;

    for (int i=0; i<count_measurement; i++){
        Message m1;
        fout.write((char*)&m1, sizeof(m1));
    }
    fout.close(); // ��������� ����    
};

ifstream::pos_type filesize(const char* filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    return in.tellg(); 
}

//---------------------------------------------------------
// ������� ���������
int main()
{
    ofstream fRes("result.xls", ios::out); // ����� �����������
    unsigned int count_measurement = 0; // ��������� ���������� ���������

    fRes << "\t\tArithmetic coding" 
            << "\t\t\t\t LZW" 
            << "\t\t\t\t Huffman's Algorithm" 
            << "\t\t\t\t bzip2(BWT + RLE)" 
            << "\t\t\t\t LZ77" 
            << "\t\t\t\t LZ78" 
            << "\t\t\t\t RAR (LZSS)" 
            << "\t\t\t\t Deflate (LZHuf)" 
            << "\t\t\t\t LZAri" 
		<< endl;

    fRes << "Count messurement \t Size in file \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
            << "Size out file \t Compress, % \t Tacts \t RAM \t"
		<< endl;
    
	do{
		int sizeArithmeticCoding = 0; // ������� ������ ����� ���������� ��������� ��������������� �����������
		unsigned long cpuArithmeticCoding = 0; // ���������� ������ �� �������������� �����������
		int ramArithmeticCoding = 0; // ����������� RAM �� �������������� �����������

		int sizeHuffman = 0; // ������� ������ ����� ���������� ������ ��������
		unsigned long cpuHuffman = 0; // ���������� ������ ����� ��������
		int ramHuffman = 0; // ����������� RAM �� ����� ��������

		int sizeLZW = 0; // ������� ������ ����� ���������� ������ LZW
		unsigned long cpuLZW = 0; // ���������� ������ ����� LZW
		int ramLZW = 0; // ����������� RAM ����� ���������� ������ LZW
	    
		int sizeBzip2 = 0; // ������� ������ ����� ���������� ������ bzip2
		unsigned long cpuBzip2 = 0; // ���������� ������ ����� bzip2
		int ramBzip2 = 0; // ����������� RAM ����� ���������� ������ bzip2

		int sizeLZ77 = 0; // ������� ������ ����� ���������� ������ LZ77
		unsigned long cpuLZ77 = 0; // ���������� ������ ����� LZ77
		int ramLZ77 = 0; // ����������� RAM ����� ���������� ������ LZ77

		int sizeLZ78 = 0; // ������� ������ ����� ���������� ������ LZ78
		unsigned long cpuLZ78 = 0; // ���������� ������ ����� LZ78
		int ramLZ78 = 0; // ����������� RAM ����� ���������� ������ LZ78

		int sizeRAR = 0; // ������� ������ ����� ���������� ������ RAR
		unsigned long cpuRAR = 0; // ���������� ������ ����� RAR
		int ramRAR = 0; // ����������� RAM ����� ���������� ������ RAR

		int sizeDeflate = 0; // ������� ������ ����� ���������� ������ Deflate
		unsigned long cpuDeflate = 0; // ���������� ������ ����� Deflate
		int ramDeflate = 0; // ����������� RAM ����� ���������� ������ Deflate

		int sizeLZAri = 0; // ������� ������ ����� ���������� ������ LZAri
		unsigned long cpuLZAri = 0; // ���������� ������ ����� LZAri
		int ramLZAri = 0; // ����������� RAM ����� ���������� ������ LZAri

		unsigned long start_ticks;

		for (int i=0; i<100; i++){
            cout << count_measurement << " - " << i << endl;
            createMessage(count_measurement);

			cout << "Arithmetic coding" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("Arith_adapt e data.dat out.dat");
			cpuArithmeticCoding += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeArithmeticCoding += filesize("out.dat");

			cout << "LZW" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("lzw e data.dat out.dat");
			cpuLZW += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeLZW += filesize("out.dat");

			cout << "Huffman's Algorithm" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("HuffAdapt e data.dat out.dat");
			cpuHuffman += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeHuffman += filesize("out.dat");
			
			cout << "bzip2" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
			system("BWT e data.dat bwt.dat");
			system("RLE e bwt.dat bwt.dat");
			cpuBzip2 += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeBzip2 += filesize("out.dat");

			cout << "LZ77" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("LZ77 e data.dat out.dat");
			cpuLZ77 += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeLZ77 += filesize("out.dat");

			cout << "LZ78" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("LZ78 e data.dat out.dat");
			cpuLZ78 += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeLZ78 += filesize("out.dat");

			cout << "RAR" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("LZSS e data.dat out.dat");
			cpuRAR += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeRAR += filesize("out.dat");
			
			cout << "Deflate" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("LZHuf e data.dat out.dat");
			cpuDeflate += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeDeflate += filesize("out.dat");
			
			cout << "LZAri" << endl;
			start_ticks = __rdtsc(); // ������ �������� ���������� � ������
            system("LZAri e data.dat out.dat");
			cpuLZAri += (__rdtsc() - start_ticks); // ������ �������� ���������� � ������ ����� ���������� ���������
			sizeLZAri += filesize("out.dat");
				
        }

		sizeArithmeticCoding	/= 100;
		sizeLZW					/= 100;
		sizeHuffman				/= 100;
		sizeBzip2				/= 100;
		sizeLZ77				/= 100;
		sizeLZ78				/= 100;
		sizeRAR					/= 100;
		sizeDeflate				/= 100;
		sizeLZAri				/= 100;
		int sizeIn = filesize("data.dat");

        fRes << count_measurement << "\t" << sizeIn << "\t"
			<< sizeArithmeticCoding		<< "\t" << 1.0 - (float(sizeArithmeticCoding) / float(sizeIn))		<< "\t" << fixed << cpuArithmeticCoding	<< "\t" << ramArithmeticCoding << "\t"
			<< sizeLZW					<< "\t" << 1.0 - (float(sizeLZW) / float(sizeIn))					<< "\t" << fixed << cpuLZW				<< "\t" << ramLZW << "\t"
			<< sizeHuffman				<< "\t" << 1.0 - (float(sizeHuffman) / float(sizeIn))				<< "\t" << fixed << cpuHuffman			<< "\t" << ramHuffman << "\t"
			<< sizeBzip2				<< "\t" << 1.0 - (float(sizeBzip2) / float(sizeIn))					<< "\t" << fixed << cpuBzip2			<< "\t" << ramBzip2 << "\t"
			<< sizeLZ77					<< "\t" << 1.0 - (float(sizeLZ77) / float(sizeIn))					<< "\t" << fixed << cpuLZ77				<< "\t" << ramLZ77 << "\t"
			<< sizeLZ78					<< "\t" << 1.0 - (float(sizeLZ78) / float(sizeIn))					<< "\t" << fixed << cpuLZ78				<< "\t" << ramLZ78 << "\t"
			<< sizeRAR					<< "\t" << 1.0 - (float(sizeRAR) / float(sizeIn))					<< "\t" << fixed << cpuRAR				<< "\t" << ramRAR << "\t"
			<< sizeDeflate				<< "\t" << 1.0 - (float(sizeDeflate) / float(sizeIn))				<< "\t" << fixed << cpuDeflate			<< "\t" << ramDeflate << "\t"
			<< sizeLZAri				<< "\t" << 1.0 - (float(sizeLZAri) / float(sizeIn))					<< "\t" << fixed << cpuLZAri			<< "\t" << ramLZAri << "\t"
        << endl;
        count_measurement += 30;
    } while (count_measurement < 600);
    
    fRes.close();
    return 0;
}