#include <iostream>
#include <fstream>
#include <string>

using namespace std;
int main()
{
  cout << "Start!" << endl;
  ifstream file ("AirQualityUCI.csv");
  if (!file){
    cout << "File with problem" << endl;
  } else {
    cout << "All right!" << endl;
  }

  string date, time, header;
  float co, c6h6, t, rh, ah;
  int pts1, nmhc,pts2, no, pts3, no2, pts4, o3;

  getline(file, header);
  file >> date >> time >> co >> pts1 >> nmhc >> c6h6 >> pts2 >> no >>
    pts3 >> no2 >> pts4 >> o3 >> t >> rh >> ah;

  cout << header << endl;
  cout << date << ah << endl;
  return 0;
}
