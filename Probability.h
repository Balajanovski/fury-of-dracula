//interface for Gaussian Probability ADT
//By Andrew Hu

#ifndef PROBABILITY_H
#define PROBABILITY_H

double findMean(float values[], int size);
double findVariance(float values[], double mean, int size);
double findSTDdeviation(double variance);

double gaussianDensity(double x, double mean, double vari, double STDdev);
double getRadiusProbability(double loB, double upB, double mean, double vari, double STDdev);


#endif