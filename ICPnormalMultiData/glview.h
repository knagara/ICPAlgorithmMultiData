#include "stdafx.h"
#include "opencv.h"
#include <gl\freeglut.h>

extern const int fileTotal;
extern int fileCount; //現在処理中のファイル番号のカウンタ
extern int pointCount; //現在処理中のGLpointsのカウンタ
extern float pointSize;

extern int model_rows[];
extern int data_rows[];
extern const int rows;
extern const int cols;

#pragma region // --- OpenGL 変数 ---
extern const double MY_MINDISTANCE; // カメラから[0.1, 100.0]の範囲しか描画しない
extern const double MY_MAXDISTANCE;
extern const double CAMERA_DISTANCE; //カメラと座標系の距離

//extern GLfloat * points[];
extern GLfloat * allpoints;

extern int window1;
extern float angle;
extern GLdouble upY, fovy;
extern float axisSize, axisThick;
extern float cubeSize, cubeSize_;
extern float velocity, velocityKey;
extern int window_w, window_h;

extern int MouseX,MouseY;
extern bool M_left;
extern bool M_right;
extern bool M_middle;
extern int wheel;

extern GLdouble nearZ, farZ;

extern GLdouble eyeX, eyeY, eyeZ;
extern GLdouble eyeXdif, eyeYdif, eyeZdif;
extern GLdouble eyeR, eyeTheta, eyePhi;
extern int eyeXleft, eyeXright, eyeYup, eyeYdown, eyeZin, eyeZout;
extern GLdouble centerX, centerY, centerZ;
extern GLdouble centerXdif, centerYdif, centerZdif;
extern int centerXleft, centerXright, centerYup, centerYdown;
#pragma endregion


/// /////////物質質感の定義/////////////
struct MaterialStruct {
 GLfloat ambient[4];
 GLfloat diffuse[4];
 GLfloat specular[4];
 GLfloat shininess;
};


///　/////////////関数宣言/////////////////
void disp();
void initFlag(void);
void initParam(void);
void initParamMove(void);
void mouse(int button, int state, int x, int y);
void drag(int x, int y);
void passive(int x, int y);
void MouseWheel(int wheel_number, int direction, int x, int y);
void glut_keyboard(unsigned char key, int x, int y);
void animate(void);
void myGlutIdle();
void Initialize();
void DrawString(string str, int w, int h, int x0, int y0);
void drowSphere(double r, 
                double x, double y, double z, 
                MaterialStruct color);
void drowDodecahedron(double r, 
                double x, double y, double z, 
                MaterialStruct color);
void drowCuboid(double a, double b, double c, 
                double x, double y, double z, 
                MaterialStruct color);
void drowCuboid(double a, double b, double c, 
                double x, double y, double z, 
                MaterialStruct color, 
                double theta, 
                double nx, double ny, double nz);
