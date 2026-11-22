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

    return returnValue;
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
  
    // produces an essentially random proposal
    int configuration[18];
    for(int dim = 0; dim < 18; dim++){
        configuration[dim] = rand() % GLOB_dimensioncardinality[dim];
    }

    return compactConfiguration(configuration);
}

