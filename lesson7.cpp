#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glut.h>
#include "tgaload.h"

#define MAX_NO_TEXTURES 3
#define CUBE_TEXTURE 0
#define FCUBE_TEXTURE 1
#define MCUBE_TEXTURE 2

//
GLuint texture_id[MAX_NO_TEXTURES];

float xrot;
float yrot;
float xspeed;			// X Rotation Speed
float yspeed;			// Y Rotation Speed
float ratio;
float	z=0.0f;			// Depth Into The Screen

GLfloat LightAmbient[]=		{ 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]=		{ 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[]=	{ 0.0f, 0.0f, 2.0f, 1.0f };

GLuint	filter;				// Which Filter To Use
bool	light;				// Lighting ON/OFF ( NEW )
bool	lp;					// L Pressed? ( NEW )
bool	fp;					// F Pressed? ( NEW )

// Storage For The Display List
GLuint	g_cube;


//Camera Info
typedef struct {
   double x,y,z;
} XYZ;

typedef struct {
   XYZ vp;              /* View position           */
   XYZ vd;              /* View direction vector   */
   XYZ vu;              /* View up direction       */
   XYZ pr;              /* Point to rotate about   */
   double focallength;  /* Focal Length along vd   */
   double aperture;     /* Camera aperture         */
   double eyesep;       /* Eye separation          */
   int screenwidth,screenheight;
} CAMERA;

//Use for adjusting the camera
void Normalise(XYZ *);
void RotateCamera(int,int,int);
void TranslateCamera(int,int,int);
void AdjustEyeSeparation(int);
void AdjustFocalLength(int);
void display( void );
void CameraHome(int);


//Methods used for content generation
void build_lists();
void DrawCube(void);

#define PI 3.141592653589793238462643
//Degrees to Radians
#define DTOR            0.0174532925
/*Flags*/
//int stereo = FALSE;
int stereo = TRUE;
int debug = TRUE;		//debug bit = use to print info
double dtheta = 1;		//delta theta - 1 degree
CAMERA camera;
XYZ origin = {0.0,0.0,0.0};			//Used to initialize camera.pr
int currentbutton = -1;				//Used to track mouse clicks
/*
   Move the camera to the home position 
*/
void CameraHome(int mode)
{

   camera.screenwidth = 400;
   camera.screenheight = 300;

   camera.aperture = 50;
   camera.focallength = 20;
   camera.eyesep = camera.focallength / 20;
   camera.pr = origin;

/*
   camera.vp.x = camera.focallength; 
   camera.vp.y = 0; 
   camera.vp.z = 0;
   camera.vd.x = -1;
   camera.vd.y = 0;
   camera.vd.z = 0;
*/
   /* Special camera position so the beam crosses the view */
   camera.vp.x = 0;
   camera.vp.y = 0;
   camera.vp.z = 20;
   camera.vd.x =0; 
   camera.vd.y = 0; 
   camera.vd.z = -10;

   camera.vu.x = 0;  
   camera.vu.y = 1; 
   camera.vu.z = 0;

   if (debug)
   {
	   fprintf(stderr,"Eye separation: (%g)\n", camera.eyesep);
	   fprintf(stderr,"Focal Length: (%g)\n", camera.focallength);
	   fprintf(stderr,"Focal Aperture (degrees): (%g)\n", camera.aperture);
	   fprintf(stderr,"Camera position: (%g,%g,%g)\n",
         camera.vp.x,camera.vp.y,camera.vp.z);
	   fprintf(stderr,"Point to Rotate About: (%g,%g,%g)\n",
         camera.pr.x,camera.pr.y,camera.pr.z);
	   fprintf(stderr,"Camera up vector: (%g,%g,%g)\n",
         camera.vu.x,camera.vu.y,camera.vu.z);
	   fprintf(stderr,"Camera direction vector: (%g,%g,%g)\n",
         camera.vd.x,camera.vd.y,camera.vd.z);

   }
}
void Normalise(XYZ *p)
{
   double length;

   length = sqrt(p->x * p->x + p->y * p->y + p->z * p->z);
   if (length != 0) {
      p->x /= length;
      p->y /= length;
      p->z /= length;
   } else {
      p->x = 0;
      p->y = 0;
      p->z = 0;
   }
}

#define CROSSPROD(p1,p2,p3) \
   p3.x = p1.y*p2.z - p1.z*p2.y; \
   p3.y = p1.z*p2.x - p1.x*p2.z; \
   p3.z = p1.x*p2.y - p1.y*p2.x


void AdjustEyeSeparation(int ix)
{
	camera.eyesep += ix*.1;

    if (debug)
      fprintf(stderr,"Eye separation: (%g)\n", camera.eyesep);
}

void AdjustFocalLength(int ix)
{
	camera.focallength += ix*.1;

    if (debug)
      fprintf(stderr,"Focal Length: (%g)\n", camera.focallength);
}
/*
   Rotate (ix,iy) or roll (iz) the camera about the focal point
   ix,iy,iz are flags, 0 do nothing, +- 1 rotates in opposite directions
   Correctly updating all camera attributes
*/
void RotateCamera(int ix,int iy,int iz)
{
   XYZ vp,vu,vd;
   XYZ right;
   XYZ newvp,newr;
   double radius,dd,radians;
   double dx,dy,dz;

   vu = camera.vu;
   Normalise(&vu);
   vp = camera.vp;
   vd = camera.vd;
   Normalise(&vd);
   CROSSPROD(vd,vu,right);
   Normalise(&right);
   radians = dtheta * PI / 180.0;

   /* Handle the roll */
   if (iz != 0) {
      camera.vu.x += iz * right.x * radians;
      camera.vu.y += iz * right.y * radians;
      camera.vu.z += iz * right.z * radians;
      Normalise(&camera.vu);
      return;
   }

   /* Distance from the rotate point */
   dx = camera.vp.x - camera.pr.x;
   dy = camera.vp.y - camera.pr.y;
   dz = camera.vp.z - camera.pr.z;
   radius = sqrt(dx*dx + dy*dy + dz*dz);

   /* Determine the new view point */
   dd = radius * radians;
   newvp.x = vp.x + dd * ix * right.x + dd * iy * vu.x - camera.pr.x;
   newvp.y = vp.y + dd * ix * right.y + dd * iy * vu.y - camera.pr.y;
   newvp.z = vp.z + dd * ix * right.z + dd * iy * vu.z - camera.pr.z;
   Normalise(&newvp);
   camera.vp.x = camera.pr.x + radius * newvp.x;
   camera.vp.y = camera.pr.y + radius * newvp.y;
   camera.vp.z = camera.pr.z + radius * newvp.z;

   /* Determine the new right vector */
   newr.x = camera.vp.x + right.x - camera.pr.x;
   newr.y = camera.vp.y + right.y - camera.pr.y;
   newr.z = camera.vp.z + right.z - camera.pr.z;
   Normalise(&newr);
   newr.x = camera.pr.x + radius * newr.x - camera.vp.x;
   newr.y = camera.pr.y + radius * newr.y - camera.vp.y;
   newr.z = camera.pr.z + radius * newr.z - camera.vp.z;

   camera.vd.x = camera.pr.x - camera.vp.x;
   camera.vd.y = camera.pr.y - camera.vp.y;
   camera.vd.z = camera.pr.z - camera.vp.z;
   Normalise(&camera.vd);

   /* Determine the new up vector */
   CROSSPROD(newr,camera.vd,camera.vu);
   Normalise(&camera.vu);

   if (debug)
      fprintf(stderr,"Camera position: (%g,%g,%g)\n",
         camera.vp.x,camera.vp.y,camera.vp.z);

}
/*
   Translate (pan) the camera view point
   In response to i,j,k,l keys
   Also move the camera rotate location in parallel

   TranslateCamera(0,1);		//Translate camera up
   TranslateCamera(0,-1);		//Translate camera down
   TranslateCamera(-1,0);		//Translate camera left
   TranslateCamera(1,0);		//Translate camera right
*/
void TranslateCamera(int ix,int iy, int iz)
{
   XYZ vp,vu,vd;
   XYZ right;
   XYZ newvp,newr;
   double radians,delta;

   vu = camera.vu;
   Normalise(&vu);
   vp = camera.vp;
   vd = camera.vd;
   Normalise(&vd);
   CROSSPROD(vd,vu,right);
   Normalise(&right);
   radians = dtheta * PI / 180.0;
   delta = dtheta * camera.focallength / 90.0;

   camera.vp.x += iy * vu.x * delta;
   camera.vp.y += iy * vu.y * delta;
   camera.vp.z += iy * vu.z * delta;
   camera.pr.x += iy * vu.x * delta;
   camera.pr.y += iy * vu.y * delta;
   camera.pr.z += iy * vu.z * delta;

   camera.vp.x += ix * right.x * delta;
   camera.vp.y += ix * right.y * delta;
   camera.vp.z += ix * right.z * delta;
   camera.pr.x += ix * right.x * delta;
   camera.pr.y += ix * right.y * delta;
   camera.pr.z += ix * right.z * delta;

   camera.vp.x += iz * vd.x * delta;
   camera.vp.y += iz * vd.y * delta;
   camera.vp.z += iz * vd.z * delta;
   camera.pr.x += iz * vd.x * delta;
   camera.pr.y += iz * vd.y * delta;
   camera.pr.z += iz * vd.z * delta;

    if (debug)
      fprintf(stderr,"Camera position: (%g,%g,%g)\n",
         camera.vp.x,camera.vp.y,camera.vp.z);

}
void display( void )
{
	 XYZ r; //right camera

	 double ratio, radians, wd2, ndfl;
	 double left, right, top, bottom, near_plane=1, far_plane=200;

	 /* Clip to avoid extreme stereo */
		if (stereo)
		{ 
			near_plane = camera.focallength / 5;
		}
   /* Misc stuff */
	 ratio  = camera.screenwidth / (double)camera.screenheight;
	 radians = DTOR * camera.aperture / 2;
	 wd2     = near_plane * tan(radians);	//half of the height
	 ndfl    = near_plane / camera.focallength; //ratio to calculate assymetric frustum

	 if (stereo) {

      /* Derive the right eye positions */
      CROSSPROD(camera.vd,camera.vu,r);
      Normalise(&r);
      r.x *= camera.eyesep / 2.0;
      r.y *= camera.eyesep / 2.0;
      r.z *= camera.eyesep / 2.0;
	
	  //Calculate the frustum for right camera
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      left  = - ratio * wd2 - 0.5 * camera.eyesep * ndfl;
      right =   ratio * wd2 - 0.5 * camera.eyesep * ndfl;
      top    =   wd2;
      bottom = - wd2;
      glFrustum(left,right,bottom,top,near_plane,far_plane);
	
	  //Calculate the viewing position and direction for right camera position
      glMatrixMode(GL_MODELVIEW);
      glDrawBuffer(GL_BACK_RIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      gluLookAt(camera.vp.x + r.x,camera.vp.y + r.y,camera.vp.z + r.z,
                camera.vp.x + r.x + camera.vd.x,
                camera.vp.y + r.y + camera.vd.y,
                camera.vp.z + r.z + camera.vd.z,
                camera.vu.x,camera.vu.y,camera.vu.z);
      
      DrawCube();

	  //Calculate the frustum for left camera
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      left  = - ratio * wd2 + 0.5 * camera.eyesep * ndfl;
      right =   ratio * wd2 + 0.5 * camera.eyesep * ndfl;
      top    =   wd2;
      bottom = - wd2;
      glFrustum(left,right,bottom,top,near_plane,far_plane);
	
	   //Calculate the viewing position and direction for right camera position
      glMatrixMode(GL_MODELVIEW);
      glDrawBuffer(GL_BACK_LEFT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      gluLookAt(camera.vp.x - r.x,camera.vp.y - r.y,camera.vp.z - r.z,
                camera.vp.x - r.x + camera.vd.x,
                camera.vp.y - r.y + camera.vd.y,
                camera.vp.z - r.z + camera.vd.z,
                camera.vu.x,camera.vu.y,camera.vu.z);
      
      DrawCube();

   } else {

   glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      left  = - ratio * wd2;
      right =   ratio * wd2;
      top    =   wd2;
      bottom = - wd2;
      glFrustum(left,right,bottom,top,near_plane,far_plane);

      glMatrixMode(GL_MODELVIEW);
      glDrawBuffer(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      
	  gluLookAt(camera.vp.x,camera.vp.y,camera.vp.z,
                camera.vp.x + camera.vd.x,
                camera.vp.y + camera.vd.y,
                camera.vp.z + camera.vd.z,
                camera.vu.x,camera.vu.y,camera.vu.z);
	 }
	


	DrawCube();
   glutSwapBuffers();
}

void build_lists()
{
	g_cube = glGenLists(3);													// Generate 2 Different Lists
	glNewList(g_cube,GL_COMPILE);											// Start With The Box List
		//glColor3f(0.0f, 1.0f, 0.0f);
		 glBegin ( GL_QUADS );
   // Front Face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		// Back Face
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		// Top Face
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		// Bottom Face
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		// Right face
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		// Left Face
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
	glEnd();
	glEndList();
}

void DrawCube(void)
{
   glPushMatrix();
   glTranslatef ( 0.0, 0.0, z );
   glRotatef ( xrot, 1.0, 0.0, 0.0 );
   glRotatef ( yrot, 0.0, 1.0, 0.0 );


   glBindTexture ( GL_TEXTURE_2D, texture_id[filter] );
	glCallList(g_cube);
  
   glPopMatrix();


     
   xrot+=xspeed;
	yrot+=yspeed;
}


void init(void)
{
	build_lists();

   glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.1f, 0.8f, 0.5f);				// Black Background
   glEnable ( GL_COLOR_MATERIAL );
   glColorMaterial ( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );

	glEnable ( GL_TEXTURE_2D );
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
   glGenTextures (3, texture_id);

   image_t   temp_image;

   glBindTexture ( GL_TEXTURE_2D, texture_id [0] );
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   tgaLoad  ( "3D_CUBE_GRAPHIC3.tga", &temp_image, TGA_FREE | TGA_LOW_QUALITY );

   glBindTexture ( GL_TEXTURE_2D, texture_id [1] );
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   tgaLoad  ( "3D_CUBE_GRAPHIC3.tga", &temp_image, TGA_FREE | TGA_LOW_QUALITY );

   glBindTexture ( GL_TEXTURE_2D, texture_id [2] );
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
   tgaLoad  ( "3D_CUBE_GRAPHIC3.tga", &temp_image, TGA_FREE | TGA_LOW_QUALITY );

   glEnable ( GL_CULL_FACE );

   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

   glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);	// Position The Light
	glEnable(GL_LIGHT1);								// Enable Light One
}
/*
   Handle a window reshape/resize
*/
void reshape(int w,int h)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glViewport(0,0,(GLsizei)w,(GLsizei)h);
   camera.screenwidth = w;
   camera.screenheight = h;
}



void keyboard ( unsigned char key, int x, int y )  // Create Keyboard Function
{
  switch ( key ) {
    case 27:        // When Escape Is Pressed...
      exit ( 0 );   // Exit The Program
      break;        // Ready For Next Case

	case 'f':
      fp=TRUE;
      filter+=1;
		if (filter>2)
		{
			filter=0;
      }
    	break;
    case 'q':
    	lp=TRUE;
					light=!light;
					if (!light)
					{
						glDisable(GL_LIGHTING);
					}
					else
					{
						glEnable(GL_LIGHTING);
					}
      break;
	  case 'h':                           /* Go home     */
      case 'H':
      CameraHome(0);
      break;
	  case 'i':                           /* Translate camera up */
	  case 'I':
		  TranslateCamera(0,1,0);
		  break;
	  case 'k':                           /* Translate camera down */
	  case 'K':
		  TranslateCamera(0,-1,0);
	  break;
	  case 'j':                           /* Translate camera left */
	  case 'J':
		  TranslateCamera(-1,0,0);
		  break;
	  case 'l':                           /* Translate camera right */
	  case 'L':
		  TranslateCamera(1,0,0);
	  break;
	  case 'n':                           /* Translate camera into screen */
	  case 'N':
		  TranslateCamera(0,0,1);
	  break;
	  case 'm':                           /* Translate camera away screen */
	  case 'M':
		  TranslateCamera(0,0,-1);
	  break;
	  case 'o':                           /* Decrease Eye Separation */
	  case 'O':
		  AdjustEyeSeparation(-1);
	  break;
	  case 'p':                           /* Increase Eye Separation*/
	  case 'P':
		  AdjustEyeSeparation(1);
	  break;
	  case 'y':                           /* Decrease focal length  */
	  case 'Y':
		  AdjustFocalLength(-1);
	  break;
	  case 'u':                           /* Increase focal length*/
	  case 'U':
		  AdjustFocalLength(1);
	  break;	
	  case '[':                           /* Roll anti clockwise */
      RotateCamera(0,0,-1);
      break;
	  case ']':                           /* Roll clockwise */
      RotateCamera(0,0,1);
      break;
	  default:        // Now Wrap It Up
	  break;
  }
}

void arrow_keys ( int a_keys, int x, int y )  // Create Special Function (required for arrow keys)
{
  switch ( a_keys ) {
    case GLUT_KEY_UP:     // When Up Arrow Is Pressed...
      xspeed-=0.01f;
      break;
    case GLUT_KEY_DOWN:               // When Down Arrow Is Pressed...
      xspeed+=0.01f;
      break;
    case GLUT_KEY_RIGHT:
    	yspeed+=0.01f;
      break;
    case GLUT_KEY_LEFT:
    	yspeed-=0.01f;
    	break;
    case GLUT_KEY_F1:
    	glutFullScreen ();
      break;
    case GLUT_KEY_F2:
		 glutPositionWindow(0, 0);
    	glutReshapeWindow ( 400, 300 );
      break;
    default:
      break;
  }
}

//-------------------------------------------------------------------------
//  This function is passed to the glutMouseFunc and is called
//  whenever the mouse is clicked.
//The x and y parameters indicate the mouse coordinates relative to the GLUT window when the mouse button's state changed.
//-------------------------------------------------------------------------
void mouse (int button, int state, int x, int y)				
{
    switch (button)
    {
        //  Left Button Clicked
        case GLUT_LEFT_BUTTON:

            switch (state)
            {
                //  Pressed
                case GLUT_DOWN:
					//  Print the mouse click position
					printf ("Mouse Left Down Click Position: %d, %d.\n", x, y);
					currentbutton = GLUT_LEFT_BUTTON;
				


                    break;
                //  Released
                case GLUT_UP:
                    printf ("Mouse Left Button Released (Up)...\n");
                    break;
            }

            break;

        //  Middle Button clicked
        case GLUT_MIDDLE_BUTTON:

            switch (state)
            {
                //  Pressed
                case GLUT_DOWN:
                    printf ("Mouse Middle Button Pressed (Down)...\n");
					currentbutton = GLUT_MIDDLE_BUTTON;
                break;
                //  Released
                case GLUT_UP:
                    printf ("Mouse Middle Button Released (Up)...\n");
                break;
            }

            break;

        //  Right Button Clicked
        case GLUT_RIGHT_BUTTON:

            switch (state)
            {
                //  Pressed
                case GLUT_DOWN:
                    printf ("Mouse Right Button Pressed (Down)...\n");
					currentbutton = GLUT_RIGHT_BUTTON;
					
                    break;
                //  Released
                case GLUT_UP:
                    printf ("Mouse Right Button Released (Up)...\n");
                    break;
            }

            break;
    }
}

//-------------------------------------------------------------------------
//  This function is passed to the glutMotionFunc and is called
//  whenever the mouse is dragged.
//-------------------------------------------------------------------------
void motion (int x, int y)
{
    //  Print the mouse drag position
    printf ("Mouse Drag Position: %d, %d.\n", x, y);

	static int xlast=-1,ylast=-1;
   int dx,dy;

   dx = x - xlast;
   dy = y - ylast;
   if (dx < 0)      dx = -1;
   else if (dx > 0) dx =  1;
   if (dy < 0)      dy = -1;
   else if (dy > 0) dy =  1;

   if (currentbutton == GLUT_LEFT_BUTTON)
      RotateCamera(-dx,dy,0);
   else if (currentbutton == GLUT_MIDDLE_BUTTON)
      RotateCamera(0,0,dx);

   xlast = x;
   ylast = y;
}
void main ( int argc, char** argv )   // Create Main Function For Bringing It All Together
{
  glutInit            ( &argc, argv ); // Erm Just Write It =)
  
  if (!stereo)
  glutInitDisplayMode ( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA); // Display Mode
  else
	 glutInitDisplayMode ( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_STEREO); // Display Mode
  
  glutInitWindowPosition (0,0);
  glutInitWindowSize  ( 500, 500 ); // If glutFullScreen wasn't called this is the window size
  glutCreateWindow    ( "NeHe Lesson 6- Ported by Rustad" ); // Window Title (argv[0] for current directory as title)
  init ();
  glutFullScreen      ( );          // Put Into Full Screen
  glutDisplayFunc     ( display );  // Matching Earlier Functions To Their Counterparts
  glutReshapeFunc     ( reshape );
  glutKeyboardFunc    ( keyboard );
  glutSpecialFunc     ( arrow_keys );
  glutMouseFunc (mouse);						//Calls mouse fucntion when button is clicked
  glutMotionFunc (motion);
  CameraHome(0);
  glutIdleFunc			 ( display );
  glutMainLoop        ( );          // Initialize The Main Loop
}



