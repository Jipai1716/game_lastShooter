#ifndef ITEM_H
#define ITEM_H
#include"ZJYmath.h"
#include"object.h"
int kill=0;
bool ItemFlag=0;
item spdUP(1,300,300),//���ٵ���
	 atkUP(2,300,300),//��������
	 aspUP(3,300,300),//���ٵ���
	 heartUP(4,100,100),//
	 boomerUP(5,200,200);
void summonitem_spdup()//����ˢ��:���ٵ��� 
{
	if(spdUP.getalive()==0&&(kill==3||kill==20||kill==50||kill==70)&&ItemFlag==1)//����ˢ�µ��� 
	{
		spdUP.changeAlive();
		spdUP.changeX(ran());
		spdUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_ATKup()//����ˢ��:���ٵ��� 
{
	if(spdUP.getalive()==0&&(kill==4||kill==30||kill==45)&&ItemFlag==1)//����ˢ�µ��� 
	{
		atkUP.changeAlive();
		atkUP.changeX(ran());
		atkUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_Aspup()//����ˢ��:���ٵ��� 
{
	if(aspUP.getalive()==0&&(kill==15||kill==55)&&ItemFlag==1)//����ˢ�µ��� 
	{
		aspUP.changeAlive();
		aspUP.changeX(ran());
		aspUP.changeY(ran());
		ItemFlag=0;
		
	}
}
void summonitem_heartup()//����ˢ��:���ٵ��� 
{
	if(heartUP.getalive()==0&&(WAVE%3==2&&WAVE!=0)&&ItemFlag==1)//����ˢ�µ��� 
	{
		heartUP.changeAlive();
		heartUP.changeX(ran());
		heartUP.changeY(ran());
		ItemFlag=0;
	}
}
void summonitem_boomerUP()//����ˢ��
{
	if(boomerUP.getalive()==0&&(WAVE%3==0&&WAVE!=0)&&ItemFlag==1)//����ˢ�µ��� 
	{
		boomerUP.changeAlive();
		boomerUP.changeX(ran());
		boomerUP.changeY(ran());
		ItemFlag=0;
	}
}

#endif
