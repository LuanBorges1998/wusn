/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *
 ****************************************************************************/

#ifndef _DRONE_H_
#define _DRONE_H_

#include <omnetpp.h>

#include "node/resourceManager/ResourceManager.h"
#include "helpStructures/TimerService.h"
#include "helpStructures/CastaliaModule.h"
#include "node/mobilityManager/VirtualMobilityManager.h"

using namespace std;

class Drone: public CastaliaModule {
 private:

 protected:
    double x;
    double y;
    double z;
    double phi;
    double theta;

    bool occupied;

    int nodeNumber;
    double xNode;
    double yNode;
    double zNode;
    double distance;

    double droneLifetime;
    double transferingEnergyTime;
    double travelTime;

    double totalTravelsTime;
    double totalTravels;

    double goingTime;
    double backTime;
    double goingEnergy;
    double backEnergy;

    double speed;
    double initialEnergy;
    double consumption;
    double maxEnergyTransfer;
    double energyTransfer;

    virtual void initialize();
    virtual void handleMessage(cMessage * msg);
    virtual void finishSpecific();
 public:

};

#endif				//_DRONEMODULE_H_
