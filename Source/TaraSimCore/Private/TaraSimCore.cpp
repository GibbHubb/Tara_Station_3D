#include "TaraSimCore.h"
#include "Modules/ModuleManager.h"

// Standard "empty" module implementation. The sim core has no global module
// state; entities and systems are constructed at runtime inside the Station.
IMPLEMENT_MODULE(FDefaultModuleImpl, TaraSimCore);
