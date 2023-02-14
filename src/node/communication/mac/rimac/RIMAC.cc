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

#include "node/communication/mac/rimac/RIMAC.h"

Define_Module(RIMAC);

void RIMAC::initialize(){
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

    backoffBaseValue = ((double)par("backoffBaseValue")) / 1000.0;  // convert msecs to secs
    backoffMaxAttempts = ((int)par("backoffMaxAttempts"));
    backoffTimes = 0;

    packetsSent = 0;
    acksReceived = 0;
}

void RIMAC::startup(){
    setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
}

void RIMAC::timerFiredCallback(int event){
    if(event==WAKEUP){
        cancelTimer(WAKEUP);
        setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
    }
    switch(macState){
        //Radio Off
        case SLEEP1:
            if(event == WAKEUP){
                setState(BEACON_CCA1, event);
                setTimer(CCA_TIMEOUT, ccaInterval * (1 + dblrand()));
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
                sendBeacon(0, -1);
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
                attempts = 0;
                setState(WAIT_DATA_OVER, event);
                cancelTimer(CCA_TIMEOUT);
                setTimer(CCA_TIMEOUT, 0);
            }
            break;
        //Radio transmitting
        case WAIT_BEACON_OVER:
            if(event == TRANSMISSION_ENDED){
                //Inicio calculo tempo de transmissao
                cPacket *pkt = new cPacket();
                MacPacket *macFrame;
                macFrame = new MacPacket("BEACON", MAC_LAYER_PACKET);
                encapsulatePacket(macFrame, pkt);
                macFrame->setSource(SELF_MAC_ADDRESS);
                macFrame->setDestination(BROADCAST_MAC_ADDRESS);
                simtime_t txTime = TX_TIME(pkt->getByteLength());
                //Fim calculo tempo de transmissao

                setState(WAIT_DATA, event);
                setTimer(BEACON_TIMEOUT, dwellInterval);
            }
            break;
        case WAIT_DATA_OVER:
            if(event == TRANSMISSION_ENDED){
                setState(WAIT_ACK, event);
                setTimer(DWELL_TIMEOUT, dwellInterval);
            }else if(radioModule->empty()){
                if(event == CCA_TIMEOUT){
                    if (!txQueue.empty()) {
                        MacPacket *macFrame = txQueue.front();
                        sendToRadioLayer(macFrame->dup());
                        packetsSent++;
                        simtime_t txTime = TX_TIME(macFrame->getByteLength());
                        setTimer(TRANSMISSION_ENDED, txTime);
                    }
                }else if(event == DWELL_TIMEOUT){
                    attempts = attempts + 1;
                    if(attempts == maxAttempts){
                        attempts = 0;
                        cancelTimer(CCA_TIMEOUT);
                        cancelTimer(WAKEUP);
                        setTimer(WAKEUP, sleepInterval * (0.5 + dblrand()));
                        break;
                    }else{
                        if (!txQueue.empty()) {
                            MacPacket *macFrame = txQueue.front();
                            sendToRadioLayer(macFrame->dup());//cria um novo pacote para enviar
                            setState(WAIT_ACK, event);
                            setTimer(DWELL_TIMEOUT, dwellInterval);
                        }
                    }
                }
            }else{
                setState(WAIT_ACK, event);
                setTimer(DWELL_TIMEOUT, dwellInterval);
            }
            break;
        case WAIT_ACK_OVER:
            /*if(event != DWELL_TIMEOUT){
                cancelTimer(DWELL_TIMEOUT);
                setTimer(DWELL_TIMEOUT, dwellInterval);
                return;
            }
            setState(SLEEP1, event);*/
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
                setTimer(DWELL_TIMEOUT, 1);
            }
            break;
        case WAIT_RESPONSE_OVER:
            break;
        //Radio receiving
        case WAIT_DATA:
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
            if(event == ACK_RECEIVED){
                txQueue.pop();
                acksReceived++;
                if (!txQueue.empty()) {
                    setState(WAIT_DATA_OVER, event);
                    setTimer(CCA_TIMEOUT, 0);
                }else {
                    setState(SLEEP1, event);
                }
            }else if(event == DWELL_TIMEOUT){
                setState(WAIT_DATA_OVER, event);
                setTimer(DWELL_TIMEOUT, 0);
            }
            break;
        case WAIT_BEACON:
            if(event == BEACON_RECEIVED){
                setState(DATA_CCA, event);
                setTimer(BEACON_RECEIVED, ccaInterval);
            }
            break;
    }
}

void RIMAC::setState(RIMACState newState, int event){
    switch(newState){
        case SLEEP1:
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

void RIMAC::sendBeacon(int type, int dest){
    cPacket *pkt = new cPacket();
    MacPacket *macFrame;
    if(type == 0){
        macFrame = new MacPacket("BEACON", MAC_LAYER_PACKET);
        encapsulatePacket(macFrame, pkt);
        macFrame->setSource(SELF_MAC_ADDRESS);
        macFrame->setDestination(BROADCAST_MAC_ADDRESS);
        sendToRadioLayer(macFrame->dup());
        setState(WAIT_BEACON_OVER, 0);
        cancelTimer(TRANSMISSION_ENDED);
        simtime_t txTime = TX_TIME(pkt->getByteLength());
        setTimer(TRANSMISSION_ENDED, txTime);
    }else if(type == 1){
        backoffTimes = backoffTimes + 1;
        macFrame = new MacPacket("BEACON-BW", MAC_LAYER_PACKET);
        encapsulatePacket(macFrame, pkt);
        macFrame->setSource(SELF_MAC_ADDRESS);
        macFrame->setDestination(BROADCAST_MAC_ADDRESS);
        sendToRadioLayer(macFrame->dup());
        setState(WAIT_BEACON_OVER, 0);
        cancelTimer(TRANSMISSION_ENDED);
        simtime_t txTime = TX_TIME(pkt->getByteLength());
        setTimer(TRANSMISSION_ENDED, txTime);
    }else if(type == 2){
        macFrame = new MacPacket("BEACON", MAC_LAYER_PACKET);
        encapsulatePacket(macFrame, pkt);
        macFrame->setSource(SELF_MAC_ADDRESS);
        macFrame->setDestination(dest);
        sendToRadioLayer(macFrame->dup());
        setState(WAIT_ACK_OVER, 0);
        cancelTimer(TRANSMISSION_ENDED);
        simtime_t txTime = TX_TIME(pkt->getByteLength());
        setTimer(TRANSMISSION_ENDED, txTime);
    }
}

bool RIMAC::isChannelClear(){
    toRadioLayer(createRadioCommand(SET_STATE, RX));
    int CCAcode = radioModule->isChannelClear();
    if (CCAcode == CLEAR) {
        return true;
    }else {
        return false;
    }
}

/* Handle packet received from upper (network) layer.
 * We need to create a MAC packet, (here it can just be the generic MacPacket)
 * and encapsulate the received network packet before forwarding it to RadioLayer
 */
void RIMAC::fromNetworkLayer(cPacket * pkt, int destination)
{
    MacPacket *macFrame = new MacPacket("Routing packet", MAC_LAYER_PACKET);
    encapsulatePacket(macFrame, pkt);
    macFrame->setSource(SELF_MAC_ADDRESS);
    macFrame->setDestination(destination);
    txQueue.push(macFrame);
    if(macState == SLEEP1){
        setState(WAIT_BEACON, 0);
    }
}

void RIMAC::sendToRadioLayer(MacPacket *macFrame){
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
void RIMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi)
{
    MacPacket *macPkt = dynamic_cast <MacPacket*>(pkt);
    string macName = macPkt->getName();
    if(macName=="INTERFERENCE"){
        if(macState == WAIT_DATA){
            if(backoffTimes == backoffMaxAttempts){
                setState(SLEEP1, 0);
                backoffTimes = 0;
                return;
            }
            sendBeacon(1, -1);
        }
        return;
    }
    if(macPkt == NULL)
        return;
    if(macName=="BEACON" && macPkt->getDestination() == SELF_MAC_ADDRESS){
        cancelTimer(ACK_RECEIVED);
        setTimer(ACK_RECEIVED, 0);
    }else if(macName=="BEACON"){
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
        sendBeacon(2, macPkt->getSource());
        setState(WAIT_ACK_OVER, 0);
        cancelTimer(TRANSMISSION_ENDED);
        simtime_t txTime = TX_TIME(pkt->getByteLength());
        setTimer(TRANSMISSION_ENDED, txTime);
    }
}

bool RIMAC::hasPacketToSent(MacPacket * macFrame){
    if(txQueue.size()>0){
        MacPacket *macFrameQueued = txQueue.front();
        if(macFrame->getSource()==macFrameQueued->getDestination())
            return true;
    }else{
        return false;
    }
}

void RIMAC::finishSpecific(){
    declareOutput("packetsSent");
    declareOutput("acksReceived");

    collectOutput("packetsSent", "packetsSent", packetsSent);
    collectOutput("acksReceived", "acksReceived", acksReceived);
}

void RIMAC::receiveEnergy(cMessage * msg)
{

}
