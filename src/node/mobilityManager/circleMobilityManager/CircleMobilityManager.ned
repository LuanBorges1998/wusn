//****************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010                         *
//*  Developed at the ATP lab, Networked Systems research theme              *
//*  Author(s): Yuriy Tselishchev                                            *
//*  This file is distributed under the terms in the attached LICENSE file.  *
//*  If you do not find this file, copies can be found by writing to:        *
//*                                                                          *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
//*      Attention:  License Inquiry.                                        *
//*                                                                          *  
//****************************************************************************/

package node.mobilityManager.circleMobilityManager;

simple CircleMobilityManager like node.mobilityManager.iMobilityManager {
 parameters:
	bool collectTraceInfo = default (false);
	double updateInterval = default (1000);
	double xCoorDestination = default (0);
	double yCoorDestination = default (0);
	double zCoorDestination = default (0);
	double speed = default (1);
	
	
	//Circle Inet
	double cx; //@unit(m); // x coord of the center of the circle
    double cy; //@unit(m); // y coord of the center of the circle
    double cz = default (0);//@unit(m) = default(0m); // z coord of the center of the circle
    double r; //@unit(m); // radius of the circle
    //double speed @unit(mps) = default(0mps); // speed of the host
    double startAngle = default(uniform(0deg, 360deg)); //@unit(deg) = default(uniform(0deg, 360deg)); // starting angle
    @class(CircleMobility);
}
