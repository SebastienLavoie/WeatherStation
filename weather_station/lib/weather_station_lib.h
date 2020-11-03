#ifndef __WEATHER__STATION__LIB__
#define __WEATHER__STATION__LIB__

#include "Arduino.h"

#define TEMP_IDX 0
#define HUM_IDX 1
#define VOLTAGE_IDX 2
#define PERCENT_IDX 3

void quicksort(float* tab, int L, int R);
int pivot_split(float* tab, int L, int R);

class Scout
{
    public:
        Scout(float t, float h, float v, float p);
        float get_value(int i);
        void set_value(int i, float val);
    private:
        float temperature;
        float humidity;
        float voltage;
        float percent;
};



#endif