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

    const int l1BlockChoices[4] = {8, 16, 32, 64};
    const int l1AssocChoices[3] = {1, 2, 4};
    const int l1SetsChoices[9] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

    const int ul2SetsChoices[10] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    const int ul2BlockChoices[4] = {16, 32, 64, 128};
    const int ul2AssocChoices[5] = {1, 2, 4, 8, 16};

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

    // Model requirement: L1 caches must be exactly 8, 16, 32, or 64 KB
    bool validDL1Size = false;
    bool validIL1Size = false;

    int validL1Sizes[4] = {8192, 16384, 32768, 65536};

    for (int i = 0; i < 4; i++) {
        if (dl1Bytes == validL1Sizes[i]) {
            validDL1Size = true;
        }
        if (il1Bytes == validL1Sizes[i]) {
            validIL1Size = true;
        }
    }

    if (!validDL1Size || !validIL1Size) {
        return 0;
    }

    // Model requirement: UL2 must be inclusive (at least as large as IL1 + DL1)
    if (ul2Bytes < il1Bytes + dl1Bytes) {
        return 0;
    }

    // Model requirement: DL1 latency follows size 
    // 8KB->1, 16KB->2, 32KB->3, 64KB->4 
    int dl1BaseLatency = 0;

    if (dl1Bytes == 8192) {
        dl1BaseLatency = 1;
    } else if (dl1Bytes == 16384) {
        dl1BaseLatency = 2;
    } else if (dl1Bytes == 32768) {
        dl1BaseLatency = 3;
    } else if (dl1Bytes == 65536) {
        dl1BaseLatency = 4;
    }

    // For 2-way add 1 cycle penalty, for 4-way add 2 cycle penalty
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

    // Model requirement: IL1 latency follows size 
    // 8KB->1, 16KB->2, 32KB->3, 64KB->4 
    int il1BaseLatency = 0;

    if (il1Bytes == 8192) {
        il1BaseLatency = 1;
    } else if (il1Bytes == 16384) {
        il1BaseLatency = 2;
    } else if (il1Bytes == 32768) {
        il1BaseLatency = 3;
    } else if (il1Bytes == 65536) {
        il1BaseLatency = 4;
    }

    // For 2-way add 1 cycle penalty, for 4-way add 2 cycle penalty
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

    // Model requirement: UL2 latency follows size 
    // 128KB->7 up to 2MB->11 at 4-way 

    int ul2BaseLatency = 0;

    if (ul2Bytes == 131072) {
        ul2BaseLatency = 7;
    } else if (ul2Bytes == 262144) {
        ul2BaseLatency = 8;
    } else if (ul2Bytes == 524288) {
        ul2BaseLatency = 9;
    } else if (ul2Bytes == 1048576) {
        ul2BaseLatency = 10;
    } else if (ul2Bytes == 2097152) {
        ul2BaseLatency = 11;
    } else {
        return 0; //unsupported UL2 size
    }

    // associativity adjustment -2/-1/0/+1/+2
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

    // replace perfect branch predictor
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
        nextCfg[3]--;
        nextCfg[4]--;

        if (nextCfg[3] < 0) {
            nextCfg[3] = 0;
        }
        if (nextCfg[4] < 0) {
            nextCfg[4] = 0;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // L2 is bigger, so try to lower ul2 sets and ul2 latency
        nextCfg[10]--;
        nextCfg[16]--;

        if (nextCfg[10] < 0) {
            nextCfg[10] = 0;
        }
        if (nextCfg[16] < 0) {
            nextCfg[16] = 0;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // Try reducing the width
        nextCfg[0]--;

        if (nextCfg[0] < 0) {
            nextCfg[0] = 0;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

    }

    // ED2P squares execution time, optimize for wider/faster dimensions 
    if (optimizeforED2P) {

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);
        
        // keep the width of 1 or 2 bcos of the crossed out??
        nextCfg[0]++;

        if (nextCfg[0] > maxWidth) {
            nextCfg[0] = maxWidth;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // Try increasing RUU size
        nextCfg[3]++;

        if (nextCfg[3] > maxRuu)
            nextCfg[3] = maxRuu;

        outCfg = compactConfiguration(nextCfg);

        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

        // Try increasing LSQ size
        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        nextCfg[4]++;

        if (nextCfg[4] > maxLsq)
            nextCfg[4] = maxLsq;

        outCfg = compactConfiguration(nextCfg);

        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }

        // reset the nextCfg
        copyCfg(baseCfg, nextCfg);

        // Try increasing the UL2 set and latency 
        nextCfg[10]++;
        nextCfg[16]++;

        if (nextCfg[10] > maxUl2Sets) {
            nextCfg[10] = maxUl2Sets;
        }
        if (nextCfg[16] > maxUl2Lat) {
            nextCfg[16] = maxUl2Lat;
        }

        outCfg = compactConfiguration(nextCfg);
        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }
    }

    // EDP optimization: balance energy and execution time
    if (optimizeforEDP) {

        // Try increasing processor width
        copyCfg(baseCfg, nextCfg);
        nextCfg[0]++;

        if (nextCfg[0] <= maxWidth) {
            outCfg = compactConfiguration(nextCfg);

            if (validateConfiguration(outCfg) == 1 &&
                GLOB_seen_configurations.find(outCfg) ==
                GLOB_seen_configurations.end()) {
                return outCfg;
            }
        }

        // Try increasing RUU size
        copyCfg(baseCfg, nextCfg);
        nextCfg[3]++;

        if (nextCfg[3] <= maxRuu) {
            outCfg = compactConfiguration(nextCfg);

            if (validateConfiguration(outCfg) == 1 &&
                GLOB_seen_configurations.find(outCfg) ==
                GLOB_seen_configurations.end()) {
                return outCfg;
            }
        }

        // Try increasing LSQ size
        copyCfg(baseCfg, nextCfg);
        nextCfg[4]++;

        if (nextCfg[4] <= maxLsq) {
            outCfg = compactConfiguration(nextCfg);

            if (validateConfiguration(outCfg) == 1 &&
                GLOB_seen_configurations.find(outCfg) ==
                GLOB_seen_configurations.end()) {
                return outCfg;
            }
        }
    }


    // reset the nextCfg
    copyCfg(baseCfg, nextCfg);

    // Try changing TLB size
    nextCfg[13]++;

    if (nextCfg[13] >= (int)GLOB_dimensioncardinality[13])
        nextCfg[13] = 0;

    outCfg = compactConfiguration(nextCfg);

    if (validateConfiguration(outCfg) == 1 &&
        GLOB_seen_configurations.find(outCfg) ==
        GLOB_seen_configurations.end()) {
        return outCfg;
    }

    // Try changing branch predictor
    copyCfg(baseCfg, nextCfg);

    nextCfg[17]++;

    if (nextCfg[17] >= (int)GLOB_dimensioncardinality[17]) {
        nextCfg[17] = 1;
    }

    outCfg = compactConfiguration(nextCfg);

    if (validateConfiguration(outCfg) == 1 &&
        GLOB_seen_configurations.find(outCfg) ==
        GLOB_seen_configurations.end()) {
        return outCfg;
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
    if (validateConfiguration(outCfg) == 1 &&
        GLOB_seen_configurations.find(outCfg) ==
        GLOB_seen_configurations.end()) {
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
    if (validateConfiguration(outCfg) == 1 &&
        GLOB_seen_configurations.find(outCfg) ==
        GLOB_seen_configurations.end()) {
        return outCfg;
    }

    // fallback if all our choices are around targetBestCfg
    // and they were already seen or rejected
    for (int i = 0; i < (int)GLOB_dimensioncardinality[0]; i++) {

        copyCfg(baseCfg, nextCfg);

        nextCfg[0] = i;

        outCfg = compactConfiguration(nextCfg);

        if (validateConfiguration(outCfg) == 1 &&
            GLOB_seen_configurations.find(outCfg) ==
            GLOB_seen_configurations.end()) {
            return outCfg;
        }
    }

    // Terminating early, search each dimension around baseCfg 
    for (int dim = 0; dim < 18; dim++) {  
        for (int val = 0; val < (int)GLOB_dimensioncardinality[dim]; val++) {  
            copyCfg(baseCfg, nextCfg);  
            nextCfg[dim] = val;  
            outCfg = compactConfiguration(nextCfg);  
            if (validateConfiguration(outCfg) == 1 &&  
                GLOB_seen_configurations.find(outCfg) ==  
                GLOB_seen_configurations.end()) {  
                return outCfg;  
            }  
        }  
    }  

    // Still terminating, last resort look at random value till we find a valid option 
    const int MAX_ATTEMPTS = 10000;  
    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {  
        copyCfg(baseCfg, nextCfg);  
        int numChanges = 1 + (rand() % 4);  
        for (int c = 0; c < numChanges; c++) {  
            int dim = rand() % 18;  
            if (dim == 17) {  
                nextCfg[dim] = 1 + (rand() % ((int)GLOB_dimensioncardinality[dim] - 1));  
            } else {  
                nextCfg[dim] = rand() % (int)GLOB_dimensioncardinality[dim];  
            }  
        }  
        outCfg = compactConfiguration(nextCfg);  
        if (validateConfiguration(outCfg) == 1 &&  
            GLOB_seen_configurations.find(outCfg) ==  
            GLOB_seen_configurations.end()) {  
            return outCfg;  
        }  
    }  

    // Exit here if we have exhausted lookups
    std::cerr << "ERROR: No valid, unexplored configuration found!" << std::endl;
    //return currentconfiguration; //this does not work
    exit(1);

}


