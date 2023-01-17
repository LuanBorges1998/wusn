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

package node.communication.mac.mrip;

simple MRIP like node.communication.mac.iMac {
 parameters:
	bool collectTraceInfo = default (false);
	int macMaxPacketSize = default (0);		//BypassMac does not limit packet size
	int macPacketOverhead = default (8);	//only 2 fields are added in simple MAC frame, 8 bytes in total
	int macBufferSize = default (0);		//BypassMac does not use buffers

	//MRIP
	int attempts = default (0);
    int maxAttempts = default (5);
	// MAC address
	string address = default("auto");
	//How long the node sleeps
	double sleepInterval = default(1);
	//Time the node spends doing
	//CCA ( clear channel assertion )
	double ccaInterval = default(0.01);
	
	//Maximum time a node waits
	//for an ACK (acknowledgement)
	double dwellInterval = default(0.03);
	
	//Size of the RI=MAC Header
	int headerLength = default(10);
	
	double bitrate = default(19200);
	int mtu = default(0);
	string radioModule = default("^.radio");
	
	int phyFrameOverhead = default (6);
    double phyDataRate = default(250);
    
    double backoffBaseValue = default (16);	// the backoff base value in ms
	int backoffMaxAttempts = default (5);	

 gates:
	output toNetworkModule;
	output toRadioModule;
	input fromNetworkModule;
	input fromRadioModule;
	input fromCommModuleResourceMgr;
}

