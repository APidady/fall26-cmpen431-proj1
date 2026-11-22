const std::string GLOB_outputpath = "rawProjectOutputData/";
const std::string GLOB_script = "worker.sh ";
const std::string GLOB_baseline = "0 0 0 0 0 0 5 0 5 0 2 2 2 3 0 0 3 0";
const std::string GLOB_dimensionnames[18] = {
	"width",
	"fetchspeed",
	"scheduling",
	"RUUsize",
	"LSQsize",
	"Memports",
	"dl1sets",
	"dl1assoc",
	"il1sets",
	"il1assoc",
	"ul2sets",
	"ul2blocksize",
	"ul2assoc",
	"tlbsets",
	"dl1lat",
	"il1lat",
	"ul2lat",
	"bpred"
};
const unsigned int GLOB_dimensioncardinality[18] = {4, 2, 2, 6, 4, 2, 9, 3, 9, 3, 10, 4, 5, 5, 7, 7, 9, 6};
const std::string GLOB_fields[7] = {"sim_num_insn ", "sim_cycle ", "il1.accesses ", "dl1.accesses ", "ul2.accesses ", "ul2.misses ", "ul2.writebacks "};
const std::string GLOB_prefixes[5] = {"0.", "1.", "2.", "3.", "4."};

extern double GLOB_baseline_EP_pair[4];
extern std::map<std::string, std::map<std::string, double>* > GLOB_extracted_values;
extern std::map<std::string, unsigned int> GLOB_seen_configurations;

/*
 * Given the current best known configuration for a particular optimization,
 * the current configuration, and using globally visible map of all previously
 * investigated configurations, suggest a new, previously unexplored design
 * point. You will only be allowed to investigate 1000 design points in a
 * particular run, so choose wisely. Using the optimizeForX variables,
 * propose your next configuration provided the optimiztion strategy.
 */
std::string generateNextConfigurationProposal(
	std::string currentconfiguration,
	std::string bestED2Pconfiguration,
	std::string bestEDPconfiguration,
	std::string bestEDAPconfiguration,
	std::string bestED2APconfiguration,
	int optimizeforED2P,
	int optimizeforEDP,
	int optimizeforEDAP,
	int optimizeforED2AP
);

// Your helper function for the above wrapper loop
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
);

/*
 * Not all configurations are inherently valid. (For example, L1 > L2).
 * Returns 1 if valid, else 0.
 */
int validateConfiguration(std::string configuration);

/*
 * Runs experiments for a given configuration, if they have not already been run. Stores raw data in globally accessible location. 
 * 2nd and 3rd arguments are for progress printing.
*/
int runexperiments(std::string configuration, unsigned int round, unsigned int iteration);

/*
 * Uses pre-populated in-memory structure to calculate the execution time for a given benchmark+configuration run
 */
double calculateExecutionTime(std::string configuration, std::string benchmarkprefix);
double calculategeomeanExecutionTime(std::string configuration);

/*
 * Uses pre-populated in-memory structure to calculate the area for a given benchmark+configuration run
 */
double calculateArea(std::string configuration, std::string benchmarkprefix);
double calculategeomeanArea(std::string configuration);

/*
 * Uses pre-populated in-memory structure to calculate the energy delay product for a given benchmark+configuration run
 */
double calculateEDP(std::string configuration, std::string benchmarkprefix);
double calculategeomeanEDP(std::string configuration);

/*
 * Uses pre-populated in-memory structure to calculate the energy delay squared product for a given benchmark+configuration run
 */
double calculateED2P(std::string configuration, std::string benchmarkprefix);
double calculategeomeanED2P(std::string configuration);


/*
 * Uses pre-populated in-memory structure to calculate the energy delay area product for a given benchmark+configuration run
 */
double calculateEDAP(std::string configuration, std::string benchmarkprefix);
double calculategeomeanEDAP(std::string configuration);

/*
 * Uses pre-populated in-memory structure to calculate the energy delay squared area product for a given benchmark+configuration run
 */
double calculateED2AP(std::string configuration, std::string benchmarkprefix);
double calculategeomeanED2AP(std::string configuration);

/*
 * Sanity checks that the configuration string represents an 18 dimensional value, each dimension within bounds.
 * The configuration may still violate some project constraints.
 */
int isan18dimconfiguration(std::string configuration);

/*
 * Retrieves data from raw result files and places it in an in-memory data structure
 */
void populate(std::string configuration);

// return cycle time in seconds, given a configuration string. returns 1ps on error.
double cycleTime(std::string configuration);

//extracts the 18 fields of a configuration string into an array of integers
void extractConfiguration(std::string configuration, int* buffer18);

// packs the 18 field array into a string
std::string compactConfiguration(int configuration[18]);
