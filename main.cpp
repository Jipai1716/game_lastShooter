#include<graphics.h>
#include<easyx.h>
#include<conio.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include"object.h"
#include<iostream>
#define FPS 1000/60
#include<mmsystem.h>
#include<math.h>
#include"ZJYmath.h"
#include"item.h"
#include"enemyact.h"
#pragma	comment(lib,"winmm.lib")

using namespace std;
/////////////////////////////////////////////////
//TO DO
//优化射击手感 DONE
//加入背景音乐 
//加入回血道具 DONE
//加入拾取的手雷道具 DONE
//完善关卡难度配置 
//		---道具刷新 
//		---敌人刷新 
//（可选）加入激光瞄具 
//（可选）加入不同的敌人 
//////////////////////////////////////////////////
/////////////////变量声明/////////////////////////
//////////////////////////////////////////////////

clock_t lastBoomer=0,lastShoot=0;
int score; 
object Bm;
int N=4;
int accuracy=1;
int WAVE=0;
hero player;
dangan B;
int Ftime=0,GameSec=0;
bool mouseFlage=0,CDflag=1;
int herospeed=4,
	heroATK=3;
int CD;
IMAGE startImg,background,man,item_speedup,rm,bullet,item_ATKUP,item_ASPup,enemy_zoombies
	,heart,boomer,boom,CrossStar,slime;
int destX,destY;
ExMessage msg={0};
bool summonFLAG=0;
int summonflag;
enemy bazi;
int boom_number;
enemy zoombies[120];
//////////////////////////////////////////////////
int  judgeEdge();
void test();
void click();
void shoot();
void judgeEnemy_attacked();
void boomPIC();
/////////////////////////////////////////////////
 
void drawObject(int x,int y,IMAGE pic)//绘制实体函数 
{
	int xp=pic.getwidth()/2;
	int yp=pic.getheight()/2;
	putimage(x-xp,y-yp,&pic);
}
void inital_variable()//数据初始化 
{
	Ftime=0;
	GameSec=0;
	mouseFlage=0,CDflag=1;
	herospeed=4;
	heroATK=3;
	kill=0;
	CD=25;
	ItemFlag=1;
	summonFLAG=1;
	boom_number=3;
	accuracy=1;
	score=0;
}
void inital_image()//图像初始化加载 
{
	loadimage(&startImg,"source/image/startPage.png");
	loadimage(&background,"source/image/background.png");
	loadimage(&man,"source/image/hero.png");
	loadimage(&item_speedup,"source/image/item_speedUp.png");
	loadimage(&rm,"source/image/reimu.png");
	loadimage(&bullet,"source/image/bullet.png");
	loadimage(&item_ATKUP,"source/image/item_ATKUp.png");
	loadimage(&item_ASPup,"source/image/item_ASPup.png");
	loadimage(&heart,"source/image/heart.png");
	loadimage(&boom,"source/image/boom.png");
	loadimage(&boomer,"source/image/boomer.png");
	loadimage(&enemy_zoombies,"source/image/enemy.png");
	loadimage(&CrossStar,"source/image/crossstar.png");
	loadimage(&slime,"source/image/slime.png");
}
void draw_boomer()//绘制当前炸弹数量// 
{
	if(boom_number==3)
	{
		drawObject(10,60,boomer);
		drawObject(40,60,boomer);
		drawObject(70,60,boomer);	
	}
	if(boom_number==2)
	{
		drawObject(10,60,boomer);
		drawObject(40,60,boomer);
	}
	if(boom_number==1)
	{
		drawObject(10,60,boomer);
	}	
} 
void draw_hitpoint()//当前血量绘制 
{
	if(player.getHitpoint()==3)
	{
		drawObject(10,10,heart);
		drawObject(40,10,heart);
		drawObject(70,10,heart);
	}
	if(player.getHitpoint()==2)
	{
		drawObject(10,10,heart);
		drawObject(40,10,heart);
	}
	if(player.getHitpoint()==1)
	{
		drawObject(10,10,heart); 
	}
	boomPIC();
}
void inital_music()
{
	mciSendString("open source/sounds/shoot.mp3", 0, 0, 0);
}
void DrawGame()//绘制游戏画面 
{	
	BeginBatchDraw();
	
	putimage(0,0,&background);//背景 
	drawObject(player.getx(),player.gety(),man);//玩家 
	
	if(spdUP.getalive()==1)//绘制 加速道具
	{
		drawObject(spdUP.getx(),spdUP.gety(),item_speedup);
	}
	
	if(atkUP.getalive()==1)//绘制 ATK
	{
		drawObject(atkUP.getx(),atkUP.gety(),item_ATKUP);
	}
	if(aspUP.getalive()==1)//绘制 ASP
	{
		drawObject(aspUP.getx(),aspUP.gety(),item_ASPup);
	}
	if(boomerUP.getalive()==1)//绘制炸弹道具 
	{
		drawObject(boomerUP.getx(),boomerUP.gety(),boomer);
	}
	if(heartUP.getalive()==1)//红心道具 
	{
		drawObject(heartUP.getx(),heartUP.gety(),heart);
	}
	if(B.getalive()==1)//绘制子弹 
	{
		drawObject(B.getx(),B.gety(),bullet);
	}	
	//if(bazi.getalive())
	//{
	//	drawObject(bazi.getx(),bazi.gety(),rm);
	//}
	draw_hitpoint();//绘制当前生命值 
	for(int i=0;i<N;i++)//绘制敌人（僵尸） 
	{
		if(zoombies[i].getalive()&&(zoombies[i].gettype()==0))
		{
			drawObject(zoombies[i].getx(),zoombies[i].gety(),enemy_zoombies);
		}
		if(zoombies[i].getalive()&&(zoombies[i].gettype()==1))
		{
			drawObject(zoombies[i].getx(),zoombies[i].gety(),slime);
		}
	}
	draw_boomer();//绘制当前炸弹数量 
	drawObject(msg.x,msg.y,CrossStar);//绘制准星 
	FlushBatchDraw();
	
}
//int inv_clock;
//bool inv_flag=0;
//bool flag2=0;
//bool Invincible()
//{
//	if(Ftime-inv_clock>=100&&inv_flag==1)
//	{
//		inv_flag=0;
//		flag2=0;
//		printf("meile\n");//无敌失效 
//		return 0;
//	}
//	if(inv_flag==1&&flag2==0)
//	{
//		inv_clock=Ftime;
//		flag2=1;
//		printf("wudi\n"); 
//		return 1;//触发无敌 
//	}
//	if(inv_flag==1&&flag2==1)
//	{ 
//		printf("wudi\n");
//		return 1;//触发无敌 
//	}
//	else
//	{
//		return 0;
//	}
//}
bool boomFLAG=0;
bool boomFLAG2=0;
int boomsec;
void boomPIC()//绘制炸弹 
{
	if(boomFLAG==1&&boomFLAG2==0)
	{
		drawObject(Bm.getx(),Bm.gety(),boom);
		boomsec=Ftime;
		boomFLAG2=1;
	}
	if(boomFLAG==1&&boomFLAG2==1)
	{
		drawObject(Bm.getx(),Bm.gety(),boom);
		if(Ftime-boomsec==50)
		{
			boomFLAG=0;
			boomFLAG2=0;
		}
	}
	
}
template<class T1,class T2>
int crashBOOM(T1*p1,T2*p2)//炸弹碰撞判定 
{
	int dx=p1->getx()-p2->getx();
	int dy=p1->gety()-p2->gety();
	
	int d=dx*dx+dy*dy;
	if(d<=1600)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void Boom()//炸弹 
{
	if(boom_number>=1)
	{
		boom_number--;
		Bm.iptXY(msg.x,msg.y);
		boomFLAG=1;
		Bm.live();
		printf("(%d,%d)\n",msg.x,msg.y);
		for(int i=0;i<N;i++)
		{
			if(crashBOOM(&zoombies[i],&Bm)&&zoombies[i].getalive()&&Bm.getalive())
			{
				zoombies[i].get_attacked(15);
				printf("boom attacked");
			}
		}
		Bm.die();
	}
	else
	{
		
	}
}

int startPage()//开始页面
{
	
	putimage(0,0,&startImg);
	while(1)
	{
		if(GetAsyncKeyState(VK_SPACE))
		{
			return 1;	
		}
	}

}
void heroMove(hero *p)//主角移动
{

	if(GetAsyncKeyState(0x57)&&(judgeEdge()!=3))//0x57  w
	{
		p->changeY(-herospeed);
	}
	if(GetAsyncKeyState(0x53)&&(judgeEdge()!=4))
	{
		p->changeY(+herospeed);
	}
	if(GetAsyncKeyState(0x41)&&(judgeEdge()!=1))
	{
		p->changeX(-herospeed);
	}
	if(GetAsyncKeyState(0x44)&&(judgeEdge()!=2))
	{
		p->changeX(+herospeed);
	}
	if(GetAsyncKeyState(0x01)&&(clock()-lastBoomer>=1000))
	{
		lastBoomer=clock();
		Boom();
	}		 	
////////////////////////////////////

}


int judgeEdge()//空气墙 
{
	if(player.getx()<=10)//最左 
	{
		return 1;
	}
	else if(player.getx()>=790)//最右 
	{
		return 2;
	}
	else if(player.gety()>=790)//最下 
	{
		return 4;
	}
	
	else if(player.gety()<=10)//最上 
	{
		return 3;
	}
	else//
	{
		return 0;
	}
	
}
template<class T1,class T2>

int crash(T1*p1,T2*p2)//碰撞判定 
{
	int dx=p1->getx()-p2->getx();
	int dy=p1->gety()-p2->gety();
	
	int d=dx*dx+dy*dy;
	if(d<=400)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void judge_item(item *i,hero *p)//道具拾取判断 :
{
	if(crash(i,p)&&i->getalive())
	{
		switch(i->gettype())
		{	
			score+=20;//道具+20分 
			case 1:
				printf("item1\n");
				herospeed++;
				i->dead();
				break;
			case 2:
				printf("item2\n");
				heroATK++;
				i->dead();
				break;
			case 3:
				printf("item3\n");
				CD-=7;
				accuracy++;
				B.speedup();
				i->dead();
				break;
			case 4:
				if(player.getHitpoint()<3)
				{
					player.changeHitpoint(1);
				}
				i->dead();
				break;
			case 5:
				if(boom_number<3)
				{
					boom_number++;
				}
				i->dead();
				break;
		}

	} 
}
void gameover()
{
	exit(0);
}
void judgeHero_attacked()//角色受击 
{
	for(int i=0;i<N;i++)
	{
		if(crash(&zoombies[i],&player)&&zoombies[i].getalive())//zoombies
		{
			//inv_flag=1;
			zoombies[i].die();
			//if(!Invincible())
		//	{
				player.changeHitpoint(-1);
				printf("ohhhh!\n");
		//	}

			printf("ohhhh!\n");
			if(player.getHitpoint()<=0)
			{
				player.changeAlive();
				gameover();
			}
		}
	}
}
void judge()//总判断函数 
{
	judge_item(&spdUP,&player);
	judge_item(&atkUP,&player);
	judge_item(&aspUP,&player);
	judge_item(&heartUP,&player);
	judge_item(&boomerUP,&player);
	///////////////////////////
	judgeEnemy_attacked(); 
	//////////////////////////
	judgeHero_attacked(); 
}
void summon_item()//总生成道具函数 
{
	summonitem_spdup();
	summonitem_ATKup();
	summonitem_Aspup();
	summonitem_heartup();
	summonitem_boomerUP();
}
void click()//鼠标相关 
{
	if(peekmessage(&msg,EX_MOUSE))
	{

		if(1) 
		{	
			//mciSendString("play source/sounds/shoot.mp3", 0, 0, 0);
			////printf("鼠标点击坐标(%d,%d)",msg.x,msg.y);
		 	destX=msg.x;
		 	destY=msg.y;
		 	
			if(CDflag)
		 	{
				shoot();
				CDflag=0;		 		
			}

		}
		/////////test////////////
		if(msg.message==WM_RBUTTONDOWN)
		{
			kill++;
			ItemFlag=1;
			printf("KILL:%d\n",kill);
		}
	}
}

void shoot()//射击函数 
{
	double dx,dy;

	if(B.getalive()==0)
	{
		B.startShoot(&player);
		dx=destX-B.getx();
		dy=destY-B.gety();
		VectorUnitization(dx,dy);
		cout<<"shoot!"<<endl;
//		cout<<"子弹坐标 ("<<B.getx()<<","<<B.gety()<<")";
//		cout<<"主角坐标("<<player.getx()<<","<<player.gety()<<")"; 
//		cout<<"单位速度矢量("<<dx<<","<<dy<<")"<<endl;
		B.Vector(dx,dy);
		
	}
	
}

int cdtimer;
bool swh=0;
void shootCD()//射击冷却 
{
	if(CDflag==0&&swh==0)
	{
		cdtimer=Ftime;
		swh=1;
	}
	if(swh==1)
	{
		if(Ftime-cdtimer>=CD)
		{
			CDflag=1;
			B.died();
			swh=0;
		}
	}
}

void bullet_move()//子弹运动 
{
	if(B.getalive()) 
	{
		B.changeX((int) B.getvx());
		B.changeY((int) B.getvy());
	}
}
void enemymove(enemy *p,hero *m)//敌人移动 
{
	long double dx,dy,z;
	if(p->getalive())
	{
		dx=m->getx()-p->getx();
		dy=m->gety()-p->gety();
		z=dx*dx+dy*dy;
		z=sqrt(z);
		dx=dx*(1/z);
		dy=dy*(1/z); 
		p->changex(dx);
		p->changey(dy);
		
	}


}
void judgeEnemy_attacked()//敌人受击 
{
	for(int i=0;i<N;i++)
	{
		if(zoombies[i].getalive()&&crash(&zoombies[i],&B)&&B.getalive())
		{
		//	mciSendString("play source/sounds/shoot.mp3 repeat", 0, 0, 0);
			B.died();
			if(zoombies[i].get_attacked(heroATK))
			{
				score+=WAVE*5+20+i; 
				kill++;
				ItemFlag=1;
				//printf("kill:%d\n",kill);
			}
			//printf("hp:%d,atk:%d\n",zoombies[i].gethp(),heroATK);
			//printf("getattacked\n");
			B.endShoot(&player);		
		}
	}
	
}

void move()//总移动 
{
	heroMove(&player);
	for(int i=0;i<N;i++)
	{
		enemymove(&zoombies[i],&player);
	}
		bullet_move();
}


void summonEVENT()//生成敌人 
{
	for(int i=0;i<N;i++)
	{
		zoombies[i].speedTo1();
		int BX,BY;
		BX=ranX(i*i);
		BY=ranX(i*i);
		printf("wave:%d,enemy:%d,score:%d\n",WAVE,N,score);
		if(ranX(i*i)%2==0)//边缘生成 
		{
			if(ranX(i*i*i)%2==0)
			{
				BX=0;
			}
			else
			{
				BX=800;
			}
		}
		else
		{
			if(ranX(i*i*i)%2==0)
			{
				BY=0;
			}
			else
			{
				BY=800;
			}
		}
		zoombies[i].birth(BX,BY,i,WAVE);

	}
	
}
bool nozoombies()
{
	int S=0;
	for(int i=0;i<N;i++)
	{
		if(zoombies[i].getalive()==0)
		{
			S++;
		}
	}
	if(S==N)
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}

void summonEnemy()
{
	
	if(nozoombies()&&summonFLAG==1)
	{
		summonEVENT();////
		WAVE++;
		N+=WAVE/2;
		summonFLAG=0;
		summonflag=kill;
	}
	if(summonFLAG==0&&nozoombies())
	{
		summonFLAG=1;
	}
	
}

void endpage()
{
	system("pause");
}






void game()
{
	while(1)
	{
		int starttime=clock();
		int frametime=clock()-starttime;
		///////timer///////
		Ftime++;
		GameSec=Ftime/60;
		///////////////////
		peekmessage(&msg,EX_MOUSE);	
		summonEnemy();
		judge();
		move();
		click();
	 	shootCD();	
		summon_item();
		///
		DrawGame();
		if(FPS>frametime)
		{
			Sleep(FPS-frametime);
		}
		if(WAVE==12)
		{
			endpage();
		}
	}
}


int main()//
{
	
	initgraph(800,800);
	inital_image();
	inital_variable();
	if(startPage())
	{
		game();
	}
	
	getch();
	closegraph();
	EndBatchDraw();
	return 0;
}

