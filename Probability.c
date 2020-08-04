#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "Probability.h"

#define INTEG_BARS 1000

double findMean(float values[], int size)
{
    int i, usedValue = 0; double sum = 0;
    for ( i = 0; i < size; i++ )
        if (values[i] != -1) {
            sum += values[i];
            usedValue++;
        }
    return sum / usedValue;
}
 
double findVariance(float values[], double mean, int size)
{
    double squareDiff = 0; int usedValue = 0;
    for (int i = 0; i < size; i++)  
        if (values[i] != -1) {
            squareDiff += (values[i] - mean) * (values[i] - mean); 
            usedValue++;
        }
    return squareDiff / usedValue;
}

double findSTDdeviation(double variance)
{
    return sqrt(variance);
}

double getGaussianDensity(double x, double mean, double vari, double STDdev)
{
   
   double denom = 0.5 * M_SQRT1_2 * M_2_SQRTPI;
   return (1 / STDdev) * denom * exp(-0.5 * (x - mean) * (x - mean) / (vari));
}

double getRadiusProbability(double loB, double upB, double mean, double vari, double STDdev)
{
    double areaProb = 0;
    double h = (upB - loB) / INTEG_BARS;
    for (double i = loB; i < upB; i += h) {
        areaProb += getGaussianDensity(i, mean, vari, STDdev) * h;
    }
    return areaProb;
}