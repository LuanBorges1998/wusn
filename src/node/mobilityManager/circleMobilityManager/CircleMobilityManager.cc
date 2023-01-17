/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "node/mobilityManager/circleMobilityManager/CircleMobilityManager.h"

Define_Module(CircleMobilityManager);

void CircleMobilityManager::initialize()
{
    //Circle Inet
    VirtualMobilityManager::initialize();

    cx = 0;
    cy = 0;
    cz = 0;
    r = -1;
    startAngle = 0;
    speed = 0;
    omega = 0;
    angle = 0;

    //Recebendo os parâmetros
    cx = par("cx");
    cy = par("cy");
    cz = par("cz");
    r = par("r");
    ASSERT(r > 0);
    startAngle = par("startAngle").doubleValue() / 180.0 * M_PI;
    speed = par("speed");
    omega = speed / r;
    //stationary = (omega == 0);

	//VirtualMobilityManager::initialize();

	updateInterval = par("updateInterval");
	updateInterval = updateInterval / 1000;

	loc1_x = nodeLocation.x;
	loc1_y = nodeLocation.y;
	loc1_z = nodeLocation.z;
	loc2_x = par("xCoorDestination");
	loc2_y = par("yCoorDestination");
	loc2_z = par("zCoorDestination");
	speed = par("speed");
	distance = sqrt(pow(loc1_x - loc2_x, 2) + pow(loc1_y - loc2_y, 2) +
		 pow(loc1_z - loc2_z, 2));
	direction = 1;
	if (speed > 0 && distance > 0) {
		double tmp = (distance / speed) / updateInterval;
		incr_x = (loc2_x - loc1_x) / tmp;
		incr_y = (loc2_y - loc1_y) / tmp;
		incr_z = (loc2_z - loc1_z) / tmp;
		setLocation(loc1_x, loc1_y, loc1_z);
		scheduleAt(simTime() + updateInterval,
			new MobilityManagerMessage("Periodic location update message", MOBILITY_PERIODIC));
	}
}

void CircleMobilityManager::setInitialPosition()
{
    move();
}

void CircleMobilityManager::move()
{
    angle = startAngle + omega * simTime().dbl();
    //lastPosition.x = cx + r * cos(angle);
    //lastPosition.y = cy + r * sin(angle);
    //lastPosition.z = cz;
    //lastSpeed.x = -sin(angle) * speed;
    //lastSpeed.y = cos(angle) * speed;
    //lastSpeed.z = 0;
    // do something if we reach the wall
    //Coord dummyCoord;
    double dummyAngle;
    //handleIfOutside(REFLECT, dummyCoord, dummyCoord, dummyAngle);
}

void CircleMobilityManager::handleMessage(cMessage * msg)
{
	int msgKind = msg->getKind();
	switch (msgKind) {

		case MOBILITY_PERIODIC:{
			if (direction) {
				nodeLocation.x += incr_x;
				nodeLocation.y += incr_y;
				nodeLocation.z += incr_z;
				if (   (incr_x > 0 && nodeLocation.x > loc2_x)
				    || (incr_x < 0 && nodeLocation.x < loc2_x)
				    || (incr_y > 0 && nodeLocation.y > loc2_y)
				    || (incr_y < 0 && nodeLocation.y < loc2_y)
				    || (incr_z > 0 && nodeLocation.z > loc2_z)
				    || (incr_z < 0 && nodeLocation.z < loc2_z)) {
					direction = 0;
					nodeLocation.x -= (nodeLocation.x - loc2_x) * 2;
					nodeLocation.y -= (nodeLocation.y - loc2_y) * 2;
					nodeLocation.z -= (nodeLocation.z - loc2_z) * 2;
				}
			} else {
				nodeLocation.x -= incr_x;
				nodeLocation.y -= incr_y;
				nodeLocation.z -= incr_z;
				if (   (incr_x > 0 && nodeLocation.x < loc1_x)
				    || (incr_x < 0 && nodeLocation.x > loc1_x)
				    || (incr_y > 0 && nodeLocation.y < loc1_y)
				    || (incr_y < 0 && nodeLocation.y > loc1_y)
				    || (incr_z > 0 && nodeLocation.z < loc1_z)
				    || (incr_z < 0 && nodeLocation.z > loc1_z)) {
					direction = 1;
					nodeLocation.x -= (nodeLocation.x - loc1_x) * 2;
					nodeLocation.y -= (nodeLocation.y - loc1_y) * 2;
					nodeLocation.z -= (nodeLocation.z - loc1_z) * 2;
				}
			}
			notifyWirelessChannel();
			scheduleAt(simTime() + updateInterval,
				new MobilityManagerMessage("Periodic location update message", MOBILITY_PERIODIC));

			trace() << "changed location(x:y:z) to " << nodeLocation.x << 
					":" << nodeLocation.y << ":" << nodeLocation.z;
			break;
		}

		default:{
			trace() << "WARNING: Unexpected message " << msgKind;
		}
	}

	delete msg;
	msg = NULL;
}

