#include "OFluxRunTime.h"

int init(int argc, char ** argv);

// fwd decl all that is needed

namespace oflux {
class RunTime;
};

extern oflux::RunTime * theRT;

#define OFLUX_SNAPSHOT theRT->log_snapshot();

#define DO_OUTPUT

extern unsigned int nodeCount_One;
extern unsigned int nodeCount_Two;
extern unsigned int nodeCount_Three;
extern unsigned int nodeCount_Four;
extern unsigned int nodeCount_Five;
extern unsigned int nodeCount_Select;
extern unsigned int nodeCount_Read;
extern unsigned int nodeCount_Route;
extern unsigned int nodeCount_Send;
