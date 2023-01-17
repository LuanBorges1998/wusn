//****************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010                         *
//*  Developed at the ATP lab, Networked Systems research theme              *
//*  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
//*  This file is distributed under the terms in the attached LICENSE file.  *
//*  If you do not find this file, copies can be found by writing to:        *
//*                                                                          *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
//*      Attention:  License Inquiry.                                        *
//*                                                                          *  
//****************************************************************************/

package drone;

simple Drone {
 parameters:
   	//drone location is defined by five parameters below
	double x = default (0);
	double y = default (0);
	double z = default (0);
	double phi = default (0);
	double theta = default (0);
	
	bool occupied = default(false);
     
     double speed = default (11); //m/s	
     double initialEnergy = default (321206.4); //drone phantom 4 battery
     double consumption = default(178.448); //watts

 gates:
	output toNode[];
	input fromNode[];

}

