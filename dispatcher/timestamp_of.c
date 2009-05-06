#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
  if (argc != 2)
    return -1;
  
  int month_borders[] =
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365,
      31+365, 59+365, 90+365, 120+365, 151+365, 181+365,
      212+365, 243+365, 273+365, 304+365, 334+365, 365+365,
      31+2*365, 59+2*365, 90+2*365, 120+2*365, 151+2*365, 181+2*365,
      212+2*365, 243+2*365, 273+2*365, 304+2*365, 334+2*365, 365+2*365,
      31+3*365, 60+3*365, 91+3*365, 121+3*365, 152+3*365, 182+3*365,
      213+3*365, 244+3*365, 274+3*365, 305+3*365, 335+3*365, 366+3*365 };
  
  int hour_number(atoi(argv[1]));
  int day_number(hour_number / 24);
  int four_year_remainder(day_number % (4 * 365));
  int year(day_number / 365 / 4 + 2009);
  
  int month(1);
  while (four_year_remainder >= month_borders[month])
    ++month;
  
  hour_number = hour_number % 24;
  int day(four_year_remainder - month_borders[month-1] + 1);
  year += (month-1) / 12;
  month = (month-1) % 12 + 1;
  
  cout<<(long long)year*1000000 + month*10000 + day*100 + hour_number<<'\n';
}
