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
#ifndef NI_HAND_VIEWER_H__
#define NI_HAND_VIEWER_H__

#include "NiSimpleViewer.h"

#include "NiHandTracker.h"

class HandViewer: public SimpleViewer
{
public:
	// Singleton



	static SimpleViewer& CreateInstance(xn::Context& context);

	virtual XnStatus Init(int argc, char **argv);
	virtual XnStatus Run();	//Does not return if successful

protected:
	HandViewer(xn::Context& context);
	bool Hand_in_circle(float circle[],float x_hand, float y_hand);
	bool Hand_calculs( float tab_position_x_y_0[], float area[]);


	virtual void UpdateHandInfo();
	

	virtual XnStatus InitOpenGL(int argc, char **argv);

private:
	HandTracker	m_HandTracker;
	bool Hand_in_circle1;
	bool Hand_in_circle2;
	bool Hand_in_circle3;
	bool Hand_in_circle4;
	bool Hand_in_circle5;


	int HAND_1_IN; //=1 for hand 1, =2 for hand 2
	int HAND_2_IN;

	bool etat_avant;
	int etat_avant_main1;
	int etat_avant_main2;
	float tab_position_x_y[20];
	float tab_position_x_y_main1[20];
	float tab_position_x_y_main2[20];

	bool main1_speed_ok;
	bool main2_speed_ok;

	bool circle1_activation;
	bool circle2_activation;
	bool circle3_activation;
	bool circle4_activation;


};

#endif //NI_HAND_VIEWER_H__