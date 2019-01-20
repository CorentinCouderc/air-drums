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
#include "NiHandViewer.h"
#include "play_sound.h"
#include <thread> 
#include <iostream>
#include <cmath>

#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;
//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define LENGTHOF(arr)			(sizeof(arr)/sizeof(arr[0]))


//---------------------------------------------------------------------------
// Method Definitions
//---------------------------------------------------------------------------

SimpleViewer& HandViewer::CreateInstance( xn::Context& context )
{
	assert(!sm_pInstance);
	return *(sm_pInstance = new HandViewer(context));
}

HandViewer::HandViewer(xn::Context& context)
:SimpleViewer(context),
m_HandTracker(context)
{}

XnStatus HandViewer::Init(int argc, char **argv)
{
	XnStatus rc;
	rc = SimpleViewer::Init(argc, argv);
	if(rc != XN_STATUS_OK)
	{
		return rc;
	}

	return m_HandTracker.Init();
}

XnStatus HandViewer::Run()
{
	XnStatus rc = m_HandTracker.Run();

	if(rc != XN_STATUS_OK)
	{
		return rc;
	}

	return SimpleViewer::Run(); // Does not return, enters OpenGL main loop instead
}


bool HandViewer::Hand_in_circle(float circle[],float x_hand, float y_hand)
{
	float dist=sqrt(pow(x_hand-circle[0],2.0) +pow(y_hand-circle[1],2.0));
	if (dist <= circle[2])
	   return true;
	
	return false;
}


bool HandViewer::Hand_calculs( float tab_position_x_y_0[], float area[])
{
	float moy_speed=0;
	float moy_dir=0;

	for (int j=0; j<9;j++)
	{
		float d=sqrt(pow(-tab_position_x_y_0[2*j]+tab_position_x_y_0[2*(j+1)],2.0) +pow(-tab_position_x_y_0[2*j+1]+tab_position_x_y_0[2*(j+1)+1],2.0));
		moy_speed+=d;			
	}

	moy_speed=moy_speed/(10);

	float v1=tab_position_x_y_0[19]-tab_position_x_y_0[17];
	float v2=tab_position_x_y_0[17]-tab_position_x_y_0[15];

	// if (moy_speed>=10.0 && moy_dir>0)
	if (moy_speed>=10.0 && tab_position_x_y_0[19]<area[1])
	{
		return true;
	}

	return false;
}

void HandViewer::UpdateHandInfo()
{
	typedef TrailHistory			History;
	typedef History::ConstIterator	HistoryIterator;
	typedef Trail::ConstIterator	TrailIterator;

	const TrailHistory&	history = m_HandTracker.GetHistory();

	// History points coordinates buffer
	XnFloat	coordinates[3 * MAX_HAND_TRAIL_LENGTH];

	etat_avant_main1=HAND_1_IN;
	etat_avant_main2=HAND_2_IN;
	int num_main=0;

	const HistoryIterator	hend = history.End();
	for(HistoryIterator		hit = history.Begin(); hit != hend; ++hit) // For every hand
	{
		num_main ++;
		
		// Dump the history to local buffer
		int				numpoints = 0;
		const Trail&	trail = hit->Value();	

		const TrailIterator	tend = trail.End();
		for(TrailIterator	tit = trail.Begin(); tit != tend; ++tit) // For each point of hand trail
		{
			XnPoint3D	point = *tit;
			m_depth.ConvertRealWorldToProjective(1, &point, &point);
			ScalePoint(point);
			coordinates[numpoints * 3] = point.X;
			coordinates[numpoints * 3 + 1] = point.Y;
			coordinates[numpoints * 3 + 2] = 0;

			++numpoints;

			Hand_in_circle1 = Hand_in_circle(SimpleViewer::circle_1,point.X,point.Y);
			Hand_in_circle2 = Hand_in_circle(SimpleViewer::circle_2,point.X,point.Y);
			Hand_in_circle3 = Hand_in_circle(SimpleViewer::circle_3,point.X,point.Y);
			Hand_in_circle4 = Hand_in_circle(SimpleViewer::circle_4,point.X,point.Y);
			Hand_in_circle5 = Hand_in_circle(SimpleViewer::circle_5,point.X,point.Y);

			// If one of the circle is entered by a hand we update tab_position for this hand
			if (!Hand_in_circle1 && !Hand_in_circle2 && !Hand_in_circle3 && !Hand_in_circle4 && !Hand_in_circle5)
			{
				if (num_main==1) // Hand 1
				{
					for (int k=0; k < LENGTHOF(tab_position_x_y)/2; k++)
					{
						if (k < ((LENGTHOF(tab_position_x_y)/2)-1))
						{
							tab_position_x_y_main1[k*2]=tab_position_x_y_main1[k*2+2];
							tab_position_x_y_main1[k*2+1]=tab_position_x_y_main1[k*2+3];
						}
						else
						{
							tab_position_x_y_main1[k*2]=point.X;
							tab_position_x_y_main1[k*2+1]=point.Y;		
						}
					}
				}
				if (num_main==2) // Hand 2
				{
					for (int k=0; k < LENGTHOF(tab_position_x_y)/2; k++)
					{
						if (k < ((LENGTHOF(tab_position_x_y)/2)-1))
						{
							tab_position_x_y_main2[k*2]=tab_position_x_y_main2[k*2+2];
							tab_position_x_y_main2[k*2+1]=tab_position_x_y_main2[k*2+3];	
						}
						else
						{
							tab_position_x_y_main2[k*2]=point.X;
							tab_position_x_y_main2[k*2+1]=point.Y;
						}
					}
				}
			}

			// Update position information of Hand 1
			if (num_main==1) 
			{ 
				if (Hand_in_circle1) {HAND_1_IN=1;}
				else if (Hand_in_circle2) {HAND_1_IN=2;}
				else if (Hand_in_circle3) {HAND_1_IN=3;}
				else if (Hand_in_circle4) {HAND_1_IN=4;}
				else if (Hand_in_circle5) {HAND_1_IN=5;}
				else {HAND_1_IN=0;}
			}

			// Update position information of Hand 2
			if (num_main==2) 
			{
				if (Hand_in_circle1) {HAND_2_IN=1;}
				else if (Hand_in_circle2) {HAND_2_IN=2;}
				else if (Hand_in_circle3) {HAND_2_IN=3;}
				else if (Hand_in_circle4) {HAND_2_IN=4;}
				else if (Hand_in_circle5) {HAND_2_IN=5;}
				else {HAND_2_IN=0;}
			}
		}

		// If one of the hands is in the circle 1, play sound of the object 1
		if( HAND_1_IN==1 || HAND_2_IN==1)
		{
			SimpleViewer::pos_HAND_for_circle1=true;
			if (HAND_1_IN==1 && HAND_1_IN!=etat_avant_main1)
			{
				int area=HAND_1_IN;
				main1_speed_ok=Hand_calculs(tab_position_x_y_main1,SimpleViewer::circle_1);
				if (main1_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle1=true;

					std::thread thread1 (play_sound,area);
					thread1.detach();
				}
			}
			if (HAND_2_IN==1 && HAND_2_IN!=etat_avant_main2)
			{
				int area=HAND_2_IN;
				main2_speed_ok=Hand_calculs(tab_position_x_y_main2,SimpleViewer::circle_1);
				if (main2_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle1=true;

					std::thread thread1 (play_sound,area);
					thread1.detach();
				}
			}
		}
		else
		{
			SimpleViewer::pos_HAND_for_circle1=false;
			SimpleViewer::speed_HAND_for_circle1=false;
		}



		// If one of the hands is in the circle 2, play sound of the object 2
		if( HAND_1_IN==2 || HAND_2_IN==2)
		{
			SimpleViewer::pos_HAND_for_circle2=true;
			if (HAND_1_IN==2 && HAND_1_IN!=etat_avant_main1)
			{
				int area=HAND_1_IN;
				main1_speed_ok=Hand_calculs(tab_position_x_y_main1,SimpleViewer::circle_2);
				if (main1_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle2=true;

					std::thread thread2 (play_sound,area);
					thread2.detach();
				}
			}
			if (HAND_2_IN==2 && HAND_2_IN!=etat_avant_main2)
			{
				int area=HAND_2_IN;
				main2_speed_ok=Hand_calculs(tab_position_x_y_main2,SimpleViewer::circle_2);
				if (main2_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle2=true;
				
					std::thread thread2 (play_sound,area);
					thread2.detach();
				}
			}
		}
		else
		{
			SimpleViewer::pos_HAND_for_circle2=false;
			SimpleViewer::speed_HAND_for_circle2=false;
		}



		// If one of the hands is in the circle 3, play sound of the object 3
		if( HAND_1_IN==3 || HAND_2_IN==3)
		{
			SimpleViewer::pos_HAND_for_circle3=true;
			if (HAND_1_IN==3 && HAND_1_IN!=etat_avant_main1)
			{
				int area=HAND_1_IN;
				main1_speed_ok=Hand_calculs(tab_position_x_y_main1,SimpleViewer::circle_3);
				if (main1_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle3=true;

					std::thread thread3 (play_sound,area);
					thread3.detach();
				}
			}
			if (HAND_2_IN==3 && HAND_2_IN!=etat_avant_main2)
			{
				int area=HAND_2_IN;
				main2_speed_ok=Hand_calculs(tab_position_x_y_main2,SimpleViewer::circle_3);
				if (main2_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle3=true;
				
					std::thread thread3 (play_sound,area);
					thread3.detach();
				}
			}
		}
		else
		{
			SimpleViewer::pos_HAND_for_circle3=false;
			SimpleViewer::speed_HAND_for_circle3=false;
		}




		// If one of the hands is in the circle 4, play sound of the object 4
		if( HAND_1_IN==4 || HAND_2_IN==4)
		{
			SimpleViewer::pos_HAND_for_circle4=true;
			if (HAND_1_IN==4 && HAND_1_IN!=etat_avant_main1)
			{
				int area=HAND_1_IN;
				main1_speed_ok=Hand_calculs(tab_position_x_y_main1,SimpleViewer::circle_4);
				if (main1_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle4=true;

					std::thread thread4 (play_sound,area);
					thread4.detach();
				}
			}
			if (HAND_2_IN==4 && HAND_2_IN!=etat_avant_main2)
			{
				int area=HAND_2_IN;
				main2_speed_ok=Hand_calculs(tab_position_x_y_main2,SimpleViewer::circle_4);
				if (main2_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle4=true;
				
					std::thread thread4 (play_sound,area);
					thread4.detach();
				}
			}
		}
		else
		{
			SimpleViewer::pos_HAND_for_circle4=false;
			SimpleViewer::speed_HAND_for_circle4=false;
		}



		// If one of the hands is in the circle 5, play sound of the object 5
		if( HAND_1_IN==5 || HAND_2_IN==5)
		{
			SimpleViewer::pos_HAND_for_circle5=true;
			if (HAND_1_IN==5 && HAND_1_IN!=etat_avant_main1)
			{
				int area=HAND_1_IN;
				main1_speed_ok=Hand_calculs(tab_position_x_y_main1,SimpleViewer::circle_5);
				if (main1_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle5=true;

					std::thread thread5 (play_sound,area);
					thread5.detach();
				}

			}
			if (HAND_2_IN==5 && HAND_2_IN!=etat_avant_main2)
			{
				int area=HAND_2_IN;
				main2_speed_ok=Hand_calculs(tab_position_x_y_main2,SimpleViewer::circle_5);
				if (main2_speed_ok)
				{
					SimpleViewer::speed_HAND_for_circle5=true;
				
					std::thread thread5 (play_sound,area);
					thread5.detach();
				}
			}
		}
		else
		{
			SimpleViewer::pos_HAND_for_circle5=false;
			SimpleViewer::speed_HAND_for_circle5=false;
		}

		// Update hand position
		SimpleViewer::transformation_model_3.translation.x = 2 * (coordinates[3] / 1280.0f - 0.5f);
		// SimpleViewer::transformation_model_3.translation.y = -2 * (coordinates[4] / 1024.0f - 0.5f);
	}
}


XnStatus HandViewer::InitOpenGL( int argc, char **argv )
{
	XnStatus rc = SimpleViewer::InitOpenGL(argc, argv); 

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return rc;
}
