#ifndef _MOBILITYMODULE_H_
#define _MOBILITYMODULE_H_

#include "MobilityManagerMessage_m.h"
#include "VirtualMobilityManager.h"

using namespace std;

class PivoMobilityManager: public VirtualMobilityManager {
 private:
	/*--- The .ned file's parameters ---*/
	double updateInterval;
	double loc1_x;
	double loc1_y;
	double loc1_z;
	double loc2_x;
	double loc2_y;
	double loc2_z;
	double speed;
	int    step;

	/*--- Custom class parameters ---*/
	double incr_x;
	double incr_y;
	double incr_z;
	double distance;
    int direction;
    int x_field;
    int y_field;
    int z_field;
	int totalSNnodes;
	double drawEnergy;
	/*bool droneOnPivo;*/
	double pivo_speed;
 protected:
	void initialize();
	void handleMessage(cMessage * msg);
};

#endif
