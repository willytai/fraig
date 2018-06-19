/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_DEF_H
#define CIR_DEF_H

#include <vector>
#include <sstream>
#include "myHashMap.h"

using namespace std;

// TODO: define your own typedef or enum

class CirGateV;
class CirGate;
class CirMgr;
class SatSolver;

#define KEY 0x1

typedef vector<CirGate*>		GateList;
typedef vector<CirGateV>		VList;
typedef unsigned long long int Simtype;
typedef pair<SimKey, CirGate*> SimNode;
typedef vector<SimNode> FEC;
typedef pair<CirGate*, CirGate*> MergeNode;

enum GateType
{
   UNDEF_GATE = 0,
   PI_GATE    = 1,
   PO_GATE    = 2,
   AIG_GATE   = 3,
   CONST_GATE = 4,

   TOT_GATE
};

#endif // CIR_DEF_H

