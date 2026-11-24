#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>

#include "431project.h"


/*
 * Not all configurations are inherently valid. (For example, L1 > L2). 
 * Returns 1 if valid, else 0.
 */

int validateConfiguration(std::string configuration){
    // is the configuration at least describing 18 integers/indices?
    if (isan18dimconfiguration(configuration) != 1)
        return 0;

    // if it is, lets convert it to an array of ints for use below
    int configurationDimsAsInts[18];
    int returnValue = 1;  // assume true, set 0 if otherwise
    extractConfiguration(configuration, configurationDimsAsInts); // Configuration parameters now available in array
    
    // 
    // FIXME - YOUR VERIFICATION CODE HERE 
    // ...

    int l1BlockChoices[4] = {8, 16, 32, 64};
    int l1AssocChoices[3] = {1, 2, 4};
    int l1SetsChoices[9] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

    int ul2SetsChoices[10] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    int ul2BlockChoices[4] = {16, 32, 64, 128};
    int ul2AssocChoices[5] = {1, 2, 4, 8, 16};

    int l1BlockBytes = l1BlockChoices[configurationDimsAsInts[0]];
    int ul2BlockBytes = ul2BlockChoices[configurationDimsAsInts[11]];

    // Model requirement: UL2 block size must be at least 2x L1 block size and at most 128 bytes
    if (ul2BlockBytes < 2 * l1BlockBytes || ul2BlockBytes > 128) {
        return 0;
    }

    int dl1Assoc = l1AssocChoices[configurationDimsAsInts[7]];
    int il1Assoc = l1AssocChoices[configurationDimsAsInts[9]];
    int ul2Assoc = ul2AssocChoices[configurationDimsAsInts[12]];

    int dl1Sets = l1SetsChoices[configurationDimsAsInts[6]];
    int il1Sets = l1SetsChoices[configurationDimsAsInts[8]];
    int ul2Sets = ul2SetsChoices[configurationDimsAsInts[10]];

    long long dl1Bytes = (long long)dl1Sets * dl1Assoc * l1BlockBytes;
    long long il1Bytes = (long long)il1Sets * il1Assoc * l1BlockBytes;
    long long ul2Bytes = (long long)ul2Sets * ul2Assoc * ul2BlockBytes;

    // Model requirement: cache sizes must not exceed 2 MB (2097152 bytes)
    if (dl1Bytes > 2097152 || il1Bytes > 2097152 || ul2Bytes > 2097152) {
        return 0;
    }

    // Model requirement: UL2 must be inclusive (at least as large as IL1 + DL1)
    if (ul2Bytes < il1Bytes + dl1Bytes) {
        return 0;
    }

    // Model requirement: DL1 latency follows size (8KB->1, 16KB->2, 32KB->3, 64KB->4) 
    // with associativity (+1 for 2-way, +2 for 4-way)
    long long dl1KB = dl1Bytes / 1024;
    int dl1BaseLatency = 4;
    if (dl1KB <= 8) {
        dl1BaseLatency = 1;
    } else if (dl1KB <= 16) {
        dl1BaseLatency = 2;
    } else if (dl1KB <= 32) {
        dl1BaseLatency = 3;
    } else {
        dl1BaseLatency = 4;
    }

    int dl1AssocPenalty = 0;
    if (dl1Assoc == 2) {
        dl1AssocPenalty = 1;
    } else if (dl1Assoc == 4) {
        dl1AssocPenalty = 2;
    }

    int dl1Latency = 1 + configurationDimsAsInts[14];
    if (dl1Latency != dl1BaseLatency + dl1AssocPenalty) {
        return 0;
    }

    // Model requirement: IL1 latency follows size (8KB->1, 16KB->2, 32KB->3, 64KB->4) 
    // with associativity (+1 for 2-way, +2 for 4-way)
    long long il1KB = il1Bytes / 1024;
    int il1BaseLatency = 4;
    if (il1KB <= 8) {
        il1BaseLatency = 1;
    } else if (il1KB <= 16) {
        il1BaseLatency = 2;
    } else if (il1KB <= 32) {
        il1BaseLatency = 3;
    } else {
        il1BaseLatency = 4;
    }

    int il1AssocPenalty = 0;
    if (il1Assoc == 2) {
        il1AssocPenalty = 1;
    } else if (il1Assoc == 4) {
        il1AssocPenalty = 2;
    }

    int il1Latency = 1 + configurationDimsAsInts[15];
    if (il1Latency != il1BaseLatency + il1AssocPenalty) {
        return 0;
    }

    // Model requirement: UL2 latency follows size (128KB->7 up to 2MB->11 at 4-way) 
    // with associativity adjustment (-2/-1/0/+1/+2)
    long long ul2KB = ul2Bytes / 1024;
    int ul2BaseLatency = 7;
    if (ul2KB <= 128) {
        ul2BaseLatency = 7;
    } else if (ul2KB <= 256) {
        ul2BaseLatency = 8;
    } else if (ul2KB <= 512) {
        ul2BaseLatency = 9;
    } else if (ul2KB <= 1024) {
        ul2BaseLatency = 10;
    } else {
        ul2BaseLatency = 11;
    }

    int ul2AssocAdjust = 0;
    if (ul2Assoc == 1) {
        ul2AssocAdjust = -2;
    } else if (ul2Assoc == 2) {
        ul2AssocAdjust = -1;
    } else if (ul2Assoc == 4) {
        ul2AssocAdjust = 0;
    } else if (ul2Assoc == 8) {
        ul2AssocAdjust = 1;
    } else if (ul2Assoc == 16) {
        ul2AssocAdjust = 2;
    }

    int ul2Latency = 5 + configurationDimsAsInts[16];
    if (ul2Latency != ul2BaseLatency + ul2AssocAdjust) {
        return 0;
    }

    return returnValue;
}

/* Helper Function */
void copyCfg(int src[18], int dst[18]) {
    for (int i = 0; i < 18; i++) {
        dst[i] = src[i];
    }
}


/*
 * Given the current best known configuration for a particular optimization, 
 * the current configuration, and using globally visible map of all previously 
 * investigated configurations, suggest a new, previously unexplored design 
 * point. You will only be allowed to investigate 1000 design points in a 
 * particular run, so choose wisely. Using the optimizeForX variables,
 * propose your next configuration provided the optimiztion strategy.
 */
std::string YourProposalFunction(
    std::string currentconfiguration,
    std::string bestED2Pconfiguration,
    std::string bestEDPconfiguration,
    std::string bestEDAPconfiguration,
    std::string bestED2APconfiguration,
    int optimizeforED2P,
    int optimizeforEDP,
    int optimizeforEDAP,
    int optimizeforED2AP
){
    /*
    * REPLACE THE BELOW CODE WITH YOUR PROPOSAL FUNCTION
    *
    * The proposal function below is extremely unintelligent and
    * will produce configurations that, while properly formatted, 
    * violate specified project constraints
    */    

    // start with the best config for optimzation run
    std::string targetBestCfg = bestEDPconfiguration;
    if (optimizeforED2P) {
        targetBestCfg = bestED2Pconfiguration;
    } else if (optimizeforEDP) {
        targetBestCfg = bestEDPconfiguration;
    } else if (optimizeforEDAP) {
        targetBestCfg = bestEDAPconfiguration;
    } else if (optimizeforED2AP) {
        targetBestCfg = bestED2APconfiguration;
    }

    int baseCfg[18];
    extractConfiguration(targetBestCfg, baseCfg);

    // replace bpred perfect branch predictor
    if (baseCfg[17] == 0) {
        baseCfg[17] = 2;
    }

    // indices are 0 based but GLOB_dimension holds total counts
    int maxWidth = (int)GLOB_dimensioncardinality[0] - 1;
    int maxRuu = (int)GLOB_dimensioncardinality[3] - 1;
    int maxLsq = (int)GLOB_dimensioncardinality[4] - 1;
    int maxDl1Sets = (int)GLOB_dimensioncardinality[6] - 1;
    int maxDl1Lat = (int)GLOB_dimensioncardinality[14] - 1;
    int maxUl2Sets = (int)GLOB_dimensioncardinality[10] - 1;
    int maxUl2Lat = (int)GLOB_dimensioncardinality[16] - 1;

    int nextCfg[18];
    std::string outCfg;

    // EDAP and ED2AP multiply by area, try to reduce area related dimensions
    if (optimizeforEDAP || optimizeforED2AP) {

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // RUU and LSQ grow quadratically in area, so reduce both of them
        nextCfg[3] -= 1;
        nextCfg[4] -= 1;

        if (nextCfg[3] < 0) {
            nextCfg[3] = 0;
        }
        if (nextCfg[4] < 0) {
            nextCfg[4] = 0;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
 	    return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // L2 is bigger, so try to lower ul2 sets and ul2 latency
        nextCfg[10] -= 1;
        nextCfg[16] -= 1;

        if (nextCfg[10] < 0) {
            nextCfg[10] = 0;
        }
        if (nextCfg[16] < 0) {
            nextCfg[16] = 0;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // Try reducing the width
        nextCfg[0] -= 1;

        if (nextCfg[0] < 0) {
            nextCfg[0] = 0;
        }
        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
            return outCfg;
        }
    }

    // ED2P squares execution time, optimize for wider/faster dimensions 
    if (optimizeforED2P) {

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);
        
        // Try increasing the width
        nextCfg[0] += 1;

        if (nextCfg[0] > maxWidth) {
            nextCfg[0] = maxWidth;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // Try increasing the UL2 set and latency 
        nextCfg[10] += 1;
        nextCfg[16] += 1;

        if (nextCfg[10] > maxUl2Sets) {
            nextCfg[10] = maxUl2Sets;
        }
        if (nextCfg[16] > maxUl2Lat) {
            nextCfg[16] = maxUl2Lat;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
            return outCfg;
        }
    }

    // reset the nextCfg
    copyCfg(baseCfg, nextCfg);

  
    // Try in-order (0) vs out-of-order (1) scheduling
    if (optimizeforED2P) {
        nextCfg[2] = 1;
    } else if (optimizeforEDAP) {
        nextCfg[2] = 0;
    } else {
        nextCfg[2] = (baseCfg[2] == 0) ? 1 : 0;
    }
    outCfg = compactConfiguration(nextCfg);
    if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
        return outCfg;
    }

    // reset the nextCfg
    copyCfg(baseCfg, nextCfg);

    // try changing DL1 sets and latency 
    // reduce when area matters, increase otherwise
    int cacheStep = (optimizeforEDAP || optimizeforED2AP) ? -1 : 1;

    nextCfg[6] += cacheStep;
    nextCfg[14] += cacheStep;

    if (nextCfg[6] < 0) {
        nextCfg[6] = 0;
    }
    if (nextCfg[6] > maxDl1Sets) {
        nextCfg[6] = maxDl1Sets;
    }
    if (nextCfg[14] < 0) {
        nextCfg[14] = 0;
    }
    if (nextCfg[14] > maxDl1Lat) {
        nextCfg[14] = maxDl1Lat;
    }

    outCfg = compactConfiguration(nextCfg);
    if (validateConfiguration(outCfg) == 1 && !GLOB_seen_configurations[outCfg]) {
        return outCfg;
    }

    // fallback if all our choices are around targetBestCfg
    // and they were already seen or rejected
    int curCfg[18];
    extractConfiguration(currentconfiguration, curCfg);

    // remove perfect branch prediction
    if (curCfg[17] == 0) {
        curCfg[17] = 2;
    }

    curCfg[0] += 1;
    if (curCfg[0] > maxWidth) {
        curCfg[0] = 0;
    }

    return compactConfiguration(curCfg);
}


