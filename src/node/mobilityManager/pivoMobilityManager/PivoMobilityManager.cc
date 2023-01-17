#include "PivoMobilityManager.h"

Define_Module(PivoMobilityManager);


void PivoMobilityManager::initialize()
{
	VirtualMobilityManager::initialize();

	updateInterval = par("updateInterval");
	updateInterval = updateInterval / 1000;
    x_field = network->par("field_x");
	y_field = network->par("field_y");
	z_field = network->par("field_z");
	drawEnergy = par("drawPowerMove");
	//droneOnPivo= par("droneOnPivo");
	step = par("speedPivo");
	pivo_speed = par("speedPivo");
	
	loc1_x = nodeLocation.x;
	loc1_y = nodeLocation.y;
	loc1_z = nodeLocation.z;

	speed = par("speed");

	direction = 1;

	if(loc1_x == 0 || loc1_x == x_field){

		setLocation(loc1_x, loc1_y, loc1_z);
		scheduleAt(simTime() + updateInterval,
			new MobilityManagerMessage("Periodic location update message", MOBILITY_PERIODIC));
		
	}
	
	
}


void PivoMobilityManager::handleMessage(cMessage * msg){
    int msgKind = msg->getKind();
	switch (msgKind) {

		case MOBILITY_PERIODIC:{
			//if (droneOnPivo && (nodeLocation.x <= x_field/2 && nodeLocation.y >= y_field/2) ) { // drone emcima do pivot por um quarto do field
			/* if (nodeLocation.x <= x_field/2 && nodeLocation.y >= y_field/2) { // drone emcima do pivot por um quarto do field
				if(nodeLocation.x < x_field &&  nodeLocation.y == y_field){ // aumentar o x
					if(nodeLocation.x + pivo_speed < x_field){
						nodeLocation.x+=pivo_speed; 
					}
					else{
						nodeLocation.x = x_field;
					}
				}
				else if(nodeLocation.x > 0  && nodeLocation.y == 0){ // diminui o x
					if(nodeLocation.x - pivo_speed > 0){
						nodeLocation.x-= pivo_speed;
					}
					else{
						nodeLocation.x = 0;
					}

				}
				else if(nodeLocation.x == x_field && nodeLocation.y > 0){ // diminui o y
					
					if(nodeLocation.y - pivo_speed > 0){
						nodeLocation.y -= pivo_speed;
					}else {
						nodeLocation.y =0;
					}

				}
				else if(nodeLocation.x == 0 && nodeLocation.y < y_field){ // aumentando o y
					if(nodeLocation.y + pivo_speed < y_field){
						nodeLocation.y += pivo_speed;
					}
					else{
						nodeLocation.y = y_field;
					}
				}

			} */
			/*else { */
				if(nodeLocation.x < x_field &&  nodeLocation.y == y_field){ // aumentar o x
				
					//powerDrawn(drawEnergy * step); // desconta energia somente se drone estive voando ou seja fora do pivo

					if(nodeLocation.x + step < x_field){
						nodeLocation.x+=step; // passo = 2
					}
					else{
						nodeLocation.x = x_field;
					}
				}
				else if(nodeLocation.x > 0  && nodeLocation.y == 0){ // diminui o x
					if(nodeLocation.x - step > 0){
						nodeLocation.x-= step;
					}
					else{
						nodeLocation.x = 0;
					}

				}
				else if(nodeLocation.x == x_field && nodeLocation.y > 0){ // diminui o y
					
					if(nodeLocation.y - step > 0){
						nodeLocation.y -= step;
					}else {
						nodeLocation.y =0;
					}

				}
				else if(nodeLocation.x == 0 && nodeLocation.y < y_field){ // aumentando o y
					if(nodeLocation.y + step < y_field){
						nodeLocation.y += step;
					}
					else{
						nodeLocation.y = y_field;
					}
				}
				

			/*}*/
			
			
			

		
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