#ifndef TACS_ELEMENT_TEMPLATES_H
#define TACS_ELEMENT_TEMPLATES_H

#include "PlaneStressQuad.h"
#include "MITCShell.h"
#include "Solid.h"

// Add define statements for the element types
#define PlaneStressQuad2 PlaneStressQuad<2>
#define PlaneStressQuad3 PlaneStressQuad<3>
#define PlaneStressQuad4 PlaneStressQuad<4>
#define MITCShell2 MITCShell<2>
#define MITCShell3 MITCShell<3>
#define MITCShell4 MITCShell<4>
#define Solid2 Solid<2>
#define Solid3 Solid<3>
#define Solid4 Solid<4>

#endif // TACS_ELEMENT_TEMPLATES_H