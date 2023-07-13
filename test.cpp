#include<graphics.h>
#include<easyx.h>
#include<conio.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>

int main()
{
	initgraph(500,500);
	MOUSEMSG m;
	m=GetMouseMsg();
	printf("%d,%d\n",m.x,m.y);	
} 
