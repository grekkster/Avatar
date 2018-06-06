//---------------------------------------------------------------------
// Ukazkovy priklad cislo 67
// Autor: Pavel Tisnovsky
//
// Program pro zobrazeni sceny s otexturovanymi telesy. Textury jsou nejdrive
// nacteny z bitmapovych souboru a z techto jsou pote zobrazovany ve scene.
// Ovladani bud mysi: otaceni+tlacitka pohyb vpred (leve) a vzad (prave)
// nebo klavesnici:
// sipka nahoru, sipka dolu: pohyb vpred/vzad
// sipka doprava, sipka doleva: otaceni
// CTRL+sipka doprava, doleva: ukroky
//---------------------------------------------------------------------
#pragma comment(lib, "glew32.lib")

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream> //kuli konverzi int to str
#include <time.h>

//#include <gl/glut.h>
#include "glew.h"     //GLEW knihovna
#include "wglew.h"    //GLEW speciality pro Windows
#include "glut.h"
#include "ac3d.h"     //nacitani AC3D modelu, export z Blenderu


#ifdef __BORLANDC__
#pragma hdrstop
#endif

#define MOVE_SCENE_DOWN         -5.0                // posun sceny dolu, aby byla videt rovina x-y
#define SCENE_SHIFT            0.0                // odsunuti sceny od pozorovatele, aby se provadelo korektne otaceni


#define DEFAULT_WINDOW_WIDTH    450                 // velikost a pocatecni pozice okna na obrazovce
#define DEFAULT_WINDOW_HEIGHT   450
#define DEFAULT_WINDOW_TOP      10
#define DEFAULT_WINDOW_LEFT     10

#define TEXTURE_GROUND           0                  // jmena textur pro vyber
#define TEXTURE_WALL1            1
#define TEXTURE_WALL2            2
#define TEXTURE_TREASURE         3
#define TEXTURE_VESMIR			 4


#define POSUN					 5
#define SPEED					 0.5
#define UHL_SPEED				 0.25
#define SENZITIVITA				 0.5

//GLuint CrossList;
GLuint textures[5];                                    // cisla textur
GLint display_list;

typedef struct  Window {                            // informace o oknu
    int         width;
    int         height;
} Window;

typedef struct  View {                              // informace o pohledu na scenu
    float       fov;
    float       nearPlane;
    float       farPlane;
} View;

typedef struct  Avatar {                            // informace o hraci (pozorovateli)
    float       posX;
    float       posY;
	float		posZ;
    float       angle;
	float       angley;
    float       moveSpeed;
	unsigned int	ammo;
	float ammoX;
	float ammoY;
	unsigned int	vypaleno;
} Avatar;

struct Plazma {
	GLUquadric *quadric;
	float posX;
	float posY;
	float posZ;
	float angle;
	float angley;
	bool visible;
} plazma[99];
/*
typedef struct Plazma {
	GLUquadric *quadric;
	float posX;
	float posY;
	float angle;
	bool visible;
} Plazma;

Plazma plazma[99];
*/

GLUquadric *quadratic;

float radius=0.5;                                   // polomer telesa
int   slices=15;                                    // rozdeleni telesa na casti - "poledniky" a "rovnobezky"
int   stacks=15;

//klavesy
//int sipky[4];
bool sipky[4];
bool skok;
float mysY;
float vyska_skok;
float cas = 0.f;


Window   window;
View     view={45.0, 1.0, 1000.0};
//Avatar   avatar={0.0, 0.0, -5, 0.0, 0.0, 0.0, 10, -500 , -500, 0};
Avatar   avatar={0.0, 0.0, 0, 0.0, 0.0, 0.0, 10, -500 , -500, 0};
int      btn=0;
int  mapa[15][20];
bool konecLvl=false;

// parametry, ktere ovlivnuji osvetleni
GLfloat materialAmbient[]={0.8f, 0.8f, 0.8f, 0.8f}; // ambientni slozka barvy materialu -- vsesmerove rozptylene
GLfloat materialDiffuse[]={1.0f, 1.0f, 1.0f, 1.0f}; // difuzni slozka barvy materialu -- 1zdroj,smer, vsemi smery
GLfloat materialSpecular[]={1.0f, 1.0f, 1.0f, 1.0f};// barva odlesku -- zrcadlo
GLfloat materialShininess[]={50.0f};                // faktor odlesku

GLfloat materialAmbient2[]={0.5f, 0.5f, 0.5f, 0.5f}; // ambientni slozka barvy materialu -- vsesmerove rozptylene
GLfloat materialShininess2[]={10.0f};                // faktor odlesku

//GLfloat light_position[]={0.0f, -1.0f, 0.0f, 1.0f};  // pozice svetla: nula na konci->bodove svetlo
GLfloat light_color[]={1.0f, 1.0f, 1.0f};           // barva svetla
//GLfloat light_position[]={0.0f, 0.0f, -1.0f, 1.0f};
//GLfloat spotDir[] = { 0.0f, 0.0f, -1.0f }; //MENIT PODLE POHYBU !!! smer svetla
//GLfloat light_position[]={0.0f, 0.0f, -5.0f, 1.0f};
//GLfloat spotDir[] = { 0.0f, 0.0f, -5.0f }; //MENIT PODLE POHYBU !!! smer svetla
GLfloat light_position[]={0.0f, 0.0f, -1.0f, 1.0f};
GLfloat spotDir[] = { 0.0f, 0.0f, -1.0f }; //MENIT PODLE POHYBU !!! smer svetla

//JINY NASTAVENI MATERIALU + ONIT POUZITO
GLfloat spec[]={1.0, 1.0 ,1.0 ,1.0};      //sets specular highlight of balls
GLfloat posl[]={0,400,0,1};               //position of ligth source
GLfloat amb[]={0.2f, 0.2f, 0.2f ,1.0f};   //global ambient
GLfloat amb2[]={0.3f, 0.3f, 0.3f ,1.0f};  //ambient of lightsource


//---------------------------------------------------------------------
// Tato funkce vykresli retezec danym bitmapovym fontem
//---------------------------------------------------------------------
void printStringUsingGlutBitmapFont(char *string, void *font, int x, int y, float r, float g, float b)
{
	glDisable(GL_LIGHTING);                      // globalni povoleni stinovani
    glDisable(GL_LIGHT0);
    glColor3f(r, g, b);                 // nastaveni barvy vykreslovanych bitmap
    glRasterPos2i(x, y);                // nastaveni pozice pocatku bitmapy
    while (*string)                     // projit celym retezcem
        glutBitmapCharacter(font, *string++); // vykresleni jednoho znaku
}

//---------------------------------------------------------------------
// Generovani bludiste
//---------------------------------------------------------------------
void generMap(void)
{
	memset(mapa, 0, sizeof(mapa));	//nulování mapy

	srand ( time(NULL) ); 
	//nejdriv vsude 0 - zadna zed
	for (int i=0;i<15;i++)
	{
		int pocetsl=(rand()%8);
		for (int j=0;j<pocetsl;j++)
		{
			int pozice=(rand()%20);
			mapa[i][pozice]=1; //zed
		}
	}
	
			//for (int k,k<pocetsl,k++)
			//mapa[i,j]=rand() %2
	
	
	int end_i=(rand()%15);
	int end_j=(rand()%20);
	int start_i=(rand()%13)+1;
	int start_j=(rand()%18)+1;
	do {
	start_i=(rand()%13)+1;
	start_j=(rand()%18)+1;
	}while (start_i==end_i && start_j==end_j);
	mapa[end_i][end_j]=2;	//konec lvl
	mapa[start_i][start_j]=0;	//start avatara
	avatar.posX=150-start_i*20;
	avatar.posY=200-start_j*20;
	
}

//---------------------------------------------------------------------
// Posun pohledu podle pozice a orientace avatara
//---------------------------------------------------------------------
void avatarMoveView(Avatar *avatar)
{
	/* // PUVODNI
    glTranslatef(0.0, MOVE_SCENE_DOWN, 0.0);		// posun sceny dolu, aby byla videt rovina z=0
    glRotatef(90.0, 1.0, 0.0, 0.0);                 // otoceni sceny okolo osy X tak, aby osa Z smerovala nahoru
    glTranslatef(0.0, SCENE_SHIFT, 0.0);            // posun sceny od pozorovatele, aby se provadelo korektne otaceni
    glRotatef(avatar->angle, 0.0, 0.0, 1.0);        // otoceni pozorovatele
    glTranslatef(avatar->posX, avatar->posY, 0.0);  // posun pozorovatele !!osetreni kamery za zdi
*/
	//POKUS 1
    glRotatef(90.0, 1.0, 0.0, 0.0);                 // otoceni sceny okolo osy X tak, aby osa Z smerovala nahoru
	glRotatef(avatar->angley, -1.0, 0.0, 0.0);
    glTranslatef(0.0, SCENE_SHIFT, 0.0);            // posun sceny od pozorovatele, aby se provadelo korektne otaceni
    glRotatef(avatar->angle, 0.0, 0.0, 1.0);        // otoceni pozorovatele
	glTranslatef(0.0, 0.0, -MOVE_SCENE_DOWN);            // posun sceny od pozorovatele, aby se provadelo korektne otaceni
	glTranslatef(avatar->posX, avatar->posY, avatar->posZ);  // posun pozorovatele !!osetreni kamery za zdi


/*
	//POKUS 2
	glRotatef(90.0, 1.0, 0.0, 0.0);                 // otoceni sceny okolo osy X tak, aby osa Z smerovala nahoru
    glTranslatef(0.0, SCENE_SHIFT, 0.0);            // posun sceny od pozorovatele, aby se provadelo korektne otaceni
    glRotatef(avatar->angle, 0.0, 0.0, 1.0);        // otoceni pozorovatele
    float a = 3.14/180*avatar->angle;
	glRotatef(avatar->angley, cos(a), -sin(a), 0.0);        // natoèení podle osy, která je aktuální "zleva doprava" vùèi pozici avatara !!
    glTranslatef(0.0, 0.0, -MOVE_SCENE_DOWN);		// posun sceny dolu, aby byla videt rovina z=0
    glTranslatef(avatar->posX, avatar->posY, 0.0);  // posun pozorovatele !!osetreni kamery za zdi

*/
}



//---------------------------------------------------------------------
// Kontrola, zda se avatar nachazi uvnitr sceny s pripadnym omezenim pohybu
//---------------------------------------------------------------------
bool avatarCheckRanges(Avatar *avatar,float newX,float newY) //,float oldX, float oldY)
{	
	
	if (avatar)
	{
	//obvodove zdi
	/*
	if (avatar->posX>145.0) avatar->posX=145.0;
    if (avatar->posX<-145.0) avatar->posX=-145.0;
    if (avatar->posY>195.0) avatar->posY=195.0;
    if (avatar->posY<-195.0) avatar->posY=-195.0;
	*/
	if (newX>145.0) return false;
    if (newX<-145.0) return false;
    if (newY>195.0) return false;
    if (newY<-195.0) return false;
	
	}
	
	//vnitrni zdi
	//prepocet do mapy
	int toMapX=15-ceil(((newX+150)/300)*15);
	int toMapY=20-ceil(((newY+200)/400)*20);
	
	if (avatar)
	{		
	if ((mapa[toMapX][toMapY]==1) || (mapa[toMapX][toMapY]==2))
		return false;
	}
	return true;	
}

//---------------------------------------------------------------------
// Kontrola kolizi plazmy, konec lvl
//---------------------------------------------------------------------
//bool plazmaKolize(Plazma *plazma, float newX, float newY) //,float oldX, float oldY)
bool plazmaKolize(float newX, float newY, float newZ) //,float oldX, float oldY)
{	
	//obvodove zdi
	/*
	if (plazma->posX>150.0) plazma->visible = false;
    if (plazma->posX<-150.0) plazma->visible=false;
    if (plazma->posY>200.0) plazma->visible=false;
    if (plazma->posY<-200.0) plazma->visible=false;
	*/
	
	if (newX>150.0) return false;
    if (newX<-150.0) return false;
    if (newY>200.0) return false;
    if (newY<-200.0) return false;
	
	if (newZ < -6) return false;
	if (newZ > 1000) return false;
	
	
	//vnitrni zdi
	//prepocet do mapy
	int toMapX=15-ceil(((newX+150)/300)*15);
	int toMapY=20-ceil(((newY+200)/400)*20);
			
	if (mapa[toMapX][toMapY]==1)
	{
		if (newZ>-1 && newZ<10)
			return false;
	}
	if (mapa[toMapX][toMapY]==2)
	{

		if (newZ>-1 && newZ<10)
		{
			konecLvl=true;
		return false;
		}
	}
	
	return true;
	
	
}
//---------------------------------------------------------------------
// Posun avatara ve smeru pohybu
//---------------------------------------------------------------------
void avatarMove(Avatar *avatar)
{
	float newX=avatar->posX + (SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY + (SPEED+POSUN)*cos(avatar->angle*3.14/180.0);

	////jiny posun pro kolize v ulhopricce x ZASTAVUJE KLOUZAVY POHYB !
	/*
	if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
	{
		if (avatarCheckRanges(avatar,newX,newY))
		{
			avatar->posX+=SPEED*sin(avatar->angle*3.14/180.0);
			avatar->posY+=SPEED*cos(avatar->angle*3.14/180.0);
		}
	}
	else
	*/

	//kontrola kolize pro oba smery souèasnì -- ošetøí rohy x NEFUNGUJE KLOUZANI
	//if (avatarCheckRanges(avatar,newX,newY))

	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
	{
		avatar->posX+=SPEED*sin(avatar->angle*3.14/180.0);
		//avatar->posY+=avatar->moveSpeed*cos(avatar->angle*3.14/180.0);
	}

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
	{
		//avatar->posX+=avatar->moveSpeed*sin(avatar->angle*3.14/180.0);
		avatar->posY+=SPEED*cos(avatar->angle*3.14/180.0);
	}

	/*
    avatar->posX+=avatar->moveSpeed*sin(avatar->angle*3.14/180.0);
    avatar->posY+=avatar->moveSpeed*cos(avatar->angle*3.14/180.0);
	*/
	/*
	//ne avatar->posX+=... ???
	float newX = avatar->moveSpeed*sin(avatar->angle*3.14/180.0);
	float newY = avatar->moveSpeed*cos(avatar->angle*3.14/180.0);
	if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=newX;
		avatar->posY+=newY;
	}
	*/
}



//---------------------------------------------------------------------
// Posun avatara dopredu (podle jeho orientace)
//---------------------------------------------------------------------
void avatarMoveForward(Avatar *avatar)
{
	float newX=avatar->posX + (SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY + (SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
/*
	if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
	{
		if (avatarCheckRanges(avatar,newX,newY))
		{
			avatar->posX+=SPEED*sin(avatar->angle*3.14/180.0);
			avatar->posY+=SPEED*cos(avatar->angle*3.14/180.0);
		}
	}
	else
	*/
	
	/*
	if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=1.0*sin(avatar->angle*3.14/180.0);
		avatar->posY+=1.0*cos(avatar->angle*3.14/180.0);
	}*/

	//kontrola kolize pro oba smery souèasnì -- ošetøí rohy x NEFUNGUJE KLOUZANI
	//if (avatarCheckRanges(avatar,newX,newY))
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=SPEED*sin(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY+=SPEED*cos(avatar->angle*3.14/180.0);
	/*
    avatar->posX+=1.0*sin(avatar->angle*3.14/180.0);
    avatar->posY+=1.0*cos(avatar->angle*3.14/180.0);
	*/
	/*
	float newX = 1.0*sin(avatar->angle*3.14/180.0);
	float newY = 1.0*cos(avatar->angle*3.14/180.0);
    if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=newX;
		avatar->posY+=newY;
	}*/
}

//---------------------------------------------------------------------
// Posun avatara dozadu (podle jeho orientace)
//---------------------------------------------------------------------
void avatarMoveBackward(Avatar *avatar)
{
	float newX=avatar->posX - (SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY - (SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
/*
	if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
	{
		if (avatarCheckRanges(avatar,newX,newY))
		{
			avatar->posX-=SPEED*sin(avatar->angle*3.14/180.0);
			avatar->posY-=SPEED*cos(avatar->angle*3.14/180.0);
		}
	}
	else
	*/
	
	/*
	if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX-=1.0*sin(avatar->angle*3.14/180.0);
		avatar->posY-=1.0*cos(avatar->angle*3.14/180.0);
	}*/
	//kontrola kolize pro oba smery souèasnì -- ošetøí rohy x NEFUNGUJE KLOUZANI
	//if (avatarCheckRanges(avatar,newX,newY))
	
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX-=SPEED*sin(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY-=SPEED*cos(avatar->angle*3.14/180.0);
	/*
    avatar->posX-=1.0*sin(avatar->angle*3.14/180.0);
    avatar->posY-=1.0*cos(avatar->angle*3.14/180.0);
	*/
	/*
	float newX =-1.0*sin(avatar->angle*3.14/180.0);
	float newY =-1.0*cos(avatar->angle*3.14/180.0);
    if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=newX;
		avatar->posY+=newY;

	}
	*/
}

//---------------------------------------------------------------------
// Posun avatara doleva (ukrok podle jeho orientace)
//---------------------------------------------------------------------
void avatarMoveLeft(Avatar *avatar)
{
	float newX=avatar->posX + (SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	float newY=avatar->posY - (SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	/*
	if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
	{
		if (avatarCheckRanges(avatar,newX,newY))
		{
			avatar->posX+=SPEED*cos(avatar->angle*3.14/180.0);
			avatar->posY-=SPEED*sin(avatar->angle*3.14/180.0);
		}
	}
	else
	*/

	/*
	if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=1.0*cos(avatar->angle*3.14/180.0);
		avatar->posY-=1.0*sin(avatar->angle*3.14/180.0);
	}*/
	//kontrola kolize pro oba smery souèasnì -- ošetøí rohy x NEFUNGUJE KLOUZANI
	//if (avatarCheckRanges(avatar,newX,newY))

	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=SPEED*cos(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY-=SPEED*sin(avatar->angle*3.14/180.0);
	/*
    avatar->posX+=1.0*cos(avatar->angle*3.14/180.0);
    avatar->posY-=1.0*sin(avatar->angle*3.14/180.0);
	*/
	/*
	float newX =1.0*cos(avatar->angle*3.14/180.0);
	float newY =-1.0*sin(avatar->angle*3.14/180.0);
    if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=newX;
		avatar->posY+=newY;
	}
	*/
}



//---------------------------------------------------------------------
// Posun avatara doprava (ukrok podle jeho orientace)
//---------------------------------------------------------------------
void avatarMoveRight(Avatar *avatar)
{
	float newX=avatar->posX-(SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	float newY=avatar->posY+(SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
/*
	if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
	{
	if (avatarCheckRanges(avatar,newX,newY))
		{
			avatar->posX-=SPEED*cos(avatar->angle*3.14/180.0);
			avatar->posY+=SPEED*sin(avatar->angle*3.14/180.0);
		}
	}
	else
	*/
	/*
	if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX-=1.0*cos(avatar->angle*3.14/180.0);
		avatar->posY+=1.0*sin(avatar->angle*3.14/180.0);
	}*/
	//kontrola kolize pro oba smery souèasnì -- ošetøí rohy x NEFUNGUJE KLOUZANI
	//if (avatarCheckRanges(avatar,newX,newY))
	
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX-=SPEED*cos(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY+=SPEED*sin(avatar->angle*3.14/180.0);
	/*
    avatar->posX-=1.0*cos(avatar->angle*3.14/180.0);
    avatar->posY+=1.0*sin(avatar->angle*3.14/180.0);
	*/
	/*
	float newX =-1.0*cos(avatar->angle*3.14/180.0);
	float newY =1.0*sin(avatar->angle*3.14/180.0);
    if (avatarCheckRanges(avatar,newX,newY))
	{
		avatar->posX+=newX;
		avatar->posY+=newY;
	}*/
}



//---------------------------------------------------------------------
// Otoceni avatara smerem doleva
//---------------------------------------------------------------------
void avatarTurnLeft(Avatar *avatar)
{
    avatar->angle+=3.0;
}



//---------------------------------------------------------------------
// Otoceni avatara smerem doprava
//---------------------------------------------------------------------
void avatarTurnRight(Avatar *avatar)
{
    avatar->angle-=3.0;
}

//--------------------pokus - pohyb na v9c kl8ves soucasne-------------
//---------------------------------------------------------------------
// Nahoru +strafe doleva
//---------------------------------------------------------------------
void avatarForwardLeft(Avatar *avatar)
{
	float newX=avatar->posX + (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY + (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	/*if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
		if (avatarCheckRanges(avatar,newX,newY))
		{
	else oprava=0;*/

	//dopredu
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=UHL_SPEED*sin(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY+=UHL_SPEED*cos(avatar->angle*3.14/180.0);

	//doleva
	newX=avatar->posX + (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	newY=avatar->posY - (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=UHL_SPEED*cos(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY-=UHL_SPEED*sin(avatar->angle*3.14/180.0);

	
}
//---------------------------------------------------------------------
// Nahoru +strafe doprava
//---------------------------------------------------------------------
void avatarForwardRight(Avatar *avatar)
{
	float newX=avatar->posX + (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY + (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	/*if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
		if (avatarCheckRanges(avatar,newX,newY))
		{
	else oprava=0;*/

	//dopredu
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=UHL_SPEED*sin(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY+=UHL_SPEED*cos(avatar->angle*3.14/180.0);

	//doprava
    newX=avatar->posX-(UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	newY=avatar->posY+(UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX-=UHL_SPEED*cos(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY+=UHL_SPEED*sin(avatar->angle*3.14/180.0);
	
}
//---------------------------------------------------------------------
// Dolu +strafe doleva
//---------------------------------------------------------------------
void avatarBackwardLeft(Avatar *avatar)
{
	float newX=avatar->posX - (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY - (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	/*if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
		if (avatarCheckRanges(avatar,newX,newY))
		{
	else oprava=0;*/

	//dolu
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX-=UHL_SPEED*sin(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY-=UHL_SPEED*cos(avatar->angle*3.14/180.0);

	//doleva
	newX=avatar->posX + (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	newY=avatar->posY - (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY))
		avatar->posX+=UHL_SPEED*cos(avatar->angle*3.14/180.0);

	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY))
		avatar->posY-=UHL_SPEED*sin(avatar->angle*3.14/180.0);
	
}
//---------------------------------------------------------------------
// Dolu +strafe doprava
//---------------------------------------------------------------------
void avatarBackwardRight(Avatar *avatar)
{
	float newX=avatar->posX - (UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	float newY=avatar->posY - (UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	/*if (((abs(avatar->angle) > 30) && (abs(avatar->angle) < 60)) || ((abs(avatar->angle) > 120) && (abs(avatar->angle) < 150)))
		if (avatarCheckRanges(avatar,newX,newY))
		{
	else oprava=0;*/

	//dolu
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY) && avatarCheckRanges(avatar,newX,avatar->posY+5) && avatarCheckRanges(avatar,newX,avatar->posY-5))
		avatar->posX-=UHL_SPEED*sin(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY) && avatarCheckRanges(avatar,avatar->posX+5,newY) && avatarCheckRanges(avatar,avatar->posX-5,newY))
		avatar->posY-=UHL_SPEED*cos(avatar->angle*3.14/180.0);

    //doprava
    newX=avatar->posX-(UHL_SPEED+POSUN)*cos(avatar->angle*3.14/180.0);
	newY=avatar->posY+(UHL_SPEED+POSUN)*sin(avatar->angle*3.14/180.0);
	//X
	if (avatarCheckRanges(avatar,newX,avatar->posY) && avatarCheckRanges(avatar,newX,avatar->posY+5) && avatarCheckRanges(avatar,newX,avatar->posY-5))
		avatar->posX-=UHL_SPEED*cos(avatar->angle*3.14/180.0);
	//Y
	if (avatarCheckRanges(avatar,avatar->posX,newY) && avatarCheckRanges(avatar,avatar->posX+5,newY) && avatarCheckRanges(avatar,avatar->posX-5,newY))
		avatar->posY+=UHL_SPEED*sin(avatar->angle*3.14/180.0);
	
}

//---------------------------------------------------------------------
// Nacteni bitmapy ze souboru typu BMP
//---------------------------------------------------------------------
int bitmapLoad(int texture, const char *filename)
{
    FILE          *fin;
    int           width, height, bpp=0;
    int           size;
    unsigned char *bitmap;
    unsigned char bmpHeader[54]={0x42, 0x4d,        // magicke cislo souboru BMP
                        0x00, 0x00, 0x00, 0x00,     // velikost souboru
                        0x00, 0x00, 0x00, 0x00,     // rezervovano, vzdy nastaveno na nula
                        0x36, 0x04, 0x00, 0x00,     // data offset=54
                        0x28, 0x00, 0x00, 0x00,     // velikost hlavicky=40
                        0x00, 0x00, 0x00, 0x00,     // sirka obrazku v pixelech=?
                        0x00, 0x00, 0x00, 0x00,     // vyska obrazku v pixelech=?
                        0x01, 0x00,                 // pocet bitovych rovin=1
                        0x08, 0x00,                 // pocet bitu na pixel=24
                        0x00, 0x00, 0x00, 0x00,     // metoda komprimace=nic
                        0x00, 0x00, 0x00, 0x00,     // velikost bitmapy
                        0x00, 0x00, 0x00, 0x00,     // pocet pixelu na metr v horizontalnim smeru
                        0x00, 0x00, 0x00, 0x00,     // pocet pixelu na metr ve vertikalnim smeru
                        0x00, 0x00, 0x00, 0x00,     // pocet pouzitych barev
                        0x00, 0x00, 0x00, 0x00,     // pocet dulezitych barev
    };
    if (!filename) return -1;
    fin=fopen(filename, "rb");
    if (!fin) return -1;                            // otevreni souboru se nezdarilo
    if (fread(bmpHeader, 54, 1, fin)!=1) return -1; // nacist hlavicku BMP souboru
    
	memcpy(&width, bmpHeader+18, 4);                // sirka obrazku v pixelech
    memcpy(&height, bmpHeader+22, 4);               // vyska obrazku v pixelech
    memcpy(&bpp, bmpHeader+28, 2);                  // pocet bitu na pixel
	
    if (bpp!=24) return -1;
    size=width*height*3;
    bitmap=(unsigned char *)malloc(size*sizeof(unsigned char));
    if (fread(bitmap, size, sizeof(unsigned char), fin)!=1) return -1;// nacteni rastrovych dat
    fclose(fin);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height,// nacteni textury do GPU
                 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
    free(bitmap);
    return 0;
}



//---------------------------------------------------------------------
// Nacteni a vytvoreni vsech textur
//---------------------------------------------------------------------
int loadTextures(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);          // zpusob ulozeni bytu v texure
    glGenTextures(5, textures);                     // vytvoreni jmena textur
    if (bitmapLoad(textures[TEXTURE_GROUND],   "ground.bmp"))   exit(0);
    if (bitmapLoad(textures[TEXTURE_WALL1],    "wall1.bmp"))    exit(0);
    if (bitmapLoad(textures[TEXTURE_WALL2],    "wall2.bmp"))    exit(0);
    if (bitmapLoad(textures[TEXTURE_TREASURE], "treasure.bmp")) exit(0);
	if (bitmapLoad(textures[TEXTURE_VESMIR], "vesmir.bmp")) exit(0);
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri inicializaci aplikace
//---------------------------------------------------------------------
void onInit(void)
{
		for (int i=0;i<99;i++)
	{
		//TYPEDEF
		/*
		plazma[i].quadric = NULL;
		plazma[i].visible=false;
		plazma[i].posX=0;
		plazma[i].posY=0;
		plazma[i].angle=0;
		*/
		//STRUCT, POVOLIT !

		gluDeleteQuadric(plazma[i].quadric);
		plazma[i].quadric = NULL;
		plazma[i].visible=false;
		
		plazma[i].posX=0;
		plazma[i].posY=0;
		plazma[i].posZ=0;
		plazma[i].angle=0;
		plazma[i].angley=0;
		
		
	}
	avatar.ammo=10;
	avatar.vypaleno=0;
	generMap();

	//POKUS //JINY NASTAVENI MATERIALU + ONIT POUZITO
	float df=100.0;

	//glClearDepth(1.0f);									// Depth Buffer Setup
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	//glClearColor(0,0,0,0);
 // 	glMatrixMode(GL_MODELVIEW);
 //   glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do

	//glMaterialfv(GL_FRONT,GL_SPECULAR,spec);
	//glMaterialfv(GL_FRONT,GL_SHININESS,&df);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
   
	glEnable(GL_BLEND);
 //   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//POKUS

//    glClearColor(0.2, 0.2, 0.4, 0.0);               // barva pro mazani color-bufferu
//    glShadeModel(GL_SMOOTH);                        // nastaveni stinovaciho rezimu
    loadTextures();									// nacist vsechny textury
//    glClearDepth(1.0f);                             // barva pro mazani z-bufferu
	glMatrixMode(GL_PROJECTION);                             // projekcni matice
    glLoadIdentity();                                           // Reset matice
    gluPerspective(view.fov,(double)window.width/(double)window.height, view.nearPlane, view.farPlane);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);                                    // Zapne mapování textur
    glShadeModel(GL_SMOOTH);                                    // nastaveni stinovaciho rezimu
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                       // barva pozadi obrazku
    glClearDepth(1.0f);                                         // barva pro mazani z-bufferu
    glEnable(GL_DEPTH_TEST);                                    // povoleni funkce pro testovani hodnot v pameti hloubky
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);         // vylepseni zobrazovani textur
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);                   // nastaveni vykresleni vyplnenych polygonu    
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
    //glEnable(GL_DEPTH_TEST);                        // nastaveni funkce pro testovani hodnot v z-bufferu
    //glDepthFunc(GL_LESS);                           // kterou funkci vybrat pro testovani z-bufferu
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);// vylepseni zobrazovani textur
    //glPointSize(10.0);                              // nastaveni velikosti vykreslovanych bodu
	glPushMatrix();
    glutSetCursor(GLUT_CURSOR_NONE);
	
	glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);    // nastaveni ambientni slozky barvy materialu
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);    // nastaveni difuzni slozky barvy materialu
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);  // nastaveni barvy odlesku
    glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);// nastaveni faktoru odlesku
	
	//pred zadanim svetla je treba korektne nastavit transf. matici MODELVIEW
	//glEnable(GL_NORMALIZE);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);      // nastaveni pozice svetla REFLEKTOR !!!
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 15);				//úhel
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);		//POZICE BY SE MELA MENIT !
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 40.0f);			//koncentrace svìtla
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);          // nastaveni barvy svetla

	//glLightfv(GL_LIGHT0, GL_AMBIENT, light_color);	//TODLE UZ NECO DELA !!!
    glEnable(GL_LIGHTING);                      // globalni povoleni stinovani
    glEnable(GL_LIGHT0);                        // povoleni svetla
	
	glPopMatrix();


}



//---------------------------------------------------------------------
// Callback funkce zavolana pri zmene velikosti okna aplikace
//---------------------------------------------------------------------
void onResize(int width, int height)
{
    glViewport(0, 0, width, height);                // viditelna oblast
    window.width=width;
    window.height=height;
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri zmene prekreslovani okna
//---------------------------------------------------------------------
void onDisplay(void)
{

    int i, j, k;
	glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// vymazani barvoveho a z-bufferu

    // Nastaveni ModelView matice tak, aby se pozorovatel prochazel
    // scenou ve stylu dungeonu
	
	glMatrixMode(GL_PROJECTION);                    // projekcni matice	
	glLoadIdentity();
	
	gluPerspective(view.fov,(double)window.width/(double)window.height, view.nearPlane, view.farPlane);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//text neni videt
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);                      // globalni povoleni stinovani
    glEnable(GL_LIGHT0);

    avatarMoveView(&avatar);	//OROTUJE SCENU -> SVETLO AZ PO ROTACI !!!!
	//glCallList(display_list);	//KULKA
	
	//glEnable(GL_NORMALIZE);

    // nakresleni texturovane podlahy
    glEnable(GL_TEXTURE_2D);                        // povoleni texturovani
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);	//OSVETLENI
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_GROUND]);   // navazani textury
    glBegin(GL_QUADS);
	glNormal3f(0,0,-1);
        glTexCoord2f(0.0, 0.0); glVertex3f(-150, -200, 1);
        glTexCoord2f(4.0, 0.0); glVertex3f(+150, -200, 1);
        glTexCoord2f(4.0, 4.0); glVertex3f(+150, +200, 1);
        glTexCoord2f(0.0, 4.0); glVertex3f(-150, +200, 1);
    glEnd();

    // nakresleni obvodovych zdi
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_WALL1]);
    glBegin(GL_QUADS);
		//OSVETLENI FUNGUJE JEN V ROZICH - VYPNUTO
		//glNormal3f(0,-1,0); //Z nic nenastaví!!!
		glTexCoord2f(0.0, 0.0); glVertex3f(-150,  200,  1);
        glTexCoord2f(9.0, 0.0); glVertex3f( 150,  200,  1);
        glTexCoord2f(9.0, 1.0); glVertex3f( 150,  200, -10);
        glTexCoord2f(0.0, 1.0); glVertex3f(-150,  200, -10);
		
        //glNormal3f( 1,0,0);
        glTexCoord2f(0.0, 0.0); glVertex3f(-150,  200,  1);
        glTexCoord2f(9.0, 0.0); glVertex3f(-150, -200,  1);
        glTexCoord2f(9.0, 1.0); glVertex3f(-150, -200, -10);
        glTexCoord2f(0.0, 1.0); glVertex3f(-150,  200, -10);

        //glNormal3f(0,1,0);
        glTexCoord2f(0.0, 0.0); glVertex3f(-150, -200,  1);
        glTexCoord2f(9.0, 0.0); glVertex3f( 150, -200,  1);
        glTexCoord2f(9.0, 1.0); glVertex3f( 150, -200, -10);
        glTexCoord2f(0.0, 1.0); glVertex3f(-150, -200, -10);

        //glNormal3f(-1,0,0);
        glTexCoord2f(0.0, 0.0); glVertex3f( 150,  200,  1);
        glTexCoord2f(9.0, 0.0); glVertex3f( 150, -200,  1);
        glTexCoord2f(9.0, 1.0); glVertex3f( 150, -200, -10);
        glTexCoord2f(0.0, 1.0); glVertex3f( 150,  200, -10);
		

    glEnd();

	//OKOLNI VESMIR
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_VESMIR]);
	//kouli
	quadratic=gluNewQuadric();// Vrátí ukazatel na nový kvadrik
	gluQuadricNormals(quadratic, GLU_SMOOTH);// Vygeneruje normálové vektory (hladké)
	gluQuadricTexture(quadratic, GL_TRUE);// Vygeneruje texturové koordináty

	gluSphere(quadratic,500.0f,32,32);// Koule

    // nakresleni vnitrnich zdi a pokladu
    for (i=0; i<15; i++) {
        for (j=0; j<20; j++) {
            if (mapa[i][j]!=0) {                  // vykreslit zed nebo poklad
                if (mapa[i][j]==1)
                    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_WALL2]);
                else
                    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_TREASURE]);

                glBegin(GL_QUADS);
					
					glNormal3f(0,-1,0);
                    glTexCoord2f(0.0, 0.0); glVertex3f( -150+i*20,  -200+j*20,  1);
                    glTexCoord2f(1.0, 0.0); glVertex3f( -130+i*20,  -200+j*20,  1);
                    glTexCoord2f(1.0, 1.0); glVertex3f( -130+i*20,  -200+j*20, -10);
                    glTexCoord2f(0.0, 1.0); glVertex3f( -150+i*20,  -200+j*20, -10);
					
					glNormal3f(0,1,0);
                    glTexCoord2f(0.0, 0.0); glVertex3f( -150+i*20,  -180+j*20,  1);
                    glTexCoord2f(1.0, 0.0); glVertex3f( -130+i*20,  -180+j*20,  1);
                    glTexCoord2f(1.0, 1.0); glVertex3f( -130+i*20,  -180+j*20, -10);
                    glTexCoord2f(0.0, 1.0); glVertex3f( -150+i*20,  -180+j*20, -10);
					
					glNormal3f(1,0,0);
                    glTexCoord2f(0.0, 0.0); glVertex3f( -130+i*20,  -200+j*20,  1);
                    glTexCoord2f(1.0, 0.0); glVertex3f( -130+i*20,  -180+j*20,  1);
                    glTexCoord2f(1.0, 1.0); glVertex3f( -130+i*20,  -180+j*20, -10);
                    glTexCoord2f(0.0, 1.0); glVertex3f( -130+i*20,  -200+j*20, -10);
					
					glNormal3f(-1,0,0);
                    glTexCoord2f(0.0, 0.0); glVertex3f( -150+i*20,  -200+j*20,  1);
                    glTexCoord2f(1.0, 0.0); glVertex3f( -150+i*20,  -180+j*20,  1);
                    glTexCoord2f(1.0, 1.0); glVertex3f( -150+i*20,  -180+j*20, -10);
                    glTexCoord2f(0.0, 1.0); glVertex3f( -150+i*20,  -200+j*20, -10);
					
                glEnd();
				
            }
        }
    }
	glDisable(GL_TEXTURE_2D);

	for (int i=0;i<avatar.vypaleno;i++)
	{
		 //TYPEDEF
		/*
		if (plazma[i].visible==true)
		{
			if (plazmaKolize(plazma,plazma[i].posX,plazma[i].posY ))
			{
				glPushMatrix();
				glTranslatef(-plazma[i].posX,-plazma[i].posY, -5);
				glColor4f(0.0,1.0,0.5,0.5);
				gluSphere(plazma[i].quadric,radius,slices,stacks);
				glPopMatrix();
			}
		
		else
        {
			plazma[i].quadric = NULL;
			plazma[i].visible = false;
            plazma[i].posX = 0;
            plazma[i].posY = 0;
            plazma[i].angle = 0;
        }
		}
		*/
		 //STRUCT
		if (plazma[i].visible==true)
		{
			if (plazmaKolize(plazma[i].posX,plazma[i].posY,plazma[i].posZ ))
			{
				
				glPushMatrix();
				glTranslatef(-plazma[i].posX,-plazma[i].posY, -plazma[i].posZ+avatar.posZ);
				glColor4f(0.0,1.0,0.8,0.5);
/*
				glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess2);// nastaveni faktoru odlesku
				glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient2);
*/
				gluSphere(plazma[i].quadric,radius,slices,stacks);
/*
				glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);// nastaveni faktoru odlesku
				glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
*/
				glPopMatrix();
			}
			
			else
			{
				gluDeleteQuadric(plazma[i].quadric);
				plazma[i].quadric = NULL;
				plazma[i].visible = false;
				
			}
			
        }
	}


	//glEnable(GL_TEXTURE_2D);
	//glDisable(GL_DEPTH_TEST); //?
	glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
	//pokus
	glMatrixMode(GL_PROJECTION);    // Vybere projekèní matici
    glPushMatrix();                 // Uloží projekèní matici
    glLoadIdentity();               // Reset matice

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	glOrtho(0, window.width, 0, window.height, -250, 250); //posunout víc nakocnec, mozna prasi svetla !!!!
	//pokus
	glMatrixMode(GL_MODELVIEW);     // Výbìr matice
    glPushMatrix();                 // Uložení matice
    glLoadIdentity();               // Reset matice

	//SEM PRIDAT CELNI LAMPU???
	//glDisable(GL_LIGHTING);                      // globalni povoleni stinovani
   // glDisable(GL_LIGHT0);

	//zamerovac
	glColor3f(1.0f, 1.0f, 1.0f);
	glLineWidth(1.0f);
    glBegin(GL_LINES);                          // nyni zacneme vykreslovat usecky
		glVertex2i(window.width/2-5, window.height/2);
        glVertex2i(window.width/2+5, window.height/2);
        glVertex2i(window.width/2, window.height/2+5);
        glVertex2i(window.width/2, window.height/2-5);
    glEnd();
	
	char buffer [33];
	itoa (avatar.ammo,buffer,10);
	printStringUsingGlutBitmapFont("ammo:",GLUT_BITMAP_HELVETICA_18, 20, 20 ,1.0,1.0,1.0); //vykresleni naboju
	printStringUsingGlutBitmapFont(buffer,GLUT_BITMAP_HELVETICA_18, 80, 20 ,1.0,1.0,1.0); //vykresleni naboju
	if (avatar.ammo ==0)
		printStringUsingGlutBitmapFont("reload - R",GLUT_BITMAP_HELVETICA_18, 100, 20 ,1.0,1.0,1.0); //vykresleni naboju
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();



	//printf("x %f y %f z %f alfa %f" , light_position);
	//printf("x %f y %f z %f" , spotDir);
/*
	printf("avatar x: %f, y: %f\n", avatar.posX, avatar.posY);
	printf("Lposition x: %f, y: %f, z: %f\n", light_position[0], light_position[1], light_position[2]);
	printf("Ldirection x: %f, y: %f, z: %f\n", spotDir[0],spotDir[1],spotDir[2]);
	//printf("angle : %f",avatar.angle);*/

	if (konecLvl)
	{
		konecLvl=false;
		onInit();
		//generMap();
		//GENERUJE DO HOTOVYHO BLUDISTE..NULOVAT!
	}

	glDisable(GL_TEXTURE_2D);
    glFlush();                                      // provedeni vsech prikazu
    glutSwapBuffers();                              // a prohozeni bufferu
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri stlaceni ASCII klavesy
//---------------------------------------------------------------------
void onKeyPress(unsigned char key, int x, int y)
{
    if (key>='A' && key<='Z')                       // uprava velkych pismen na mala
        key+='a'-'A';

    switch (key) {                                  // rozeskok podle stlacene klavesy
        case 27:                                    // klavesa Escape
        case 'q':
        case 'x':
            exit(0);                                // ukonceni programu
            break;
        case 'f':
            glutFullScreen();                       // prepnuti na celou obrazovku
            break;
		case 'r':
			avatar.ammo=10;                       // prepnuti na celou obrazovku
            break;
        case 'w':                                   // prepnuti do okna
            glutReshapeWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
            glutPositionWindow(DEFAULT_WINDOW_LEFT, DEFAULT_WINDOW_TOP);
            break;
		/*
		case ' ':
			skok=true;
			break;
			*/
        default:
            break;
    }
    glutPostRedisplay();
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri stlaceni non-ASCII klavesy
//---------------------------------------------------------------------
void onKeyDown(int key, int x, int y)
{
    int modifiers=glutGetModifiers();               // ziskat stav klaves ALT, CTRL a SHIFT

    switch (key) {
        case GLUT_KEY_UP:
			sipky[0]=true;
			//sipky[0]=1;
			//avatarMoveForward(&avatar);
			break;
        case GLUT_KEY_DOWN:
			sipky[1]=true;
			//sipky[1]=1;
            //avatarMoveBackward(&avatar);
            break;
        case GLUT_KEY_LEFT:
            if (modifiers & GLUT_ACTIVE_CTRL)       // CTRL+sipka je turn
				avatarTurnLeft(&avatar);
            else
				sipky[2]=true;
				//sipky[2]=1;
                //avatarMoveLeft(&avatar);
            break;
        case GLUT_KEY_RIGHT:
            if (modifiers & GLUT_ACTIVE_CTRL)       // CTRL+sipka je turn
                avatarTurnRight(&avatar);
            else
				sipky[3]=true;
				//sipky[3]=1;
				//avatarMoveRight(&avatar);  
            break;
        default: break;			
    }

    glutPostRedisplay();
}

void onKeyUp(int key, int x, int y)
{
    int modifiers=glutGetModifiers();               // ziskat stav klaves ALT, CTRL a SHIFT

    switch (key) {
        case GLUT_KEY_UP:
			sipky[0]=false;
			break;
        case GLUT_KEY_DOWN:
			sipky[1]=false;
            break;
        case GLUT_KEY_LEFT:
            if (modifiers & GLUT_ACTIVE_CTRL)       // CTRL+sipka je turn
				avatarTurnLeft(&avatar);
            else
				sipky[2]=false;
            break;
        case GLUT_KEY_RIGHT:
            if (modifiers & GLUT_ACTIVE_CTRL)       // CTRL+sipka je turn
                avatarTurnRight(&avatar);
            else
				sipky[3]=false;
            break;
        default: break;			
    }
    glutPostRedisplay();
}

//---------------------------------------------------------------------
// Callback funkce zavolana pri stlaceni nebo pusteni tlacitka mysi
//---------------------------------------------------------------------
void onMouseButton(int button, int state, int x, int y)
{
    if (button==GLUT_LEFT_BUTTON) {
        if (state==GLUT_DOWN) {
            btn=1;	// zrychleni dopredu
			
			if (avatar.ammo>0) {
			avatar.ammo=avatar.ammo-1; //pri prepisovani nutno volat glutPostRedisplay();
			//STRUCT, POVOLIT !
			plazma[avatar.vypaleno].quadric = gluNewQuadric();	 // vytvoreni kvadriky

			//TYPEDEF
			//plazma[avatar.vypaleno].quadric = gluNewQuadric();	 // vytvoreni kvadriky

			//quadric=gluNewQuadric();                        // vytvoreni kvadriky
			//gluQuadricDrawStyle(plazma[avatar.vypaleno].quadric, GLU_LINE);         // nastaveni vlastnosti kvadriky

			//STRUCT, POVOLIT !
			gluQuadricNormals(plazma[avatar.vypaleno].quadric, GLU_SMOOTH);         // smer generovanych normal

			//TYPEDEF
			//gluQuadricNormals(plazma[avatar.vypaleno].quadric, GLU_SMOOTH);         // smer generovanych normal

			//gluSphere(plazma[avatar.vypaleno].quadric, (GLdouble)radius, (GLint)slices, (GLint)stacks);
			//gluDeleteQuadric(quadric);                      // zruseni kvadriky

			//glTranslatef(plazma[i].posX,plazma[i].posY, -5);
			//glColor4f(0.0,1.0,0.5,0.5);
			//gluSphere(plazma[avatar.vypaleno].quadric,radius,slices,stacks);	//NEMUSI BEJT ?

			//STRUCT, POVOLIT !
			
            plazma[avatar.vypaleno].visible = true;
            plazma[avatar.vypaleno].posX = avatar.posX;
            plazma[avatar.vypaleno].posY = avatar.posY;
			plazma[avatar.vypaleno].posZ = avatar.posZ+5.f;
            plazma[avatar.vypaleno].angle = avatar.angle;
			plazma[avatar.vypaleno].angley = avatar.angley;
			avatar.vypaleno++;
			
			
			//TYPEDEF
			/*
			plazma[avatar.vypaleno].visible = true;
            plazma[avatar.vypaleno].posX = avatar.posX;
            plazma[avatar.vypaleno].posY = avatar.posY;
            plazma[avatar.vypaleno].angle = avatar.angle;
			avatar.vypaleno++;
			*/

			}

        }
        else {
            btn=0;
        }
    }
    if (button==GLUT_RIGHT_BUTTON) {
        if (state==GLUT_DOWN) {
            //btn=2;                                  // zrychleni dozadu
			skok=true;
        }
        else {
           // btn=0;
			skok=false;
        }
    }
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri pohybu mysi
//---------------------------------------------------------------------
void onMousePassiveMotion(int x, int y)
{
	//X
    static int first=1;
    static int old_x;
    if (first) {
        old_x=x;
        first=0;
    }
    else {
        avatar.angle=(-x+old_x)*SENZITIVITA;
    }
	//posun myši - nekoneèná rotace
	
	if (x > 375/SENZITIVITA)
       glutWarpPointer(15/SENZITIVITA,y);
   else if (x < 15/SENZITIVITA)
       glutWarpPointer(375/SENZITIVITA,y);

	//Y
	static int first_y=1;
    static int old_y;
	float newAngley;
	float oldAngleY=avatar.angley;
	//float oldY=y;

    if (first_y) {
        old_y=y;
        first_y=0;		
    }
    else {
		newAngley = avatar.angley +(-y+old_y)*SENZITIVITA;
		if ((newAngley > -90) && (newAngley < 90))
		{
			avatar.angley=newAngley;

		}
		//else
		//{
		//	if (newAngley>80)
		//		avatar.angley=89;//oldAngleY;
		//	if (newAngley<-80)
		//		avatar.angley=-89;
		//	//glutWarpPointer(x,89);

		//}
		old_y=y;
    }
	//posun myši - nekoneèná rotace
	/*
	if (y > 375*1/SENZITIVITA)
       glutWarpPointer(x,15*1/SENZITIVITA);
   else if (y < 15*1/SENZITIVITA)
	   glutWarpPointer(375*1/SENZITIVITA,x);
	*/
	printf("angleY : %f\n",avatar.angle);
	printf("oldAngleY : %f\n",oldAngleY);
/*	
	//POKUS 2
	   static int lastx;
	int diffx=x-lastx;              //check the difference between the current x and the last x position
	lastx = x;                      //set lastx to the current x position

   avatar.angle -= (float) diffx/2;   //set the anglez to zrot with the addition of the difference in the x position
   if (x > 735)
       glutWarpPointer(15,y);
   else if (x < 15)
       glutWarpPointer(735,y);

     static int lasty;
	int diffy=y-lasty;              //check the difference between the current x and the last x position
	lasty = y;                      //set lastx to the current x position

   avatar.angley += (float) diffy/2;   //set the anglez to zrot with the addition of the difference in the x position
   if (y > 735)
       glutWarpPointer(x,15);
   else if (y < 15)
       glutWarpPointer(x,735);
*/

    glutPostRedisplay();
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri tiku casovace kazdych 10ms
//---------------------------------------------------------------------
void onTimer(int timer)
{
	//PLYNULY POHYB - PRESUNUTO Z onKeyDown
	if ((sipky[0]) && (!sipky[1]) && (!sipky[2]) && (!sipky[3]))
		avatarMoveForward(&avatar);
	if ((!sipky[0]) && (sipky[1]) && (!sipky[2]) && (!sipky[3]))
		avatarMoveBackward(&avatar);
	if ((!sipky[0]) && (!sipky[1]) && (sipky[2]) && (!sipky[3]))
		avatarMoveLeft(&avatar);
	if ((!sipky[0]) && (!sipky[1]) && (!sipky[2]) && (sipky[3]))
		avatarMoveRight(&avatar);
	if ((sipky[0]) && (!sipky[1]) && (sipky[2]) && (!sipky[3]))
		avatarForwardLeft(&avatar);
	if ((sipky[0]) && (!sipky[1]) && (!sipky[2]) && (sipky[3]))
		avatarForwardRight(&avatar);
	if ((!sipky[0]) && (sipky[1]) && (sipky[2]) && (!sipky[3]))
		avatarBackwardLeft(&avatar);
	if ((!sipky[0]) && (sipky[1]) && (!sipky[2]) && (sipky[3]))
		avatarBackwardRight(&avatar);
	//PLYNULLY POHYB

	//skok
	static bool probiha_skok = false;
	static float cas,cas_vyskoku;
	if ( probiha_skok ) {
		cas += 0.05f;
		avatar.posZ = max( 0.f, 10.0*(cas-cas_vyskoku) - 0.5*9.81*(cas-cas_vyskoku)*(cas-cas_vyskoku) );
		//probiha_skok = avatar.posZ > 0.01f || cas <= cas_vyskoku+1;
		probiha_skok = avatar.posZ > 0.01f;
	}
	if ( skok && !probiha_skok )
	{
		probiha_skok = true;
		cas_vyskoku = cas = 0.f;
	}



	for (int i=0;i<avatar.vypaleno;i++)
	{
	//STRUCT, POVOLIT !
	
	if (plazma[i].visible == true)
        {
            plazma[i].posX +=  2*cos(plazma[i].angley*3.14/180.0)*sin(plazma[i].angle*3.14/180.0);
            plazma[i].posY += 2*cos(plazma[i].angley*3.14/180.0)*cos(plazma[i].angle*3.14/180.0);
			plazma[i].posZ += 2*sin(plazma[i].angley*3.14/180.0);

        }
		/*
		float pnewX=plazma[i].posX += sin(plazma[i].angle*3.14/180.0);
		float pnewY= plazma[i].posY += cos(plazma[i].angle*3.14/180.0);
		//TYPEDEF
		if (plazma[i].visible == true)
        {
			//CHYBA, NEJDE ZMAKNOUT S ITOU PLAZMOU
			//if (plazmaKolize(plazma,pnewX,pnewY)) { 
            plazma[i].posX += sin(plazma[i].angle*3.14/180.0);
            plazma[i].posY += cos(plazma[i].angle*3.14/180.0);
			//}
        }
		*/
	}
    //if (btn==1) avatar.moveSpeed=5.0;               // leve tlacitko mysi -> plna rychlost dopredu
    if (btn==2) avatar.moveSpeed=-5.0;              // prave tlacitko mysi -> plna rychlost dozadu
    if (btn==0) {                                   // zadne tlacitko mysi -> zpomaleni
        if (avatar.moveSpeed>0.1) avatar.moveSpeed-=0.5;
        if (avatar.moveSpeed<-0.1) avatar.moveSpeed+=0.5;
    }
	
    if (abs(avatar.moveSpeed)>0.01) {
        avatarMove(&avatar);
        glutPostRedisplay();
    }
    else {
        avatar.moveSpeed=0.0;
    }
	glutPostRedisplay();
    glutTimerFunc(10, onTimer, timer);
}



//---------------------------------------------------------------------
// Callback funkce zavolana pri tiku casovace kazdych 10ms
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
	//ACObject *ob; //pro nacteni AC3 modelu
    //static char *acFileName="model.ac"; //jmeno souboru s AC3 modelem 

    glutInit(&argc,argv);                           // inicializace knihovny GLUT
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH); // graficky mod okna
    glutInitWindowSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);// pocatecni velikost okna
    glutInitWindowPosition(DEFAULT_WINDOW_LEFT, DEFAULT_WINDOW_TOP);              // pocatecni pozice okna
    glutCreateWindow("Avatar by Panek&Kuk");           // vytvoreni okna pro kresleni
	//full screen rovnou, aby fungovalo jak má
	glutFullScreen();

	/*
	 if ( glewInit() != GLEW_OK )
    {
        printf("GLEW failed\n");
        return EXIT_FAILURE;
    }
    else
    {
        printf("GLEW ok\n");
    }
    //nyni je mozne primo pouzivat rozsirene funkce, datove typy apod.
	*/
    glutDisplayFunc(onDisplay);                     // registrace funkce volane pri prekreslovani
    glutReshapeFunc(onResize);                      // registrace funkce volane pri zmene velikosti
	glutIgnoreKeyRepeat(true);	//pokus kvuli plynulemu pohybu
    glutKeyboardFunc(onKeyPress);                   // registrace funkce volane pri stisku ASCII klavesy
    glutSpecialFunc(onKeyDown);                     // registrace funkce volane pri stisku non-ASCII klavesy
	glutSpecialUpFunc(onKeyUp);
    glutMouseFunc(onMouseButton);                   // registrace funkce volane pri stisku ci pusteni tlacitka mysi
    glutPassiveMotionFunc(onMousePassiveMotion);    // registrace funkce volane pri pohybu mysi
    glutTimerFunc(10, onTimer, 0x1234);             // registrace funkce volane pri tiku casovace
    onInit();                                       // inicializace aplikace
	/*
	ac_prepare_render();  //inicializace knihovny
    ob = ac_load_ac3d(acFileName);    //pokus o nacteni
    if (ob == NULL) //nenacetlo se
    {
        printf("NENACTENO\n");
        return EXIT_FAILURE;                                       
    }
    display_list = ac_display_list_render_object(ob); //vykresli do display listu
	*/
    glutMainLoop();                                 // nekonecna smycka, kde se volaji zaregistrovane funkce
    return 0;                                       // ANSI C potrebuje ukoncit fci main prikazem return
                                                    // i kdyz se sem program nikdy nedostane
}



//---------------------------------------------------------------------
// finito
//---------------------------------------------------------------------
