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
#ifndef NI_SIMPLE_VIEWER_H__
#define NI_SIMPLE_VIEWER_H__

#include <XnCppWrapper.h>

#define GLEW_STATIC 1
#include <GL/glew.h>	
#include <GL/gl.h>

#include "lib/mat4.hpp"
#include "lib/vec3.hpp"


enum DisplayModes_e
{
	DISPLAY_MODE_OVERLAY,
	DISPLAY_MODE_DEPTH,
	DISPLAY_MODE_IMAGE
};

//Matrice de transformation
struct transformation
{
    mat4 rotation;
    vec3 rotation_center;
    vec3 translation;

    transformation():rotation(),rotation_center(),translation(){}
};


class SimpleViewer
{
public:
	// Booleans for zone detection
	bool pos_HAND_for_circle1;
	bool speed_HAND_for_circle1;

	bool pos_HAND_for_circle2;
	bool speed_HAND_for_circle2;

	bool pos_HAND_for_circle3;
	bool speed_HAND_for_circle3;

	bool pos_HAND_for_circle4;
	bool speed_HAND_for_circle4;

	bool pos_HAND_for_circle5;
	bool speed_HAND_for_circle5;

	float circle_1[3]={140,540,120}; //{cx,cy,r}
	float circle_2[3]={330,740,110}; //{cx,cy,r}
	float circle_3[3]={640,890,150}; //{cx,cy,r}
	float circle_4[3]={930,740,110}; //{cx,cy,r}
	float circle_5[3]={1140,540,120}; //{cx,cy,r}

	static SimpleViewer& CreateInstance(xn::Context& context);
	static void DestroyInstance(SimpleViewer& instance);

	virtual XnStatus Init(int argc, char **argv);
	virtual XnStatus Run();	//Does not return

	// Buffers for OpenGL
	GLuint vbo_camera;
	GLuint vboi_camera;
	int nb_triangle_camera;

	GLuint vbo_charleston;
	GLuint vboi_charleston;
	int nb_triangle_charleston;

	GLuint vbo_caisse_claire1;
	GLuint vboi_caisse_claire1;
	int nb_triangle_caisse_claire1;

	GLuint vbo_grosse_caisse;
	GLuint vboi_grosse_caisse;
	int nb_triangle_grosse_caisse;

	GLuint vbo_caisse_claire2;
	GLuint vboi_caisse_claire2;
	int nb_triangle_caisse_claire2;

	GLuint vbo_cymbale;
	GLuint vboi_cymbale;
	int nb_triangle_cymbale;

	GLuint vbo_left_hand;
	GLuint vboi_left_hand;
	int nb_triangle_left_hand;

	GLuint vbo_right_hand;
	GLuint vboi_right_hand;
	int nb_triangle_right_hand;



	//Transformation des modeles
	transformation transformation_model_1;
	transformation transformation_model_2;
	transformation transformation_model_3;

	transformation transformation_model_charleston;
	transformation transformation_model_caisse_claire1;
	transformation transformation_model_grosse_caisse;
	transformation transformation_model_caisse_claire2;
	transformation transformation_model_cymbale;

	transformation transformation_model_left_hand;
	transformation transformation_model_right_hand;

	// Bool for projection application
	bool apply_projection = true;

protected:
	SimpleViewer(xn::Context& context);
	virtual ~SimpleViewer();

	virtual void Display();
	virtual void UpdateHandInfo(){};	// Overload to draw over the screen image
	void draw_camera();

	void draw_charleston();
	void draw_caisse_claire1();
	void draw_grosse_caisse();
	void draw_caisse_claire2();
	void draw_cymbale();

	void draw_left_hand();
	void draw_right_hand();

	virtual void OnKey(unsigned char key, int x, int y);

	virtual XnStatus InitOpenGL(int argc, char **argv);
	void InitOpenGLHooks();
	void init();

	void InitCamera();

	void Init_charleston();
	void Init_caisse_claire1();
	void Init_grosse_caisse();
	void Init_caisse_claire2();
	void Init_cymbale();

	void Init_left_hand();
	void Init_right_hand();

	void load_texture(const char* filename, GLuint *texture_id);

	static SimpleViewer& Instance();

	xn::Context&		m_rContext;
	xn::DepthGenerator	m_depth;
	xn::ImageGenerator	m_image;

	static SimpleViewer*	sm_pInstance;

	void ScalePoint(XnPoint3D& point);
private:
	// GLUT callbacks
	static void glutIdle();
	static void glutDisplay();
	static void glutKeyboard(unsigned char key, int x, int y);

	// Shader programs
	GLuint              program_kinect;

	// Textures
	GLuint  			texture_camera;

	GLuint  			texture_charleston;
	GLuint  			texture_caisse_claire1;
	GLuint  			texture_grosse_caisse;
	GLuint  			texture_caisse_claire2;
	GLuint  			texture_cymbale;

	GLuint  			texture_left_hand;
	GLuint  			texture_right_hand;

	// Camera
	float*				m_pDepthHist;
	XnRGB24Pixel*		m_pTexMap;
	unsigned int		m_nTexMapX;
	unsigned int		m_nTexMapY;
	DisplayModes_e		m_eViewState;
	xn::DepthMetaData	m_depthMD;
	xn::ImageMetaData	m_imageMD;
};

#endif //NI_SIMPLE_VIEWER_H__