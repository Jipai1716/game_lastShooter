#ifndef ITEM_H
#define ITEM_H
#include"ZJYmath.h"
#include"object.h"
int kill=0;
bool ItemFlag=0;
item spdUP(1,300,300),//加速道具
	 atkUP(2,300,300),//攻击道具
	 aspUP(3,300,300),//攻速道具
	 heartUP(4,100,100),//
	 boomerUP(5,200,200);
void summonitem_spdup()//道具刷新:加速道具 
{
	if(spdUP.getalive()==0&&(kill==3||kill==20||kill==50||kill==70)&&ItemFlag==1)//条件刷新道具 
	{
		spdUP.changeAlive();
		spdUP.changeX(ran());
		spdUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_ATKup()//道具刷新:加速道具 
{
	if(spdUP.getalive()==0&&(kill==4||kill==30||kill==45)&&ItemFlag==1)//条件刷新道具 
	{
		atkUP.changeAlive();
		atkUP.changeX(ran());
		atkUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_Aspup()//道具刷新:加速道具 
{
	if(aspUP.getalive()==0&&(kill==15||kill==55)&&ItemFlag==1)//条件刷新道具 
	{
		aspUP.changeAlive();
		aspUP.changeX(ran());
		aspUP.changeY(ran());
		ItemFlag=0;
		
	}
}
void summonitem_heartup()//道具刷新:加速道具 
{
	if(heartUP.getalive()==0&&(WAVE%3==2&&WAVE!=0)&&ItemFlag==1)//条件刷新道具 
	{
		heartUP.changeAlive();
		heartUP.changeX(ran());
		heartUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_boomerUP()//道具刷新
{
	if(boomerUP.getalive()==0&&(WAVE%3==0&&WAVE!=0)&&ItemFlag==1)//条件刷新道具 
	{
		boomerUP.changeAlive();
		boomerUP.changeX(ran());
		boomerUP.changeY(ran());
		ItemFlag=0;
	}
}

#endif
