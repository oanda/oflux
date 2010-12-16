#ifndef MIMPL_H
#define MIMPL_H

#include "OFluxRunTimeAbstract.h"
#include <boost/shared_ptr.hpp>

int init(int argc, char ** argv);

// fwd decl all that is needed

namespace oflux {
class RunTimeBase;
};

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

#define OFLUX_SNAPSHOT theRT->log_snapshot();

//#define DO_OUTPUT

extern unsigned int nodeCount_One;
extern unsigned int nodeCount_Two;
extern unsigned int nodeCount_Three;
extern unsigned int nodeCount_Four;
extern unsigned int nodeCount_Five;
extern unsigned int nodeCount_Select;
extern unsigned int nodeCount_Read;
extern unsigned int nodeCount_Route;
extern unsigned int nodeCount_Send;

#endif // MIMPL_H
