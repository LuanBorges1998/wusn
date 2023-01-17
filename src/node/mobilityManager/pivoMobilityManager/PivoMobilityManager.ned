package node.mobilityManager.pivoMobilityManager;

simple PivoMobilityManager like node.mobilityManager.iMobilityManager {
    parameters:
       bool collectTraceInfo = default (false);
       double updateInterval = default (1000);
       double xCoorDestination = default (0);
       double yCoorDestination = default (0);
       double zCoorDestination = default (0);
       double speed = default (1);
       //int speedPivo = default(1);
       double drawPowerMove = default(15000); // mW
       //bool droneOnPivo = default(false);
       double speedPivo = default(1);
   }