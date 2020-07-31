#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "Probability.h"

static double const SQROOT_2PI = M_SQRT1_2 * M_2_SQRTPI;

double findMean(int values[], int size)
{
    int i, usedValue = 0; double sum = 0;
    for ( i = 0; i < size; i++ )
        if (values[i] != -1) {
            sum += values[i];
            usedValue++;
        }
    return sum / usedValue;
}
 
double findVariance(int values[], double mean, int size)
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
   double numerator = exp(-0.5 * ((x - mean) * (x - mean)) / ((vari) * (vari)));
   double denominator = SQROOT_2PI * STDdev;
   return  numerator / denominator;
}

double getRadiusProbability(double loB, double upB, double mean, double vari, double STDdev)
{

    double areaProb = 0, n_strips = 10;
    float h = (upB - loB) / 10;

    for (int i = loB; i < upB; i += h) {
        areaProb += getGaussianDensity(i, mean, vari, STDdev);
    }
    return areaProb;
}