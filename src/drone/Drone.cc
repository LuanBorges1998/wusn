/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2011                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *
 ****************************************************************************/

#include "drone/Drone.h"

Define_Module(Drone);

void Drone::initialize()
{
    x = par("x");
    y = par("y");
    z = par("z");
    phi = par("phi");
    theta = par("theta");

    occupied = par("occupied");
    totalTravels = 0;
    totalTravelsTime = 0;

    speed = par("speed");
    initialEnergy = par("initialEnergy");
    consumption = par("consumption");
}

void Drone::handleMessage(cMessage * msg){

    int msgKind = (int)msg->getKind();

    switch (msgKind) {

        case REQUEST_ENERGY:{
            nodeNumber = msg->getArrivalGate()->getIndex();

            if(occupied){
                double timeToDisoccupie;
                cMessageHeap& heap = cSimulation::getActiveSimulation()->getMessageQueue();
                cMessageHeap::Iterator it(heap);
                do{
                    cMessage * event = it();
                    if((int)event->getKind() == DRONE_OCCUPIED_TIMEOUT){
                        timeToDisoccupie = event->getArrivalTime().dbl() - event->getCreationTime().dbl();
                    }
                    it++;
                }while (!it.end());
                send(new cMessage(to_string(timeToDisoccupie).c_str(), DRONE_OCCUPIED), "toNode", nodeNumber);
                break;
            }

            VirtualMobilityManager *nodeMobility = check_and_cast<VirtualMobilityManager*>(getParentModule()->getSubmodule("node",nodeNumber)->getSubmodule("MobilityManager"));
            xNode = (double)nodeMobility->getLocation().x;
            yNode = (double)nodeMobility->getLocation().y;
            zNode = (double)nodeMobility->getLocation().z;

            distance = sqrt(pow(x - xNode, 2) + pow(y - yNode, 2) +
                     pow(z - zNode, 2));

            goingTime = distance / speed;
            backTime = goingTime;
            droneLifetime = initialEnergy / consumption;
            transferingEnergyTime = droneLifetime - goingTime - backTime;

            goingEnergy = goingTime * consumption;
            backEnergy = backTime * consumption;

            maxEnergyTransfer = initialEnergy - (goingEnergy + backEnergy);
            energyTransfer = transferingEnergyTime * 0.055; //considering 10 cm depth, 55 mW transfer
            if(energyTransfer > maxEnergyTransfer)
                energyTransfer = maxEnergyTransfer;

            //set precision energyTransfer
            int tempIntEntergyTransfer = energyTransfer * 100;
            energyTransfer = tempIntEntergyTransfer / 100;

            travelTime = goingTime+backTime+transferingEnergyTime;

            occupied = true;
            scheduleAt((simTime() + travelTime), new cMessage("DRONE_OCCUPIED_TIMEOUT", DRONE_OCCUPIED_TIMEOUT));

            send(new cMessage(to_string(energyTransfer).c_str(), RECEIVE_ENERGY), "toNode", nodeNumber);
            break;
        }

        case DRONE_OCCUPIED_TIMEOUT:{
            totalTravels += 1;
            totalTravelsTime += travelTime;

            travelTime = 0;
            occupied = false;
            break;
        }
    }
}

void Drone::finishSpecific(){
    declareOutput("Total Travels");
    collectOutput("Total Travels", "", totalTravels);

    declareOutput("Total Time Traveling");
    collectOutput("Total Time Traveling", "", totalTravelsTime);
}


