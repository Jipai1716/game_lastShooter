#ifndef ZJYmath_H
#define ZJYmath_H
#include<time.h>
#include<math.h>
#include<iostream>
#include<stdlib.h>
void VectorUnitization(double &x,double &y)
{
	double z;
	z=x*x+y*y;
	z=sqrt(z);
	x/=z;
	y/=z;
}

int ran()//随机数生成函数（道具用） 
{
	time_t t;
    time(&t);
	int seed=clock();
	srand(seed);
	int randoxNumber = rand() % 800 ;
	return randoxNumber;
}
int ranX(int x)//随机数生成函数（道具用） 
{
	time_t t;
    time(&t);
	int seed=clock();
	seed+=x;
	srand(seed);
	int randoxNumber = rand() % 800 ;
	return randoxNumber;
}
#endif
