
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program was requested by Patrick Earl; hopefully someone else
   will write the equivalent Direct3D immediate mode program. */

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <vector>

using namespace std;

// ------------------------------ texture
#define RED 255,0,0
#define GRE 0,255,0
#define BLU 0,0,255

int texSize = 4;
vector<unsigned char> image;
unsigned int textureID;
// ------------------------------

GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};  /* Red diffuse light. */
GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};  /* Infinite light location. */
GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */

void drawBox(void) {
  int i;
  glBindTexture(GL_TEXTURE_2D, textureID);
  glEnable(GL_TEXTURE_2D);

  for (i = 0; i < 6; i++) {
    glBegin(GL_QUADS);
    glNormal3fv(&n[i][0]);
    glTexCoord2f(0.0, 0.0);  glVertex3fv(&v[faces[i][0]][0]);
    glTexCoord2f(1.0, 0.0);  glVertex3fv(&v[faces[i][1]][0]);
    glTexCoord2f(1.0, 1.0);  glVertex3fv(&v[faces[i][2]][0]);
    glTexCoord2f(0.0, 1.0);  glVertex3fv(&v[faces[i][3]][0]);
    glEnd();
  }
  glDisable(GL_TEXTURE_2D);
}

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawBox();
  glutSwapBuffers();
  glutPostRedisplay();
}

void init(void) {
  /* Setup cube vertex data. */
  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;

  /* Enable a single OpenGL light. */
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);

  /* Setup the view of the cube. */
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
    /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */

  /* Adjust cube position to be asthetic angle. */
  glTranslatef(0.0, 0.0, -1.0);
  glRotatef(60, 1.0, 0.0, 0.0);
  glRotatef(-20, 0.0, 0.0, 1.0);

  // texture
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  image = {
      BLU, GRE, RED, GRE,
      GRE, RED, GRE, RED,
      RED, GRE, RED, GRE,
      GRE, RED, GRE, RED,
  };

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
}

void reshape(int w, int h) {
    /*glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScalef(1, -1, 1);
    glTranslatef(0, -h, 0);
    glMatrixMode(GL_MODELVIEW);*/
}

string glGetParam(GLenum e) {
    const char* s = (const char*)glGetString(e);
    return s ? string(s) : "";
}

void testGLCapabilities() {
    cout << "Check OpenGL capabilities:" << endl;
    cout << " OpenGL vendor: " << glGetParam(GL_VENDOR) << endl;
    cout << " OpenGL version: " << glGetParam(GL_VERSION) << endl;
}

void runGlutTest(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("glut test");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  testGLCapabilities();
  init();
  glutMainLoop();
}