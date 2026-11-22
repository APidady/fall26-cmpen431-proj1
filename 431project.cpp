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

double GLOB_baseline_metrics[4];
std::map<std::string, std::map<std::string, double>* > GLOB_extracted_values;
std::map<std::string, unsigned int> GLOB_seen_configurations;


void initbaseline(std::string configuration) {
	GLOB_baseline_metrics[0] = calculategeomeanEDP(configuration);
	GLOB_baseline_metrics[1] = calculategeomeanED2P(configuration);
	GLOB_baseline_metrics[2] = calculategeomeanEDAP(configuration);
	GLOB_baseline_metrics[3] = calculategeomeanED2AP(configuration);
}


int main(int argc, char** argv) {
	std::ofstream logfile;
	std::ofstream bestfile;
	std::string bestED2Pconfig = GLOB_baseline;
	std::string bestEDPconfig = GLOB_baseline;
	std::string bestEDAPconfig = GLOB_baseline;
	std::string bestED2APconfig = GLOB_baseline;
	std::string currentConfiguration = GLOB_baseline;
	int optimizeforED2P = 0;
	int optimizeforEDP = 0;
	int optimizeforEDAP = 0;
	int optimizeforED2AP = 0;
	srand(0); // for stability during testing

	if (2 != argc) {
		fprintf(stderr, "Wrong number of arguments! Run as DSE e or DSE p for energy or performance run, respectively\n");
		return -1;
	}
	else {
		int isED2Parg = ('e' == argv[1][0]);
		int isEDParg = ('p' == argv[1][0]);
		int isEDAParg = ('d' == argv[1][0]);
		int isED2AParg = ('D' == argv[1][0]);

		if (!(isED2Parg || isEDParg || isEDAParg || isED2AParg)) {
			// todo prints
			fprintf(stderr, "Invalid argument! Run as \"DSE e\" or \"DSE p\" or \"DSE d\" or \"DSE D\" for performance or energy or energy-area or performance-energy-area optimization runs, respectively\n");
			return -1;
		}
		else {
			system("mkdir -p logs");
			system("mkdir -p summaryfiles");
			system("mkdir -p rawProjectOutputData");
			if (isED2Parg) {// do performance exploration
				optimizeforED2P = 1;
				logfile.open("logs/minED2P.log");
				bestfile.open("logs/minED2P.best");
			}
			else if (isEDParg) {// do energy exploration
				optimizeforEDP = 1;
				logfile.open("logs/minEDP.log");
				bestfile.open("logs/minEDP.best");
			}
			else if (isEDAParg) {// do energy-area exploration
				optimizeforEDAP = 1;
				logfile.open("logs/minEDAP.log");
				bestfile.open("logs/minEDAP.best");
			}
			else if (isED2AParg) {// do energy-area-performance exploration
				optimizeforED2AP = 1;
				logfile.open("logs/minED2AP.log");
				bestfile.open("logs/minED2AP.best");
			}
		}
	}
	runexperiments(GLOB_baseline, 0, 0); // generate baseline values
	populate(GLOB_baseline); // read raw values from files
	initbaseline(GLOB_baseline); // special-case handling for baseline

	logfile << calculategeomeanEDP(currentConfiguration) / GLOB_baseline_metrics[0] << ","
			<< calculategeomeanED2P(currentConfiguration) / GLOB_baseline_metrics[1] << ","
			<< calculategeomeanEDAP(currentConfiguration) / GLOB_baseline_metrics[2] << ","
			<< calculategeomeanED2AP(currentConfiguration) / GLOB_baseline_metrics[3] << ","
			<< calculategeomeanEDP(currentConfiguration) << ","
			<< calculategeomeanED2P(currentConfiguration) << ","
			<< calculategeomeanEDAP(currentConfiguration) << ","
			<< calculategeomeanED2AP(currentConfiguration) << std::endl;

	std::cout << std::endl; //clean up line breaks for baseline

	// do extremely simplistic approximation of simulated annealing
	for (unsigned int round = 1; round < 51; ++round) {
		double threshold = pow(2.71, ((-1.0) * (1 + (round) / 5.0)));
		for (unsigned int iteration = 1; iteration < 21; ++iteration) {
			double randDbl = ((double)(rand())) / ((double)(RAND_MAX));
			std::string nextconf = generateNextConfigurationProposal(
				currentConfiguration, 
				bestED2Pconfig, 
				bestEDPconfig, 
				bestEDAPconfig, 
				bestED2APconfig, 
				optimizeforED2P,
				optimizeforEDP,
				optimizeforEDAP,
				optimizeforED2AP
			);
			runexperiments(nextconf, round, iteration);
			populate(nextconf);
			if (0 == (*(GLOB_extracted_values[nextconf]))[GLOB_prefixes[0] + GLOB_fields[0]]) { // quick and dirty sanity check
				// run failed, try another, don't count this one
				std::cout << "R";
				--iteration; continue;
			}
			double proposedGEOEDP = calculategeomeanEDP(nextconf);
			double bestEDP = calculategeomeanEDP(bestEDPconfig);
			double proposedGEOED2P = calculategeomeanED2P(nextconf);
			double bestED2P = calculategeomeanED2P(bestED2Pconfig);
			double proposedGEOEDAP = calculategeomeanEDAP(nextconf);
			double bestEDAP = calculategeomeanEDAP(bestEDAPconfig);
			double proposedGEOED2AP = calculategeomeanED2AP(nextconf);
			double bestED2AP = calculategeomeanED2AP(bestED2APconfig);


			if (proposedGEOEDP < bestEDP) { // new best EDP
				bestEDPconfig = nextconf;
			}
			if (proposedGEOED2P < bestED2P) { // new best ED2P
				bestED2Pconfig = nextconf;
			}
			if (proposedGEOEDAP < bestEDAP) { // new best EDAP
				bestEDAPconfig = nextconf;
			}
			if (proposedGEOED2AP < bestED2AP) { // new best ED2AP
				bestED2APconfig = nextconf;
			}

			if (optimizeforED2P) { // if optimizing for ED2P
				if (proposedGEOED2P < bestED2P) { // if better, always accept
					currentConfiguration = nextconf;
				}
				else if (randDbl < threshold) { // else accept with probability threshold
					currentConfiguration = nextconf;
				}
			}
			else if (optimizeforEDP) { // if optimizing for EDP
				if (proposedGEOEDP < bestEDP) { // if better, always accept
					currentConfiguration = nextconf;
				}
				else if (randDbl < threshold) { // else accept with probability threshold
					currentConfiguration = nextconf;
				}
			}
			else if (optimizeforEDAP) { // if optimizing for EDAP
				if (proposedGEOEDAP < bestEDAP) { // if better, always accept
					currentConfiguration = nextconf;
				}
				else if (randDbl < threshold) { // else accept with probability threshold
					currentConfiguration = nextconf;
				}
			}
			else if (optimizeforED2AP) { // if optimizing for ED2AP
				if (proposedGEOED2AP < bestED2AP) { // if better, always accept
					currentConfiguration = nextconf;
				}
				else if (randDbl < threshold) { // else accept with probability threshold
					currentConfiguration = nextconf;
				}
			}

			logfile << calculategeomeanEDP(currentConfiguration) / GLOB_baseline_metrics[0] << ","
					<< calculategeomeanED2P(currentConfiguration) / GLOB_baseline_metrics[1] << ","
					<< calculategeomeanEDAP(currentConfiguration) / GLOB_baseline_metrics[2] << ","
					<< calculategeomeanED2AP(currentConfiguration) / GLOB_baseline_metrics[3] << ","
					<< calculategeomeanEDP(currentConfiguration) << ","
					<< calculategeomeanED2P(currentConfiguration) << ","
					<< calculategeomeanEDAP(currentConfiguration) << ","
					<< calculategeomeanED2AP(currentConfiguration) << std::endl;

		}
		std::cout << std::endl;
	}
	// all done

	//dump best configuration and associated data
	
	// EDP
	bestfile << bestEDPconfig << ","
		<< calculategeomeanEDP(bestEDPconfig) / GLOB_baseline_metrics[0] << ","
		<< calculategeomeanED2P(bestEDPconfig) / GLOB_baseline_metrics[1] << ","
		<< calculategeomeanEDAP(bestEDPconfig) / GLOB_baseline_metrics[2] << ","
		<< calculategeomeanED2AP(bestEDPconfig) / GLOB_baseline_metrics[3] << ","
		<< calculategeomeanEDP(bestEDPconfig) << ","
		<< calculategeomeanED2P(bestEDPconfig) << ","
		<< calculategeomeanEDAP(bestEDPconfig) << ","
		<< calculategeomeanED2AP(bestEDPconfig) << ",";
	for (int i = 0; i < 5; ++i) {
		bestfile << calculateEDP(bestEDPconfig, GLOB_prefixes[i]) << ","
			<< calculateEDP(bestEDPconfig, GLOB_prefixes[i]) / calculateEDP(GLOB_baseline, GLOB_prefixes[i]) << ",";
	}
	bestfile << std::endl;

	// ED2P
	bestfile << bestED2Pconfig << "," 
		<< calculategeomeanEDP(bestED2Pconfig) / GLOB_baseline_metrics[0] << ","
		<< calculategeomeanED2P(bestED2Pconfig) / GLOB_baseline_metrics[1] << ","
		<< calculategeomeanEDAP(bestED2Pconfig) / GLOB_baseline_metrics[2] << ","
		<< calculategeomeanED2AP(bestED2Pconfig) / GLOB_baseline_metrics[3] << ","
		<< calculategeomeanEDP(bestED2Pconfig) << ","
		<< calculategeomeanED2P(bestED2Pconfig) << ","
		<< calculategeomeanEDAP(bestED2Pconfig) << ","
		<< calculategeomeanED2AP(bestED2Pconfig) << ",";
	for (int i = 0; i < 5; ++i) {
		bestfile << calculateED2P(bestED2Pconfig, GLOB_prefixes[i]) << ","
			<< calculateED2P(bestED2Pconfig, GLOB_prefixes[i]) / calculateED2P(GLOB_baseline, GLOB_prefixes[i]) << ",";
	}
	bestfile << std::endl;

	// EDAP
	bestfile << bestEDAPconfig << ","
		<< calculategeomeanEDP(bestEDAPconfig) / GLOB_baseline_metrics[0] << ","
		<< calculategeomeanED2P(bestEDAPconfig) / GLOB_baseline_metrics[1] << ","
		<< calculategeomeanEDAP(bestEDAPconfig) / GLOB_baseline_metrics[2] << ","
		<< calculategeomeanED2AP(bestEDAPconfig) / GLOB_baseline_metrics[3] << ","
		<< calculategeomeanEDP(bestEDAPconfig) << ","
		<< calculategeomeanED2P(bestEDAPconfig) << ","
		<< calculategeomeanEDAP(bestEDAPconfig) << ","
		<< calculategeomeanED2AP(bestEDAPconfig) << ",";
	for (int i = 0; i < 5; ++i) {
		bestfile << calculateEDAP(bestEDAPconfig, GLOB_prefixes[i]) << ","
			<< calculateEDAP(bestEDAPconfig, GLOB_prefixes[i]) / calculateEDAP(GLOB_baseline, GLOB_prefixes[i]) << ",";
	}
	bestfile << std::endl;

	// ED2AP
	bestfile << bestED2APconfig << "," 
		<< calculategeomeanEDP(bestED2APconfig) / GLOB_baseline_metrics[0] << ","
		<< calculategeomeanED2P(bestED2APconfig) / GLOB_baseline_metrics[1] << ","
		<< calculategeomeanEDAP(bestED2APconfig) / GLOB_baseline_metrics[2] << ","
		<< calculategeomeanED2AP(bestED2APconfig) / GLOB_baseline_metrics[3] << ","
		<< calculategeomeanEDP(bestED2APconfig) << ","
		<< calculategeomeanED2P(bestED2APconfig) << ","
		<< calculategeomeanEDAP(bestED2APconfig) << ","
		<< calculategeomeanED2AP(bestED2APconfig) << ",";
	for (int i = 0; i < 5; ++i) {
		bestfile << calculateED2AP(bestED2APconfig, GLOB_prefixes[i]) << ","
			<< calculateED2AP(bestED2APconfig, GLOB_prefixes[i]) / calculateED2AP(GLOB_baseline, GLOB_prefixes[i]) << ",";
	}
	bestfile << std::endl;

	// close up
	logfile.close();
	bestfile.close();
}
