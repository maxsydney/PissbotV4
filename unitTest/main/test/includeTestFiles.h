#ifndef INCLUDE_TEST_FILES
#define INCLUDE_TEST_FILES

// This file forward declares a dummy function from each test file to force the tests to link.
// TODO: Replace this workaround with build system instructions

void includeThermoTests(void);
void includeUtilitiesTests(void);
void includeConnectionManagerTests(void);
void includeSensorManagerTests(void);
void includeOneWireBusTests(void);
void includeWebserverTests(void);
void includePumpTests(void);
void includeControllerTests(void);
void includeDistillerManagerTests(void);
void includeFlowmeterTests(void);

#endif // INCLUDE_TEST_FILES