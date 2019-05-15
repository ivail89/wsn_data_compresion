#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

struct Row {
  string date, time;
  float co, c6h6, t, rh, ah;
  int pts1, nmhc,pts2, no, pts3, no2, pts4, o3;
};

int main()
{
  cout << "Start!" << endl;
  ifstream file ("AirQualityUCIsed.csv");
  ofstream data ("data.dat", ios::binary);
  if (!file){
    cout << "File with problem" << endl;
  } else {
    cout << "All right!" << endl;
  }

  string header, cell;
  Row r;

  getline(file, header);
  
  for (int i=0; i<3; i++){
  // Read all rows 
    getline(file, r.date, ';');
    getline(file, r.time, ';');

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
  }
  data.write((char*)&r, sizeof(r));
  data.close();

  return 0;
}
