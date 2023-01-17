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

#include "node/communication/mac/swptmac/SWPTMAC.h"

Define_Module(SWPTMAC);

void SWPTMAC::initialize(){
    VirtualMac::initialize();

    phyDataRate = par("phyDataRate");
    phyLayerOverhead = par("phyFrameOverhead");

    attempts = par("attempts");
    maxAttempts = par("maxAttempts");

    bitrate = par("bitrate");
    ctrlFrameLength = par("headerLength");
    sleepInterval = par("sleepInterval");
    ccaInterval = par("ccaInterval");
    dwellInterval = par("dwellInterval");

    macState = SLEEP1;

    wakeup = new cMessage("Wakeup", WAKEUP);
    ccaTimeout = new cMessage("CCA Timeout", CCA_TIMEOUT);
    dwellTimeout = new cMessage("Dwell Time", DWELL_TIMEOUT);
    ackTimeout = new cMessage("Ack timeout", ACK_TIMEOUT);
    beaconTimeout = new cMessage("Beacon Timeout", BEACON_TIMEOUT);
    beaconReceived = new cMessage("Beacon Received", BEACON_RECEIVED);
    pushedQueue = new cMessage("Pushed queue", PUSH_QUEUE);

    isChannelClearBool = true;

    energyArrived = false;
    timeReceivingEnergy = 0;

    backoffBaseValue = ((double)par("backoffBaseValue")) / 1000.0;  // convert msecs to secs
    backoffMaxAttempts = ((int)par("backoffMaxAttempts"));
    backoffTimes = 0;
}

void SWPTMAC::startup(){
    //setTimer(WAKEUP, sleepInterval);
    setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
}

void SWPTMAC::timerFiredCallback(int event){
    if(event==RECEIVED_ENERGY){
        energyArrived = false;
        setTimer(WAKEUP, 0);
    }else if(event==WAKEUP){
        cancelTimer(WAKEUP);
        setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
    }
    switch(macState){
        //Radio Off
        case SLEEP1:
            if(event == WAKEUP){
                setState(BEACON_CCA1, event);
                setTimer(CCA_TIMEOUT, ccaInterval * (1 + dblrand()));
            }else if(event == RECEIVE_ENERGY){
                setState(RECEIVING_ENERGY, event);
                setTimer(RECEIVE_ENERGY, 0);
            }
            break;
        //Radio receiving
        case BEACON_CCA1:
            if(event == CCA_TIMEOUT){
                if(!isChannelClear()){
                    cancelTimer(CCA_TIMEOUT);
                    setTimer(CCA_TIMEOUT, ccaInterval * (1 + dblrand()));
                    return;
                }
                sendBeacon(false);
            }
            break;
        case DATA_CCA:
            if(event == BEACON_RECEIVED){
                if(!isChannelClear()){
                    attempts = attempts + 1;
                    if (attempts == maxAttempts){
                        attempts = 0;
                        cancelTimer(CCA_TIMEOUT);
                        cancelTimer(WAKEUP);
                        setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
                        break;
                    }
                    cancelTimer(CCA_TIMEOUT);
                    setTimer(CCA_TIMEOUT, ccaInterval * (1 + dblrand()));
                    return;
                }
                setState(WAIT_DATA_OVER, event);
                cancelTimer(CCA_TIMEOUT);
                setTimer(CCA_TIMEOUT, 0);
            }
            break;
        //Radio transmitting
        case WAIT_BEACON_OVER:
            if(event == TRANSMISSION_ENDED){
                setState(WAIT_DATA, event);
                setTimer(BEACON_TIMEOUT, 1);
            }
            break;
        case WAIT_DATA_OVER:
            if(event == CCA_TIMEOUT){
                if (!txQueue.empty()) {
                    MacPacket *macFrame = txQueue.front();
                    txQueue.pop();
                    sendToRadioLayer(macFrame);
                    setState(WAIT_ACK, event);
                    setTimer(DWELL_TIMEOUT, dwellInterval);
                }
            }
            break;
        case WAIT_ACK_OVER:
            /*if(event != DWELL_TIMEOUT){
                cancelTimer(DWELL_TIMEOUT);
                setTimer(DWELL_TIMEOUT, dwellInterval);
                return;
            }*/

            if(event == TRANSMISSION_ENDED){
                setState(WAIT_DATA, 0);

                //Inicio calculo tempo de transmissao
                cPacket *pkt = new cPacket();
                MacPacket *macFrame;
                macFrame = new MacPacket("BEACON", MAC_LAYER_PACKET);
                encapsulatePacket(macFrame, pkt);
                macFrame->setSource(SELF_MAC_ADDRESS);
                macFrame->setDestination(BROADCAST_MAC_ADDRESS);
                simtime_t txTime = TX_TIME(pkt->getByteLength());
                //Fim calculo tempo de transmissao

                cancelTimer(DWELL_TIMEOUT);
                setTimer(DWELL_TIMEOUT, txTime + dwellInterval);
            }

            //Solicitando energia do drone - STATE == TESTE_ENERGY_LEVEL
            if(energyArrived){
                setState(RECEIVING_ENERGY, event);
                setTimer(RECEIVE_ENERGY, 0);
                break;
            }else if(isNeedEnergy()){
                if(!requestEnergy()){
                    //setState(RECEIVING_ENERGY, event);
                    //setTimer(REQUEST_ENERGY1, 0);
                    needEnergy();
                    //break;
                }
            }

            //setState(SLEEP1, event);
            break;
        case WAIT_RESPONSE_OVER:
            break;
        //Radio receiving
        case WAIT_DATA:
            /*if(event == BEACON_TIMEOUT){
                setState(SLEEP1, event);
            }*/
            /*if(event == BEACON_TIMEOUT){
                //setState(SLEEP1, event);
                if(txQueue.empty()){
                    setState(SLEEP1, event);
                }else{
                    setState(WAIT_BEACON, event);
                }
            }*/

            if(event == BEACON_TIMEOUT){
                if(txQueue.empty()){
                    setState(SLEEP1, event);
                }else{
                    setState(WAIT_BEACON, event);
                }
            }else if(event == DWELL_TIMEOUT){
                if(txQueue.empty()){
                    setState(SLEEP1, event);
                }else{
                    setState(WAIT_BEACON, event);
                }
            }
            break;
        case WAIT_ACK:
            if(event == BEACON_RECEIVED){ //BEACON==ACK
                //Solicitando energia do drone - STATE == TESTE_ENERGY_LEVEL
                if(energyArrived){
                    setState(RECEIVING_ENERGY, event);
                    setTimer(RECEIVE_ENERGY, 0);
                    break;
                }else if(isNeedEnergy()){
                    if(!requestEnergy()){
                        //setState(RECEIVING_ENERGY, event);
                        //setTimer(REQUEST_ENERGY1, 0);
                        needEnergy();
                        //break;
                    }
                }

                if (!txQueue.empty()) {
                    setState(WAIT_DATA_OVER, event);
                    setTimer(CCA_TIMEOUT, 0);
                }else {
                    setState(SLEEP1, event);
                }
            }else if(event == DWELL_TIMEOUT){
                //Solicitando energia do drone - STATE == TESTE_ENERGY_LEVEL
                if(energyArrived){
                    setState(RECEIVING_ENERGY, event);
                    setTimer(RECEIVE_ENERGY, 0);
                    break;
                }else if(isNeedEnergy()){
                    if(!requestEnergy()){
                        //setState(RECEIVING_ENERGY, event);
                        //setTimer(REQUEST_ENERGY1, 0);
                        needEnergy();
                        //break;
                    }
                }

                setState(WAIT_DATA_OVER, event);
                setTimer(CCA_TIMEOUT, 0);
            }
            break;
        case WAIT_BEACON:
            if(event == BEACON_RECEIVED){
                setState(DATA_CCA, event);
                setTimer(BEACON_RECEIVED, ccaInterval);
            }else if(event == WAIT_BEACON_TIMEOUT){
                setState(SLEEP_WAIT_BEACON, event);
                //cancelTimer(WAKEUP);
                //setTimer(WAKEUP, sleepInterval);
                cancelTimer(WAKEUP_WAIT_BEACON);
                setTimer(WAKEUP_WAIT_BEACON, 2.5);
            }else if(event == RECEIVE_ENERGY){
                setState(RECEIVING_ENERGY, event);
                setTimer(RECEIVE_ENERGY, 0);
            }
            break;
        case SLEEP_WAIT_BEACON:
            if(event == WAKEUP_WAIT_BEACON){
                cancelTimer(WAIT_BEACON_TIMEOUT);
                setTimer(WAIT_BEACON_TIMEOUT, 1.5);
                setState(WAIT_BEACON, 0);
                //cancelTimer(WAKEUP);
                //setTimer(WAKEUP, sleepInterval * 0.49999);
            }else if(event == RECEIVE_ENERGY){
                setState(RECEIVING_ENERGY, event);
                setTimer(RECEIVE_ENERGY, 0);
            }
            break;
        case RECEIVING_ENERGY:
            //if(event == REQUEST_ENERGY1){
            //    needEnergy();
            //}
            if(RECEIVE_ENERGY){
                //double timeReceivingEnergy = atof(msg->getName());
                setState(SLEEP1, 0);
                setTimer(RECEIVED_ENERGY, timeReceivingEnergy);
                break;
            }
            break;
    }
}

void SWPTMAC::receiveEnergy(cMessage * msg)
{

    int msgKind = (int)msg->getKind();

    if (disabled && msgKind != NODE_STARTUP) {
        delete msg;
        return;
    }

    switch (msgKind) {
        case RECEIVE_ENERGY:{
            //double timeReceivingEnergy = atof(msg->getName());
            //setState(SLEEP1, 0);
            //setTimer(WAKEUP, timeReceivingEnergy);
            //setTimer(WAKEUP, 0);

            energyArrived = true;
            timeReceivingEnergy = atof(msg->getName());

            switch(macState){
                    case SLEEP1:
                        setTimer(RECEIVE_ENERGY, 0);
                        break;
                    case BEACON_CCA1:
                        break;
                    case DATA_CCA:
                        //ok
                        break;
                    case WAIT_BEACON_OVER:
                        break;
                    case WAIT_DATA_OVER:
                        //ok
                        break;
                    case WAIT_ACK_OVER:
                        break;
                    case WAIT_RESPONSE_OVER:
                        break;
                    case WAIT_DATA:
                        break;
                    case WAIT_ACK:
                        //aqui ok e verifica no timer se energyArrived
                        break;
                    case WAIT_BEACON:
                        setTimer(RECEIVE_ENERGY, 0);
                        break;
                    case SLEEP_WAIT_BEACON:
                        setTimer(RECEIVE_ENERGY, 0);
                        break;
                    case RECEIVING_ENERGY:
                        break;
                }

            //receiveEnergy = true;
            //break;
        }
    }
}

void SWPTMAC::setState(SWPTMACState newState, int event){
    switch(newState){
        case SLEEP1:
        case SLEEP_WAIT_BEACON:
            toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
            break;
        case WAIT_BEACON_OVER:
        case WAIT_DATA_OVER:
        case WAIT_ACK_OVER:
        case WAIT_RESPONSE_OVER:
            toRadioLayer(createRadioCommand(SET_STATE, TX));
            break;
        case INIT:
        case BEACON_CCA1:
        case WAIT_DATA:
        case WAIT_BEACON:
        case WAIT_ACK:
            toRadioLayer(createRadioCommand(SET_STATE, RX));
            break;
    }
    macState = newState;
}

void SWPTMAC::sendBeacon(bool withBW){
    cPacket *pkt = new cPacket();
    MacPacket *macFrame;
    if(withBW){
        backoffTimes = backoffTimes + 1;
        macFrame = new MacPacket("BEACON-BW", MAC_LAYER_PACKET);
    }else{
        macFrame = new MacPacket("BEACON", MAC_LAYER_PACKET);
    }
    encapsulatePacket(macFrame, pkt);
    macFrame->setSource(SELF_MAC_ADDRESS);
    macFrame->setDestination(BROADCAST_MAC_ADDRESS); //Broadcast????
    toRadioLayer(macFrame);
    setState(WAIT_BEACON_OVER, 0);
    simtime_t txTime = TX_TIME(pkt->getByteLength());
    setTimer(TRANSMISSION_ENDED, txTime);
}

bool SWPTMAC::isChannelClear(){
    toRadioLayer(createRadioCommand(SET_STATE, RX));
    int CCAcode = radioModule->isChannelClear();
    if (CCAcode == CLEAR) {
        return true;
    }else {
        return true;
    }
}

/* Handle packet received from upper (network) layer.
 * We need to create a MAC packet, (here it can just be the generic MacPacket)
 * and encapsulate the received network packet before forwarding it to RadioLayer
 */
void SWPTMAC::fromNetworkLayer(cPacket * pkt, int destination)
{
    if(destination == SELF_MAC_ADDRESS)
        return;
    MacPacket *macFrame = new MacPacket("Routing packet", MAC_LAYER_PACKET);
    encapsulatePacket(macFrame, pkt);
    macFrame->setSource(SELF_MAC_ADDRESS);
    macFrame->setDestination(destination);
    txQueue.push(macFrame);
    //setState(WAIT_BEACON, 0);
    //if(macState == SLEEP1){
        //O que caracteriza o MRIP
        cancelTimer(WAIT_BEACON_TIMEOUT);
        setTimer(WAIT_BEACON_TIMEOUT, sleepInterval * 1.5);
        //////////////////////////
        setState(WAIT_BEACON, 0);
    //}
}

void SWPTMAC::sendToRadioLayer(MacPacket *macFrame){
    toRadioLayer(macFrame);
    toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/* Handle packet received from lower (radio) layer.
 * We accept packets from all MAC protocols (cast to the base class MacPacket)
 * Then we filter by the destination field. By default we set the generic
 * destination field to broadcast when we encapsulate a NET packet. If a
 * specific protocol does not change that field then bypassMAC will be
 * operating in a promiscuous mode.
 */
void SWPTMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi)
{
    MacPacket *macPkt = dynamic_cast <MacPacket*>(pkt);
    string macName = macPkt->getName();
    if(macName=="INTERFERENCE"){
        if(backoffTimes == backoffMaxAttempts){
            setState(SLEEP1, 0);
            backoffTimes = 0;
            return;
        }
        sendBeacon(true);
        return;
    }
    if(macPkt == NULL)
        return;
    //Personalizado por Luan
    if(macName=="BEACON"){
        if(hasPacketToSent(macPkt)){
            cancelTimer(BEACON_RECEIVED);
            setTimer(BEACON_RECEIVED, 0);
        }
    }else if(macName=="BEACON-BW"){
        if(hasPacketToSent(macPkt)){
            double backoffTimer = 0;
            //cálculo do backoff exponencial
            backoffTimer = backoffBaseValue *
                                        pow(2.0, (double)(((backoffTimes - 1) < 0) ? 0 : backoffTimes - 1));
            backoffTimer = getRNG(1)->doubleRand() * backoffTimer;
            cancelTimer(BEACON_RECEIVED);
            setTimer(BEACON_RECEIVED, backoffTimer);
        }
    }else if(macPkt->getDestination() == SELF_MAC_ADDRESS ||
        macPkt->getDestination() == BROADCAST_MAC_ADDRESS)
    {
        cancelTimer(BEACON_TIMEOUT);
        toNetworkLayer(decapsulatePacket(macPkt));
        sendBeacon(false);
        //???
        setState(WAIT_ACK_OVER, 0);
        cancelTimer(TRANSMISSION_ENDED);
        simtime_t txTime = TX_TIME(pkt->getByteLength());
        setTimer(TRANSMISSION_ENDED, txTime);
    }
}

bool SWPTMAC::hasPacketToSent(MacPacket * macFrame){
    if(txQueue.size()>0){
        MacPacket *macFrameQueued = txQueue.front();
        if(macFrame->getSource()==macFrameQueued->getDestination())
            return true;
    }else{
        return false;
    }
}

void SWPTMAC::finish(){

}