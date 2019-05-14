#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;
int main()
{
  cout << "Start!" << endl;
  ifstream file ("AirQualityUCIsed.csv");
  if (!file){
    cout << "File with problem" << endl;
  } else {
    cout << "All right!" << endl;
  }

  string date, time, header, cell;
  float co, c6h6, t, rh, ah;
  int pts1, nmhc,pts2, no, pts3, no2, pts4, o3;

  getline(file, header);
  
  for (int i=0; i<3; i++){
  // Read all rows 
    getline(file, date, ';');
    getline(file, time, ';');

    getline(file, cell, ';'); co = atof(cell.c_str());
    getline(file, cell, ';'); pts1 = atof(cell.c_str());
    getline(file, cell, ';'); nmhc = atof(cell.c_str());
    getline(file, cell, ';'); c6h6 = atof(cell.c_str());
    getline(file, cell, ';'); pts2 = atof(cell.c_str());
    getline(file, cell, ';'); no = atof(cell.c_str());
    getline(file, cell, ';'); pts3 = atof(cell.c_str());
    getline(file, cell, ';'); no2 = atof(cell.c_str());
    getline(file, cell, ';'); pts4 = atof(cell.c_str());
    getline(file, cell, ';'); o3 = atof(cell.c_str());
    getline(file, cell, ';'); t = atof(cell.c_str());
    getline(file, cell, ';'); rh = atof(cell.c_str());
    getline(file, cell, ';'); ah = atof(cell.c_str());

    getline(file, cell); // The tail of the string
  }

  return 0;
}
