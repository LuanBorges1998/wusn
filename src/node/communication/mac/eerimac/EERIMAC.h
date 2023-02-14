/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2011                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev, Athanassios Boulis                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *
 ****************************************************************************/

#ifndef _EERIMAC_H_
#define _EERIMAC_H_

#include <omnetpp.h>
#include "node/communication/mac/VirtualMac.h"

#define TX_TIME(x) (phyLayerOverhead + x)*1/(1000*phyDataRate/8.0)  //x are in BYTES

using namespace std;

enum EERIMACEvent{
	WAKEUP = 1, // Time to wake up
	CCA_TIMEOUT = 2, // CCA period ended
	ACK_TIMEOUT = 3, // ACK not received in time
	DWELL_TIMEOUT = 4, // The node has received a beacon ,
	// but the data has not arrived
	// within the expected timeframe
	BEACON_TIMEOUT = 5, // Beacon not received in time
	TRANSMISSION_ENDED = 6, // Current transmission ended
	BEACON_RECEIVED = 7, // Beacon was received
	DATA_RECEIVED = 8, // Data was received

	//Personalizado por Luan
    PUSH_QUEUE = 9,
    WAIT_BEACON_TIMEOUT = 10,
    WAKEUP_WAIT_BEACON = 11,
    ACK_RECEIVED = 12,
    BEACON_BW_RECEIVED = 13
};

enum EERIMACType {
	RIMAC_DATA = 1,
	RIMAC_BEACON = 2
};

enum EERIMACState{
    SLEEP1, // Sleep
	// RADIO OFF/ Sleeping
	INIT, // I n i t i a l i z e protocol

	// Radio receiving
	BEACON_CCA1, // Wait channel to be fr e e to send beacon
	DATA_CCA, // Wait channel to be fr e e to send data

	// Radio transmitting
	WAIT_BEACON_OVER, // Wait beacon transmission to be over
	WAIT_DATA_OVER, // Wait data transmission to be over
	WAIT_ACK_OVER, // Wait ACK transmission to be over
	WAIT_RESPONSE_OVER, // Wait beacon=on=request transmission
	// to be over

	// Radio receiving
	WAIT_DATA, // Wait fo r data to a r r i ve
	WAIT_ACK, // Wait ACK beacon to a r r i ve
	WAIT_BEACON, // Wait normal beacon to a r r i ve

	SLEEP_WAIT_BEACON
};


class EERIMAC: public VirtualMac
{
    bool collectTraceInfo;
    int macMaxPacketSize;     //BypassMac does not limit packet size
    int macPacketOverhead;    //only 2 fields are added in simple MAC frame, 8 bytes in total
    int macBufferSize;        //BypassMac does not use buffers

    double backoffBaseValue;// the backoff value
    int backoffTimes;   // number of consequtive backoff times
    int backoffMaxAttempts;

    int phyLayerOverhead;
    double phyDataRate;

    //RIMAC
    EERIMACState macState;

    //How long the node sleeps
    double sleepInterval;
    //Time the node spends doing
    //CCA ( clear channel assertion )
    double ccaInterval;
    cMessage* ccaTimeout;

    //Maximum time a node waits
    //for an ACK (acknowledgement)
    double dwellInterval;
    cMessage* dwellTimeout;

    int attempts;
    int maxAttempts;
    double ctrlFrameLength;

    bool isChannelClearBool;

    cMessage* wakeup;
    cMessage* ackTimeout;
    cMessage* beaconTimeout;
	cMessage* beaconReceived;
	cMessage* pushedQueue;

    double bitrate;

	queue< MacPacket* > txQueue;

    //Initialization of the module and some variables
    void initialize();

    //Delete all dynamically allocated objects of the module
    void finish();

    void sendBeacon(int, int);
    bool isChannelClear();
    void setState(EERIMACState newState, int event);

    void timerFiredCallback(int);

	/* In order to create a MAC based on VirtualMacModule, we need to define only two
	 * functions: one to handle a packet received from the layer above (routing),
	 * and one to handle a packet from the layer below (radio)
	 */
	protected:
		void fromRadioLayer(cPacket *, double, double);
		void fromNetworkLayer(cPacket *, int);
		void receiveEnergy(cMessage *);

		virtual void startup();

		//Personalisado por Luan
		void sendToRadioLayer(MacPacket *);
		bool hasPacketToSent(MacPacket *);
};

class EERIMACHeaderBase{// extends FieldsChunk{
	int srcAddr;
	int destAddr;
	EERIMACType type;
};

class EERIMACBeaconFrame: EERIMACHeaderBase { };

class EERIMACDataFrame: EERIMACHeaderBase{
 	int networkProtocol = -1;
};

#endif
