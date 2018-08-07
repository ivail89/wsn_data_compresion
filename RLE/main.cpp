// RLE.cpp 

#include <stdio.h> 
#include <memory.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <cstdlib>

/* Коды ошибок */
#define NO_ERROR      0
#define BAD_FILE_NAME 1
#define BAD_ARGUMENT  2

using namespace std;

#define FALSE 0
#define TRUE  1

/* входной и выходной файлы */
    FILE *source_file, *dest_file;

//FILE *source_file = fopen("data.dat", "r");
//    FILE *dest_file = fopen("out.dat", "w");

/*
'byte_stored_status' равен 'TRUE'если байт
прочитан 'fgetc'или 'FALSE' если нет дейтвительного
байта, не прочитано и не содержится в 'val_byte_stored'
*/

int byte_stored_status=FALSE;
int val_byte_stored;


/* псевдопроцедуры */

#define end_of_data() (byte_stored_status?FALSE:\
	!(byte_stored_status=\
	((val_byte_stored=fgetc(source_file))!=EOF)))
	
#define read_byte()  (byte_stored_status?\
	byte_stored_status=FALSE,(unsigned char)val_byte_stored:\
	(unsigned char)fgetc(source_file))
	
#define write_byte(byte)  ((void)fputc((byte),dest_file))

#define write_array(array,byte_nb_to_write)\
	  ((void)fwrite((array),1,(byte_nb_to_write),dest_file))

#define write_block(byte,time_nb) { unsigned char array_to_write[129]; (void)memset(array_to_write,(byte),(time_nb)); write_array(array_to_write,(time_nb)); }
//=========================================================
// Сжатие методом RLE
void rle1encoding(){
    register unsigned char byte1, byte2, frame_size, array[129];

 if (!end_of_data())
 {
  byte1=read_byte();
  frame_size=1;
  if (!end_of_data())
  {
   byte2=read_byte();
   frame_size=2;
   do {
    if (byte1==byte2)
    {
     while ((!end_of_data())&&(byte1==byte2)
      &&(frame_size<129))
     {
      byte2=read_byte();
      frame_size++;
     }
     if (byte1==byte2) /* встретили
           последовательность
           одинаковых байт */
     {
      write_byte(126+frame_size);
      write_byte(byte1);
      if (!end_of_data())
      {
       byte1=read_byte();
       frame_size=1;
      }
      else frame_size=0;
     } else
     {
      write_byte(125+frame_size);
      write_byte(byte1);
      byte1=byte2;
      frame_size=1;
     }
     if (!end_of_data())
     {
      byte2=read_byte();
      frame_size=2;
     }
    } else /* подготовка массива для
        сравнений, в нем будут
        храниться все идентичные байты */
    {
     *array = byte1;
     array[1]=byte2;
     while ((!end_of_data())&&(array[frame_size-2]!=
      array[frame_size-1])&&(frame_size<128))
     {
      array[frame_size]=read_byte();
      frame_size++;
     }
     if (array[frame_size-2]==array[frame_size-1])
      /* встретилась ли последовательность из разных байт,
      следующих за одинаковыми? */
     { /* Да, тогда не считаем 2 последних байта */
      write_byte(frame_size-3);
      write_array(array,frame_size-2);
      byte1=array[frame_size-2];
      byte2=byte1;
      frame_size=2;
     } else
     {
      write_byte(frame_size-1);
      write_array(array,frame_size);
      if (end_of_data())
       frame_size=0;
      else {
       byte1=read_byte();
       if (end_of_data())
        frame_size=1;
       else {
        byte2=read_byte();
        frame_size=2;
       }
      }
     }
    }
   } while ((!end_of_data())||(frame_size>=2));
  }
  if (frame_size==1)
  {
   write_byte(0);
   write_byte(byte1);
  }
 }
} 
//---------------------------------------------------------
// Декомпрессия методом RLE
void rle1decoding()
{
 unsigned char header;
 register unsigned char i;

 while (!end_of_data())
 {
  header=read_byte();
  switch (header & 128)
  {
   case 0:
    if (!end_of_data())
     for (i=0;i<=header;i++)
      write_byte(read_byte());
    /* else INVALID FILE */
    break;
   case 128:
    if (!end_of_data())
     write_block(read_byte(),(header & 127)+2);
    /* else INVALID FILE */
  }
 }
}
//---------------------------------------------------------
// Вывод помощи о пользовании программой
void help()
{
 printf("RLE_01 e(encoding)|d(decoding) source target\n");
} 

//---------------------------------------------------------
struct Message {
    float ultrasonic_distance_sensor[5]; //пять ультразвуковых датчиков расстояния
    float infrared_distance_sensor[4]; //четыре инфракрасных датчика расстояния
    float accelerometer[3]; //три оси акселерометра
    float gyroscope[4][3]; //четыре гироскопа
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

// главная функция
int main(int argc, char** argv) {

    ofstream fRes("data.xls", ios::out);
    unsigned int count_measurement = 0;
    int a[100];
    
    do{
        for (int i=0; i<100; i++){
            source_file=fopen("data.dat","rb");
            dest_file=fopen("out.dat","wb");

            createMessage(count_measurement);
            // Компрессия:
            rle1encoding();

            fclose(source_file);
            fclose(dest_file);

            a[i] = filesize("out.dat");
            cout << count_measurement << " - " << i << endl;
        }
        float medium =0;
        for (int i=0; i<100; i++){
            medium += a[i];
        }
        medium = medium / 100;
        fRes << count_measurement << "\t" << filesize("data.dat") << "\t"
                << medium << endl;
        count_measurement += 30;
    } while (count_measurement < 600);
    
    fRes.close();
    
    
    
    return 0;
}