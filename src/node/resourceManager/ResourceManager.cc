/*******************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2012                            *
 *  Developed at the ATP lab, Networked Systems research theme                 *
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev *
 *  This file is distributed under the terms in the attached LICENSE file.     *
 *  If you do not find this file, copies can be found by writing to:           *
 *                                                                             *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                *
 *      Attention:  License Inquiry.                                           *
 *                                                                             *  
 *******************************************************************************/

#include "node/resourceManager/ResourceManager.h"

Define_Module(ResourceManager);

void ResourceManager::initialize()
{
	sigmaCPUClockDrift = par("sigmaCPUClockDrift");
	//using the "0" rng generator of the ResourceManager module
	cpuClockDrift = normal(0, sigmaCPUClockDrift);
	/* Crop any values beyond +/- 3 sigmas. Some protocols (e.g., MAC) rely on
	 * bounded cpuClockDrift. Although the bounds are conservative (usually 3sigmas),
	 * if you instantiate thousands of nodes (in multiple runs) we will get a
	 * couple of nodes that will be beyond this bound. Limiting/Croping the drift
	 * is actually realistic, since usually there is some kind of quality
	 * control on quartz crystals or the boards that use them (sensor node)
	 */
	if (cpuClockDrift > 3 * sigmaCPUClockDrift)
		cpuClockDrift = 3 * sigmaCPUClockDrift;
	if (cpuClockDrift < -3 * sigmaCPUClockDrift)
		cpuClockDrift = -3 * sigmaCPUClockDrift;

	self = getParentModule()->getParentModule()->getIndex();

	initialEnergy = par("initialEnergy");
	ramSize = par("ramSize");
	baselineNodePower = par("baselineNodePower");
	periodicEnergyCalculationInterval = (double)par("periodicEnergyCalculationInterval") / 1000;

	if (baselineNodePower < 0 || periodicEnergyCalculationInterval < 0)
		throw cRuntimeError("Illegal values for baselineNodePower and/or periodicEnergyCalculationInterval in resource manager module");

	currentNodePower = baselineNodePower;
	remainingEnergy = initialEnergy;
	totalRamData = 0;
	disabled = true;

	sumEnergyReceived = 0;
	droneRequest = par("droneRequest");
	requestedEnergy = false;

	droneOccupied.resize(gateSize("toDrone"), false);
}

void ResourceManager::calculateEnergySpent()
{
	if (remainingEnergy > 0) {
		simtime_t timePassed = simTime() - timeOfLastCalculation;
		trace() << "energy consumed in the last " << timePassed << 
			"s is " <<(timePassed * currentNodePower);
		consumeEnergy(SIMTIME_DBL(timePassed * currentNodePower / 1000.0));
		timeOfLastCalculation = simTime();

		cancelEvent(energyMsg);
		scheduleAt(simTime() + periodicEnergyCalculationInterval, energyMsg);
	}
}

/* The ResourceManager module has only one "unconnected" port where it can receive messages that
 * update the power drawn by a module, or a NODE_STARTUP message. If disabled we still process
 * messages because we want to have the latest power drawn from any module.
 */
void ResourceManager::handleMessage(cMessage * msg)
{

	switch (msg->getKind()) {

		case NODE_STARTUP:{
			disabled = false;
			timeOfLastCalculation = simTime();
			energyMsg = new cMessage("Periodic energy calculation", TIMER_SERVICE);
        		scheduleAt(simTime() + periodicEnergyCalculationInterval, energyMsg);
			break;
		}
	
		case TIMER_SERVICE:{
			calculateEnergySpent();
			return;
		}

		case RESOURCE_MANAGER_DRAW_POWER:{
			ResourceManagerMessage *resMsg = check_and_cast<ResourceManagerMessage*>(msg);
			int id = resMsg->getSenderModuleId();
			double oldPower = storedPowerConsumptions[id];
			trace() << "New power consumption, id = " << id << ", oldPower = " << 
					currentNodePower << ", newPower = " << 
					currentNodePower - oldPower + resMsg->getPowerConsumed();
			if (!disabled)
				calculateEnergySpent();
			currentNodePower = currentNodePower - oldPower + resMsg->getPowerConsumed();
			storedPowerConsumptions[id] = resMsg->getPowerConsumed();
			break;
		}

		case DRONE_OCCUPIED:{
		    double timeToDroneDisoccupie = atof(msg->getName());
		    droneOccupied[droneRequest] = true;
		    scheduleAt(simTime() + timeToDroneDisoccupie, new cMessage(to_string(droneRequest).c_str(), DRONE_OCCUPIED_TIMEOUT));

		    scheduleAt(simTime() + 0.1, new cMessage("REQUEST_ENERGY", REQUEST_ENERGY));
		    break;
		}

		case DRONE_OCCUPIED_TIMEOUT:{
		    int droneDisoccupie = atoi(msg->getName());
		    droneOccupied[droneDisoccupie] = false;
		}

		case REQUEST_ENERGY:{
		    double timeToSendRequestEnergy = 0;
		    double timeToDisoccupie = 0;
		    int numDrones = gateSize("toDrone");
            if(numDrones > 0){
                int count = 0;
                while(numDrones > count){
                    if(droneOccupied[count]){
                        cMessageHeap& heap = cSimulation::getActiveSimulation()->getMessageQueue();
                        cMessageHeap::Iterator it(heap);
                        do{
                            cMessage * event = it();
                            if(event->isSelfMessage()){
                                if((int)event->getKind() == DRONE_OCCUPIED_TIMEOUT && atoi(event->getName())==count){
                                    timeToDisoccupie = event->getArrivalTime().dbl() - event->getCreationTime().dbl();
                                    if(timeToDisoccupie<timeToSendRequestEnergy || timeToSendRequestEnergy==0){
                                        timeToSendRequestEnergy = timeToDisoccupie + 0.1;
                                        cancelEvent(new cMessage("REQUEST_ENERGY", REQUEST_ENERGY));
                                        scheduleAt(simTime() + timeToSendRequestEnergy, new cMessage("REQUEST_ENERGY", REQUEST_ENERGY));
                                    }
                                }
                            }
                            it++;
                        }while (!it.end());
                    }else{
                        requestedEnergy = true;
                        droneRequest = count;
                        droneOccupied[count] = true;
                        cancelEvent(new cMessage("REQUEST_ENERGY", REQUEST_ENERGY));
                        send(new cMessage(to_string(self).c_str(), REQUEST_ENERGY), "toDrone", droneRequest);
                        break;
                    }
                    count += 1;
                }



                /*if(numDrones > droneRequest + 1){
                    droneRequest += 1;
                    send(new cMessage("REQUEST_ENERGY", REQUEST_ENERGY), "toDrone", droneRequest);
                }else{
                    droneRequest = 0;
                    send(new cMessage("REQUEST_ENERGY", REQUEST_ENERGY), "toDrone", droneRequest);
                }*/
            }
            break;
		}

		case RECEIVE_ENERGY:{
		    requestedEnergy = false;
		    double energyReceived = atof(msg->getName());
		    receiveEnergy(energyReceived);
		    //droneRequest = 0;
            break;
        }

		default:{
			throw cRuntimeError("ERROR: Unexpected message received by resource manager: %s", msg->getKind());
		}
	}
	delete msg;
}

void ResourceManager::finishSpecific()
{
	calculateEnergySpent();
	declareOutput("Consumed Energy");
	collectOutput("Consumed Energy", "", initialEnergy + sumEnergyReceived - remainingEnergy);
	declareOutput("Remaining Energy");
	collectOutput("Remaining Energy", "", remainingEnergy);

	declareOutput("Received Energy");
	collectOutput("Received Energy", "", sumEnergyReceived);

	if (getParentModule()->getIndex() == 0) {
		cTopology *topo;	// temp variable to access energy spent by other nodes
		topo = new cTopology("topo");
		topo->extractByNedTypeName(cStringTokenizer("node.Node").asVector());

		double minLifetime = estimateLifetime();
		for (int i = 1; i < topo->getNumNodes(); i++) {
			ResourceManager *resMng = dynamic_cast<ResourceManager*>
				(topo->getNode(i)->getModule()->getSubmodule("ResourceManager"));
			if (minLifetime > resMng->estimateLifetime()) 
				minLifetime = resMng->estimateLifetime();
		}
		declareOutput("Estimated network lifetime (days)");
		collectOutput("Estimated network lifetime (days)", "", minLifetime);
		delete(topo);
	}
}

double ResourceManager::estimateLifetime(void) 
{
	return ((initialEnergy * simTime().dbl()) / ((initialEnergy - remainingEnergy) * 86400.0));
}

double ResourceManager::getSpentEnergy(void)
{
	Enter_Method("getSpentEnergy()");
	return (initialEnergy - remainingEnergy);
}

double ResourceManager::getCPUClockDrift(void)
{
	Enter_Method("getCPUClockDrift(void)");
	return (1.0f + cpuClockDrift);
}

void ResourceManager::consumeEnergy(double amount)
{
	Enter_Method("consumeEnergy(double amount)");

	if (remainingEnergy <= amount) {
		remainingEnergy = 0;
		send(new cMessage("Destroy node message", OUT_OF_ENERGY), "toSensorDevManager");
		send(new cMessage("Destroy node message", OUT_OF_ENERGY), "toApplication");
		send(new cMessage("Destroy node message", OUT_OF_ENERGY), "toNetwork");
		send(new cMessage("Destroy node message", OUT_OF_ENERGY), "toMac");
		send(new cMessage("Destroy node message", OUT_OF_ENERGY), "toRadio");
        disabled = true;
        declareOutput("Dead Node");
		collectOutput("Dead Node", "yes?", 1);
		collectOutput("Dead Node", "time", SIMTIME_DBL(simTime()));
	} else
		remainingEnergy -= amount;
}

bool ResourceManager::isNeedEnergy()
{
    Enter_Method("isNeedEnergy()");
    if(remainingEnergy < initialEnergy)
        return true;
    else
        return false;
}

bool ResourceManager::requestEnergy()
{
    return requestedEnergy;
}

void ResourceManager::needEnergy()
{
    Enter_Method("needEnergy()");
    int numDrones = gateSize("toDrone");
    //setTimer(REQUEST_ENERGY, 0);
    //send(new cMessage("REQUEST_ENERGY", REQUEST_ENERGY), "toDrone", droneRequest);
    //if((numDrones>0) && (droneRequest == 0))
    if(numDrones>0)
        scheduleAt(simTime(), new cMessage("REQUEST_ENERGY", REQUEST_ENERGY));
    //else
    //    scheduleAt(simTime(), new cMessage("DRONE_OCCUPIED", DRONE_OCCUPIED));
}

void ResourceManager::receiveEnergy(double amount)
{
    Enter_Method("receiveEnergy(double amount)");

    if (remainingEnergy + amount > initialEnergy) {
        double tempCosumedEnergy = initialEnergy-remainingEnergy;
        //int tempIntConsumedEnergy = tempCosumedEnergy * 100;
        sumEnergyReceived = sumEnergyReceived + tempCosumedEnergy;// / 100;
        remainingEnergy = initialEnergy;
    } else{
        sumEnergyReceived += amount;
        remainingEnergy += amount;
    }

    double timeReceivingEnergy = amount / 0.055; //considering 10 cm depth, 55 mW transfer
    send(new cMessage(to_string(timeReceivingEnergy).c_str(), RECEIVE_ENERGY), "toMac");
}

void ResourceManager::destroyNode(void)
{
	Enter_Method("destroyNode(void)");

	send(new cMessage("Destroy node message", DESTROY_NODE), "toSensorDevManager");
	send(new cMessage("Destroy node message", DESTROY_NODE), "toApplication");
	send(new cMessage("Destroy node message", DESTROY_NODE), "toNetwork");
	send(new cMessage("Destroy node message", DESTROY_NODE), "toMac");
	send(new cMessage("Destroy node message", DESTROY_NODE), "toRadio");
	disabled = true;
    declareOutput("Dead Node");
	collectOutput("Dead Node", "yes?", 1);
	collectOutput("Dead Node", "time", SIMTIME_DBL(simTime()));
}

int ResourceManager::RamStore(int numBytes)
{
	Enter_Method("RamStore(int numBytes)");

	int ramHasSpace = ((totalRamData + numBytes) <= ramSize) ? 1 : 0;
	if (!ramHasSpace) {
		trace() << "\n[Resource Manager] t= " << simTime() <<
				": WARNING: Data not stored to Ram. Not enough space to store them.";
		return 0;
	} else
		totalRamData += numBytes;
	return 1;
}

void ResourceManager::RamFree(int numBytes)
{
	Enter_Method("RamFree(int numBytes)");
	totalRamData -= numBytes;
	totalRamData = (totalRamData < 0) ? 0 : totalRamData;
}

