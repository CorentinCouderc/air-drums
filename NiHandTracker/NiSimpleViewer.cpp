/****************************************************************************
*                                                                           *
*  OpenNI 1.x Alpha                                                         *
*  Copyright (C) 2011 PrimeSense Ltd.                                       *
*                                                                           *
*  This file is part of OpenNI.                                             *
*                                                                           *
*  OpenNI is free software: you can redistribute it and/or modify           *
*  it under the terms of the GNU Lesser General Public License as published *
*  by the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                      *
*                                                                           *
*  OpenNI is distributed in the hope that it will be useful,                *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
*  GNU Lesser General Public License for more details.                      *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
*                                                                           *
****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "NiSimpleViewer.h"
#include "NiHandViewer.h"
#include "play_sound.h"
#include <XnOS.h>
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <GL/glu.h>
#endif
#include <cmath>
#include <cassert>
#include <iostream>

#define GLEW_STATIC 1
#include <GL/glew.h>	
#include <GL/gl.h>

#define GL_GLEXT_PROTOTYPES
#include "lib/glutils.hpp"

#include "lib/mat4.hpp"
#include "lib/vec3.hpp"
#include "lib/vec2.hpp"
#include "lib/triangle_index.hpp"
#include "lib/vertex_opengl.hpp"
#include "lib/image.hpp"
#include "lib/mesh.hpp"


using namespace std;
using namespace xn;


//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define GL_WIN_SIZE_X	1280//640
#define GL_WIN_SIZE_Y	1024 //480
#define TEXTURE_SIZE	512

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_IMAGE //DISPLAY_MODE_DEPTH

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

//---------------------------------------------------------------------------
// Statics
//---------------------------------------------------------------------------
SimpleViewer* SimpleViewer::sm_pInstance = NULL;


//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
int signe_1=1;
int signe_2=1;
int etat_avant_main=0;

//Transformation de la vue (camera)
transformation transformation_view;

//Matrice de projection
mat4 projection;

//---------------------------------------------------------------------------
// GLUT Hooks
//---------------------------------------------------------------------------
void SimpleViewer::glutIdle (void)
{
	// Display the frame
	glutPostRedisplay();
}

void SimpleViewer::glutDisplay (void)
{
	Instance().Display();
}

void SimpleViewer::glutKeyboard (unsigned char key, int x, int y)
{
	Instance().OnKey(key, x, y);
}


//---------------------------------------------------------------------------
// Method Definitions
//---------------------------------------------------------------------------
SimpleViewer::SimpleViewer(xn::Context& context)
	:m_pTexMap(NULL),
	m_nTexMapX(0),
	m_nTexMapY(0),
	m_eViewState(DEFAULT_DISPLAY_MODE),
	m_rContext(context)
{}

SimpleViewer::~SimpleViewer()
{
	delete[] m_pTexMap;
	delete[] m_pDepthHist;
}

SimpleViewer& SimpleViewer::CreateInstance( xn::Context& context )
{
	assert(!sm_pInstance);
	return *(sm_pInstance = new SimpleViewer(context));
}

void SimpleViewer::DestroyInstance(SimpleViewer& instance)
{
	assert(sm_pInstance);
	assert(sm_pInstance == &instance);
	XN_REFERENCE_VARIABLE(instance);
	delete sm_pInstance;
	sm_pInstance = NULL;
}

SimpleViewer& SimpleViewer::Instance()
{
	assert(sm_pInstance);
	return *sm_pInstance;
}

XnStatus SimpleViewer::Init(int argc, char **argv)
{
	init_sound();

    XnStatus	rc;

	rc = m_rContext.FindExistingNode(XN_NODE_TYPE_DEPTH, m_depth);
	if (rc != XN_STATUS_OK)
	{
		printf("No depth node exists! Check your XML.");
		return rc;
	}

	rc = m_rContext.FindExistingNode(XN_NODE_TYPE_IMAGE, m_image);
	if (rc != XN_STATUS_OK)
	{
		printf("No image node exists! Check your XML.");
		return rc;
	}

	m_depth.GetMetaData(m_depthMD);
	m_image.GetMetaData(m_imageMD);

	// Hybrid mode isn't supported in this sample
	if (m_imageMD.FullXRes() != m_depthMD.FullXRes() || m_imageMD.FullYRes() != m_depthMD.FullYRes())
	{
		printf ("The device depth and image resolution must be equal!\n");
		return 1;
	}

	// RGB is the only image format supported.
	if (m_imageMD.PixelFormat() != XN_PIXEL_FORMAT_RGB24)
	{
		printf("The device image format must be RGB24\n");
		return 1;
	}

	// Texture map init
	m_nTexMapX = MIN_CHUNKS_SIZE(m_depthMD.FullXRes(), TEXTURE_SIZE);
	m_nTexMapY = MIN_CHUNKS_SIZE(m_depthMD.FullYRes(), TEXTURE_SIZE);
	m_pTexMap = new XnRGB24Pixel[m_nTexMapX * m_nTexMapY];

	m_pDepthHist = new float[m_depth.GetDeviceMaxDepth() + 1];

	return InitOpenGL(argc, argv);
}

XnStatus SimpleViewer::Run()
{
	// Per frame code is in Display
	glutMainLoop();	// Does not return!

	return XN_STATUS_OK;
}

XnStatus SimpleViewer::InitOpenGL(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(640, 480);

	glutCreateWindow ("Air Drums");
	glutSetCursor(GLUT_CURSOR_NONE);
	glewExperimental=true;

	InitOpenGLHooks();

	glewInit();
	init();

	return XN_STATUS_OK;
}

void SimpleViewer::init()
{
	 // Chargement du shader
	program_kinect = read_shader("../../NiHandTracker/media/shaders/shader.vert", "../../NiHandTracker/media/shaders/shader.frag"); 	PRINT_OPENGL_ERROR();

    //matrice de projection
    projection = matrice_projection(60.0f*M_PI/180.0f,1.0f,0.01f,100.0f);
    glUniformMatrix4fv(get_uni_loc(program_kinect,"projection"),1,false,pointeur(projection)); PRINT_OPENGL_ERROR();

    //centre de rotation de la 'camera' 
    transformation_view.rotation_center = vec3(0.0f,0.0f,-0.05f);

    //activation de la gestion de la profondeur
    glEnable(GL_DEPTH_TEST); PRINT_OPENGL_ERROR();

	// Init camera
	InitCamera();

	// Init objects
	Init_charleston();
	Init_caisse_claire1();
	Init_grosse_caisse();
	Init_caisse_claire2();
	Init_cymbale();

    Init_left_hand();
    Init_right_hand();
}

void SimpleViewer::InitCamera()
{
	//Creation manuelle du model 1

    //coordonnees geometriques des sommets
    vec3 p0=vec3(-57.5f,-57.5f,-99.0f);
    vec3 p1=vec3(-57.5f, 57.5f,-99.0f);
    vec3 p2=vec3( 57.5f,-57.5f,-99.0f);
    vec3 p3=vec3( 57.5f, 57.5f,-99.0f);

    //normales pour chaque sommet
    vec3 n0=vec3(0.0f,0.0f,1.0f);
    vec3 n1=n0;
    vec3 n2=n0;
    vec3 n3=n0;

    //couleur pour chaque sommet
    vec3 c0=vec3(1.0f,1.0f,1.0f);
    vec3 c1=c0;
    vec3 c2=c0;
    vec3 c3=c0;

    //texture du sommet
    vec2 t0=vec2(0.0f,0.9375f);
    vec2 t1=vec2(0.0f,0.0f);
    vec2 t2=vec2(0.625f,0.9375f);
    vec2 t3=vec2(0.625f,0.0f);

    vertex_opengl v0=vertex_opengl(p0,n0,c0,t0);
    vertex_opengl v1=vertex_opengl(p1,n1,c1,t1);
    vertex_opengl v2=vertex_opengl(p2,n2,c2,t2);
    vertex_opengl v3=vertex_opengl(p3,n3,c3,t3);

    //tableau entrelacant coordonnees-normales
    vertex_opengl geometrie[]={v0,v1,v2,v3};

    //indice des triangles
    triangle_index tri0=triangle_index(0,1,2);
    triangle_index tri1=triangle_index(1,3,2);
    triangle_index index[]={tri0,tri1};
    nb_triangle_camera = 2;

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_camera);                                             PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_camera);                                PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,sizeof(geometrie),geometrie,GL_STATIC_DRAW);  PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_camera);                                            PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_camera);                       PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(index),index,GL_STATIC_DRAW);  PRINT_OPENGL_ERROR();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);                                 PRINT_OPENGL_ERROR();
	glGenTextures(1, &texture_camera);                                     PRINT_OPENGL_ERROR();
	glBindTexture(GL_TEXTURE_2D, texture_camera);                          PRINT_OPENGL_ERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);          PRINT_OPENGL_ERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);          PRINT_OPENGL_ERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);      PRINT_OPENGL_ERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);      PRINT_OPENGL_ERROR();

	glUniform1i (get_uni_loc(program_kinect, "texture"), 0);			   PRINT_OPENGL_ERROR();
}

void SimpleViewer::Init_charleston()
{
    mesh m_charleston = load_obj_file("../../NiHandTracker/media/objets/elements/charleston.obj");

    // Affecte une transformation sur les sommets du maillage
    float s_charleston = 2.5f;
    mat4 transform = mat4(   s_charleston, 0.0f, 0.0f, -18.0f,
                          0.0f,    s_charleston, 0.0f, -8.0f,
                          0.0f, 0.0f,   s_charleston , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

	apply_deformation(&m_charleston,matrice_rotation(-M_PI/10.0f,1.0f,0.0f,0.0f));
    apply_deformation(&m_charleston,matrice_rotation(-M_PI/8.0f,0.0f,0.0f,1.0f));

    apply_deformation(&m_charleston,transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    transformation_model_charleston.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_charleston);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_charleston,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_charleston); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_charleston); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_charleston.vertex.size()*sizeof(vertex_opengl),&m_charleston.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_charleston); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_charleston); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_charleston.connectivity.size()*sizeof(triangle_index),&m_charleston.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_charleston = m_charleston.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer2.tga",&texture_charleston);
}




void SimpleViewer::Init_caisse_claire1()
{
    mesh m_caisse_claire1 = load_obj_file("../../NiHandTracker/media/objets/elements/caisse_claire.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 2.5f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, -13.0f,
                          0.0f,    s, 0.0f, -14.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

	//apply_deformation(&m_caisse_claire1,matrice_rotation(-M_PI/10.0f,1.0f,0.0f,0.0f));
    apply_deformation(&m_caisse_claire1,matrice_rotation(-M_PI/5.0f,1.0f,0.0f,0.0f));

    apply_deformation(&m_caisse_claire1,transform);

    // Centre la rotation du modele caisse_claire1 autour de son centre de gravite approximatif
    transformation_model_caisse_claire1.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_caisse_claire1);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_caisse_claire1,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_caisse_claire1); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_caisse_claire1); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_caisse_claire1.vertex.size()*sizeof(vertex_opengl),&m_caisse_claire1.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_caisse_claire1); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_caisse_claire1); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_caisse_claire1.connectivity.size()*sizeof(triangle_index),&m_caisse_claire1.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_caisse_claire1 = m_caisse_claire1.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer6.tga",&texture_caisse_claire1);
}

void SimpleViewer::Init_grosse_caisse()
{
    mesh m_grosse_caisse = load_obj_file("../../NiHandTracker/media/objets/elements/grosse_caisse.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 6.0f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, -0.0f,
                          0.0f,    s, 0.0f, -14.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

	apply_deformation(&m_grosse_caisse,matrice_rotation(+M_PI/12.0f,0.0f,1.0f,0.0f));
    apply_deformation(&m_grosse_caisse,matrice_rotation(-M_PI/1.4f,1.0f,0.0f,0.0f));

    apply_deformation(&m_grosse_caisse,transform);

    // Centre la rotation du modele grosse_caisse autour de son centre de gravite approximatif
    transformation_model_grosse_caisse.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_grosse_caisse);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_grosse_caisse,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_grosse_caisse); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_grosse_caisse); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_grosse_caisse.vertex.size()*sizeof(vertex_opengl),&m_grosse_caisse.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_grosse_caisse); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_grosse_caisse); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_grosse_caisse.connectivity.size()*sizeof(triangle_index),&m_grosse_caisse.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_grosse_caisse = m_grosse_caisse.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer10.tga",&texture_grosse_caisse);
}



void SimpleViewer::Init_caisse_claire2()
{
    mesh m_caisse_claire2 = load_obj_file("../../NiHandTracker/media/objets/elements/caisse_claire.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 2.5f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, +6.0f,
                          0.0f,    s, 0.0f, -14.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

	//apply_deformation(&m_caisse_claire2,matrice_rotation(-M_PI/10.0f,1.0f,0.0f,0.0f));
    apply_deformation(&m_caisse_claire2,matrice_rotation(-M_PI/5.0f,1.0f,0.0f,0.0f));

    apply_deformation(&m_caisse_claire2,transform);

    // Centre la rotation du modele caisse_claire2 autour de son centre de gravite approximatif
    transformation_model_caisse_claire2.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_caisse_claire2);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_caisse_claire2,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_caisse_claire2); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_caisse_claire2); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_caisse_claire2.vertex.size()*sizeof(vertex_opengl),&m_caisse_claire2.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_caisse_claire2); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_caisse_claire2); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_caisse_claire2.connectivity.size()*sizeof(triangle_index),&m_caisse_claire2.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_caisse_claire2 = m_caisse_claire2.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer1.tga",&texture_caisse_claire2);


}


void SimpleViewer::Init_cymbale()
{
    mesh m_cymbale = load_obj_file("../../NiHandTracker/media/objets/elements/charleston.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 2.5f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, +18.0f,
                          0.0f,    s, 0.0f, -8.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

	apply_deformation(&m_cymbale,matrice_rotation(-M_PI/10.0f,1.0f,0.0f,0.0f));
    apply_deformation(&m_cymbale,matrice_rotation(+M_PI/8.0f,0.0f,0.0f,1.0f));

    apply_deformation(&m_cymbale,transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    transformation_model_cymbale.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_cymbale);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_cymbale,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_cymbale); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_cymbale); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_cymbale.vertex.size()*sizeof(vertex_opengl),&m_cymbale.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_cymbale); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_cymbale); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_cymbale.connectivity.size()*sizeof(triangle_index),&m_cymbale.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_cymbale = m_cymbale.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer5.tga",&texture_cymbale);
}

void SimpleViewer::Init_left_hand()
{
    mesh m_left_hand = load_obj_file("../../NiHandTracker/media/objets/elements/left_hand_lp.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 1.0f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, -6.0f,
                          0.0f,    s, 0.0f, +3.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

    apply_deformation(&m_left_hand,matrice_rotation(+M_PI/2.0f,0.0f,1.0f,0.0f));
    apply_deformation(&m_left_hand,matrice_rotation(+M_PI/10.0f,1.0f,0.0f,0.0f));


    apply_deformation(&m_left_hand,transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    transformation_model_left_hand.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_left_hand);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_left_hand,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_left_hand); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_left_hand); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_left_hand.vertex.size()*sizeof(vertex_opengl),&m_left_hand.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_left_hand); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_left_hand); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_left_hand.connectivity.size()*sizeof(triangle_index),&m_left_hand.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_left_hand = m_left_hand.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer11.tga",&texture_left_hand);
}


void SimpleViewer::Init_right_hand()
{
    mesh m_right_hand = load_obj_file("../../NiHandTracker/media/objets/elements/right_hand_lp.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 1.0f;
    mat4 transform = mat4(   s, 0.0f, 0.0f, +6.0f,
                          0.0f,    s, 0.0f, +3.0f,
                          0.0f, 0.0f,   s , -30.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

    apply_deformation(&m_right_hand,matrice_rotation(-M_PI/10.0f,1.0f,0.0f,0.0f));
    apply_deformation(&m_right_hand,matrice_rotation(+M_PI,0.0f,1.0f,0.0f));

    apply_deformation(&m_right_hand,transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    transformation_model_right_hand.rotation_center = vec3(0.0f,0.0f,-0.05f);
    

    // Calcul automatique des normales du maillage
    update_normals(&m_right_hand);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m_right_hand,vec3(1.0f,1.0f,1.0f));

    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1,&vbo_right_hand); PRINT_OPENGL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER,vbo_right_hand); PRINT_OPENGL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER,m_right_hand.vertex.size()*sizeof(vertex_opengl),&m_right_hand.vertex[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();


    //attribution d'un autre buffer de donnees
    glGenBuffers(1,&vboi_right_hand); PRINT_OPENGL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_right_hand); PRINT_OPENGL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_right_hand.connectivity.size()*sizeof(triangle_index),&m_right_hand.connectivity[0],GL_STATIC_DRAW); PRINT_OPENGL_ERROR();

    // Nombre de triangles de l'objet 1
    nb_triangle_right_hand = m_right_hand.connectivity.size();

    // Chargement de la texture
    load_texture("../../NiHandTracker/media/objets/color/Layer11.tga",&texture_right_hand);
}

void SimpleViewer::InitOpenGLHooks()
{
	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);
}

void SimpleViewer::Display()
{
	XnStatus		rc = XN_STATUS_OK;

	// Read a new frame
	rc = m_rContext.WaitAnyUpdateAll();
	if (rc != XN_STATUS_OK)
	{
		printf("Read failed: %s\n", xnGetStatusString(rc));
		return;
	}

	m_depth.GetMetaData(m_depthMD);
	m_image.GetMetaData(m_imageMD);


	xnOSMemSet(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(XnRGB24Pixel));

	// check if we need to draw image frame to texture
	if (m_eViewState == DISPLAY_MODE_IMAGE)
	{
		const XnRGB24Pixel* pImageRow = m_imageMD.RGB24Data();
		XnRGB24Pixel* pTexRow = m_pTexMap + m_imageMD.YOffset() * m_nTexMapX;

		for (XnUInt y = 0; y < m_imageMD.YRes(); ++y)
		{
			const XnRGB24Pixel* pImage = pImageRow;
			XnRGB24Pixel* pTex = pTexRow + m_imageMD.XOffset();

			for (XnUInt x = 0; x < m_imageMD.XRes(); ++x, ++pImage, ++pTex)
			{
				*pTex = *pImage;
			}
			pImageRow += m_imageMD.XRes();
			pTexRow += m_nTexMapX;
		}
	}

    /****** Draw objects *******/

	// Clear the OpenGL buffers
    glClearColor(0.5f, 0.6f, 0.9f, 1.0f);                 PRINT_OPENGL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   PRINT_OPENGL_ERROR();

    // Affecte les parametres uniformes de la vue (identique pour tous les modeles de la scene)
    {
        //envoie de la rotation
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_view"),1,false,pointeur(transformation_view.rotation)); PRINT_OPENGL_ERROR();

        //envoie du centre de rotation
        vec3 cv = transformation_view.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_view") , cv.x,cv.y,cv.z , 0.0f); PRINT_OPENGL_ERROR();

        //envoie de la translation
        vec3 tv = transformation_view.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_view") , tv.x,tv.y,tv.z , 0.0f); PRINT_OPENGL_ERROR();
    }

    // Calculates hand position
	UpdateHandInfo();

    // Set this bool to apply projection
    apply_projection = true;

	// Draw Camera
	glBindTexture(GL_TEXTURE_2D, texture_camera); PRINT_OPENGL_ERROR();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTexMapX, m_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pTexMap);
	draw_camera();

	if(speed_HAND_for_circle1)
	{
		draw_charleston();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		draw_charleston();
		glDisable(GL_BLEND); 
	}


	if(speed_HAND_for_circle2)
	{
		draw_caisse_claire1();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		draw_caisse_claire1();
		glDisable(GL_BLEND); 
	}


	if(speed_HAND_for_circle3)
	{
		draw_grosse_caisse();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		draw_grosse_caisse();
		glDisable(GL_BLEND); 
	}


	if(speed_HAND_for_circle4)
	{
		draw_caisse_claire2();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		draw_caisse_claire2();
		glDisable(GL_BLEND); 
	}


	if(speed_HAND_for_circle5)
	{
		draw_cymbale();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		draw_cymbale();
		glDisable(GL_BLEND); 
	}

    glDisable(GL_BLEND); 
	

	if (HandTracker::nbr_of_hands==0 )
    {
        if (etat_avant_main==1)
        {
            transformation_model_right_hand.translation.z=0.0f;
            transformation_model_left_hand.translation.z=0.0f;
            etat_avant_main=0;
        }

        draw_left_hand();
        draw_right_hand();
    }

    if (HandTracker::nbr_of_hands==1 && transformation_model_3.translation.x>0)
    {
        draw_left_hand();
        etat_avant_main=1;
    }
    if(HandTracker::nbr_of_hands==1 && transformation_model_3.translation.x<0)
    {
        draw_right_hand();

        etat_avant_main=1;
    }
    if (HandTracker::nbr_of_hands==2)
    {
        etat_avant_main=1;
    }

	glutSwapBuffers();
}

void SimpleViewer::draw_camera()
{
    //envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_1.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_1.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_1.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
		
		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);
	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_camera);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();
    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_camera);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_camera);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_camera, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }
}

void SimpleViewer::draw_charleston()
{
	//envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_charleston.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_charleston.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_charleston.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
		
		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);
	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_charleston);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_charleston);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_charleston);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_charleston, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }
}

void SimpleViewer::draw_caisse_claire1()
{
	//envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_caisse_claire1.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_caisse_claire1.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_caisse_claire1.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
    
		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);

	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_caisse_claire1);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_caisse_claire1);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_caisse_claire1);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_caisse_claire1, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }

}


void SimpleViewer::draw_grosse_caisse()
{
	//envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_grosse_caisse.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_grosse_caisse.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_grosse_caisse.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
		
		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);
	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_grosse_caisse);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_grosse_caisse);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_grosse_caisse);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_grosse_caisse, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }

}



void SimpleViewer::draw_caisse_claire2()
{
	//envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_caisse_claire2.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_caisse_claire2.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_caisse_claire2.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();

		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);

	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_caisse_claire2);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_caisse_claire2);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_caisse_claire2);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_caisse_claire2, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }

}


void SimpleViewer::draw_cymbale()
{
	//envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_cymbale.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_cymbale.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_cymbale.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
		
		glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);

	}

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_cymbale);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_cymbale);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_cymbale);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_cymbale, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }
}

void SimpleViewer::draw_left_hand()
{
    
    if (transformation_model_left_hand.translation.z >5.0f)
    {
        signe_1=-1;
    }
    if(transformation_model_left_hand.translation.z <0.0f)
    {
       signe_1=1;
    }

    transformation_model_left_hand.translation.z = transformation_model_left_hand.translation.z+ 0.13f*(signe_1);

    //envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_left_hand.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_left_hand.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_left_hand.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
        
        glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);

    }

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_left_hand);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_left_hand);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_left_hand);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_left_hand, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }
}

void SimpleViewer::draw_right_hand()
{
    

    if (transformation_model_right_hand.translation.z >5.0f)
    {
        signe_2=-1;
    }
    if(transformation_model_right_hand.translation.z <0.0f)
    {
       signe_2=1;
    }

    transformation_model_right_hand.translation.z = transformation_model_right_hand.translation.z+ 0.13f*(signe_2);

    //envoie des parametres uniformes
    {
        glUniformMatrix4fv(get_uni_loc(program_kinect,"rotation_model"),1,false,pointeur(transformation_model_right_hand.rotation));    PRINT_OPENGL_ERROR();

        vec3 c = transformation_model_right_hand.rotation_center;
        glUniform4f(get_uni_loc(program_kinect,"rotation_center_model") , c.x,c.y,c.z , 0.0f);                                 PRINT_OPENGL_ERROR();

        vec3 t = transformation_model_right_hand.translation;
        glUniform4f(get_uni_loc(program_kinect,"translation_model") , t.x,t.y,t.z , 0.0f);                                     PRINT_OPENGL_ERROR();
        
        glUniform1i(get_uni_loc(program_kinect, "apply_projection"), apply_projection);

    }

    //placement des VBO
    {
        //selection du VBO courant
        glBindBuffer(GL_ARRAY_BUFFER,vbo_right_hand);                                                    PRINT_OPENGL_ERROR();

        // mise en place des differents pointeurs
        glEnableClientState(GL_VERTEX_ARRAY);                                                          PRINT_OPENGL_ERROR();
        glVertexPointer(3, GL_FLOAT, sizeof(vertex_opengl), 0);                                        PRINT_OPENGL_ERROR();

        glEnableClientState(GL_NORMAL_ARRAY); PRINT_OPENGL_ERROR();                                    PRINT_OPENGL_ERROR();
        glNormalPointer(GL_FLOAT, sizeof(vertex_opengl), buffer_offset(sizeof(vec3)));                 PRINT_OPENGL_ERROR();

        glEnableClientState(GL_COLOR_ARRAY); PRINT_OPENGL_ERROR();                                     PRINT_OPENGL_ERROR();
        glColorPointer(3,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(2*sizeof(vec3)));              PRINT_OPENGL_ERROR();

        glEnableClientState(GL_TEXTURE_COORD_ARRAY); PRINT_OPENGL_ERROR();                             PRINT_OPENGL_ERROR();
        glTexCoordPointer(2,GL_FLOAT, sizeof(vertex_opengl), buffer_offset(3*sizeof(vec3)));           PRINT_OPENGL_ERROR();

    }

    //affichage
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi_right_hand);                           PRINT_OPENGL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture_right_hand);                             PRINT_OPENGL_ERROR();
        glDrawElements(GL_TRIANGLES, 3*nb_triangle_right_hand, GL_UNSIGNED_INT, 0);     PRINT_OPENGL_ERROR();
    }
}

void SimpleViewer::load_texture(const char* filename, GLuint *texture_id)
{
    // Chargement d'une texture (seul les textures tga sont supportes)
    Image  *image = image_load_tga(filename);
    if (image) //verification que l'image est bien chargee
    {

        //Creation d'un identifiant pour la texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); PRINT_OPENGL_ERROR();
        glGenTextures(1, texture_id); PRINT_OPENGL_ERROR();

        //Selection de la texture courante a partir de son identifiant
        glBindTexture(GL_TEXTURE_2D, *texture_id); PRINT_OPENGL_ERROR();

        //Parametres de la texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); PRINT_OPENGL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); PRINT_OPENGL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); PRINT_OPENGL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); PRINT_OPENGL_ERROR();

        //Envoie de l'image en memoire video
        if(image->type==IMAGE_TYPE_RGB){ //image RGB
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data); PRINT_OPENGL_ERROR();}
        else if(image->type==IMAGE_TYPE_RGBA){ //image RGBA
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data); PRINT_OPENGL_ERROR();}
        else{
            std::cout<<"Image type not handled"<<std::endl;}

        delete image;
    }
    else
    {
        std::cerr<<"Erreur chargement de l'image, etes-vous dans le bon repertoire?"<<std::endl;
        abort();
    }

    glUniform1i (get_uni_loc(program_kinect, "texture"), 0); PRINT_OPENGL_ERROR();
}

void SimpleViewer::OnKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
		exit (1);
	}
}

void SimpleViewer::ScalePoint(XnPoint3D& point)
{
	point.X *= GL_WIN_SIZE_X;
	point.X /= m_depthMD.XRes();

	point.Y *= GL_WIN_SIZE_Y;
	point.Y /= m_depthMD.YRes();
}
