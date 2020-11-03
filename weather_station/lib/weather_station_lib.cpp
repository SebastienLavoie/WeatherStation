#include "Arduino.h"
#include "weather_station_lib.h"


Scout::Scout(float t, float h, float v, float p)
{
        temperature = t;
        humidity = h;
        voltage = v;
        percent = p;
}

float Scout::get_value(int i)
{
    switch(i) {
        case TEMP_IDX:
            return temperature;
        case HUM_IDX:
            return humidity;
        case VOLTAGE_IDX:
            return voltage;
        case PERCENT_IDX:
            return percent;
        default:
            return -1.0;
    }
}

void Scout::set_value(int i, float val)
{
    switch(i) {
        case TEMP_IDX:
            temperature = val;
            break;
        case HUM_IDX:
            humidity = val;
            break;
        case VOLTAGE_IDX:
            voltage = val;
            percent = ((voltage - 3.3) / 0.9) * 100; // map voltage to percentage of charge
            break;
        case PERCENT_IDX:
            percent = val;
            break;
        default:
            break;
    }
}


void quicksort(float* tab, int L, int R){
    if(L < R){
    int P = pivot_split(tab, L, R);
    quicksort(tab, L, P-1);
    quicksort(tab, P+1, R);
  }
}

int pivot_split(float* tab, int L, int R){
  int x = tab[R];
  int i = L-1;
  int tmp;
  
  for(int j=L; j <= R-1; j++){
    if (tab[j] < x){
      i++;
      tmp = tab[i];
      tab[i] = tab[j];
      tab[j] = tmp;
    }
  }
  tmp = tab[i+1];
  tab[i+1] = tab[R];
  tab[R] = tmp;

  return i+1;
} 