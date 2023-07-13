#ifndef OBJECT_H
#define OBJECT_H
#include"ZJYmath.h"
#include<iostream>
extern int WAVE;
class object
{
	protected:
		int x,y;
		bool alive;
	public:
		int getx()
		{
			return x;
		}
		int gety()
		{
			return y;
		}
		bool getalive()
		{
			return alive;
		}
		void changeAlive()
		{
			if(alive==1)
			{
				alive=0;				
			}
			else
			{
				alive=1;
			}
		}
		void live()
		{
			alive=1;
		}
		void die()
		{
			alive=0;
		}
		void iptXY(int dx,int dy)
		{
			x=dx;
			y=dy;
		}
		
};
class hero:public object
{
	protected:
		int hitpoint;
	public:
		hero()
		{
			hitpoint=3;
			alive=1;
			x=400;
			y=400;
		}
		void changeX(int d)
		{
			x+=d;
		}
		void changeY(int d)
		{
			y+=d;
		}
		void changeHitpoint(int d)
		{
			hitpoint+=d;
		}
		int getHitpoint()
		{
			return hitpoint;
		}
		
};


class item:public object
{
	protected:
		int type;
	public:
		item(int t,int ix,int iy)
		{
			type=t;
			alive=0;
			x=ix;
			y=iy;
		}
		void changeX(int d)
		{
			x=d;
		}
		void changeY(int d)
		{
			y=d;
		}
		void dead()
		{
			alive=0;
		}
		int gettype()
		{
			return type;
		}
};


class dangan:public object
{
	protected:
		double vx,vy;
		int lifetime;
		int shootSpeed;
	public:
		dangan()
		{
			alive=0;
			lifetime=300;
			shootSpeed=15;
		}
		void live()
		{
			alive=1;
		}
		void endShoot(hero*p)
		{
			x=p->getx();
			y=p->gety();				
		
		}
		void died()
		{
			alive=0;
		}
		void speedup()
		{
			shootSpeed+=8;
		}
		void startShoot(hero*p)
		{
			x=p->getx();
			y=p->gety();
			alive=1;
		}
		void changeX(int d)
		{
			x+=d;
		}
		void changeY(int d)
		{
			y+=d;
		}
		void Vector(double x,double y)
		{
			vx=x*shootSpeed;
			vy=y*shootSpeed;
		}
		double getvx()
		{
			return vx;
		}
		double getvy()
		{
			return vy;
		}
};


class enemy:public object
{
	protected:
		int hitpoint;
		int speed;
		double destx,desty;
		int type;
	public:
		enemy()
		{
			hitpoint=10;
			alive=0;
			x=300;
			y=300;
			speed=2;
		}
		bool birth(int bx,int by,int i,int wave)//½©Ê¬Éú³É// 
		{
			if((wave>5)&&(i%15==0)&&(i!=0))
			{
				hitpoint=50;
				x=bx;
				y=by;
				alive=1;
				speed=2;
				type=1;
				return 1;
			}
			hitpoint=10+(wave/2);
			x=bx;
			y=by;
			alive=1;
			speed+=(i%3)+((wave/3)%5);
			type=0;
			return 0;
			
		}
		void changex(double d)
		{
			x+=d*speed+1;
		}
		void changey(double d)
		{
			y+=d*speed+1;
		}
		int gettype()
		{
			return type;
		}
		void dest(int x,int y)
		{
			destx=x;
			desty=y;
		}
		int gethp()
		{
			return hitpoint;
		}
		bool get_attacked(int d)
		{
			hitpoint-=d;
			if(hitpoint<=0)
			{
				alive=0;
				hitpoint=10;
				return 1;
			}
			else
			{
				return 0;
			}

		}
		void die()
		{
			alive=0;
		}
		void speedTo1()
		{
			speed=2;

		}
};



#endif
