/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
PIGate::reportGate() const
{
  stringstream ss, value;
  ss << "= " << getTypeStr() << '(' << _id << ')';
  if (_name != "") ss << "\"" << _name << "\"";
  ss << ", line " << _line;
  Simtype token = _value;
  for (int i = 1; i <= 64; ++i) {
    value << ((token & (Simtype(KEY) << (64-i))) >> (64-i));
    if (i % 8 == 0 && i != 64) value << '_';
  }
  cout << "================================================================================" << endl;
  cout << ss.str() << endl;
  cout << '=' << " FECs:"; printFEC();
  cout << '=' << " Value: " << value.str() << endl;
  cout << "================================================================================" << endl;
}

void
POGate::reportGate() const
{
  stringstream ss, value;
  ss << "= " << getTypeStr() << '(' << _id << ')';
  if (_name != "") ss << "\"" << _name << "\"";
  ss << ", line " << _line;
  Simtype token = _value;
  for (int i = 1; i <= 64; ++i) {
    value << ((token & (Simtype(KEY) << (64-i))) >> (64-i));
    if (i % 8 == 0 && i != 64) value << '_';
  }
  cout << "================================================================================" << endl;
  cout << ss.str() << endl;
  cout << '=' << " FECs:"; printFEC();
  cout << '=' << " Value: " << value.str() << endl;
  cout << "================================================================================" << endl;
}

void
AIGGate::reportGate() const
{
  stringstream ss, value;
  ss << "= " << getTypeStr() << '(' << _id << ')';
  ss << ", line " << _line;
  Simtype token = _value;
  for (int i = 1; i <= 64; ++i) {
    value << ((token & (Simtype(KEY) << (64-i))) >> (64-i));
    if (i % 8 == 0 && i != 64) value << '_';
  }
  cout << "================================================================================" << endl;
  cout << ss.str() << endl;
  cout << '=' << " FECs:"; printFEC();
  cout << '=' << " Value: " << value.str() << endl;
  // cout << "debug: " << _value << endl;
  cout << "================================================================================" << endl;
}

void
CONSTGate::reportGate() const
{
  stringstream ss;
  ss << "= " << getTypeStr() << '(' << _id << ')';
  ss << ", line " << _line;
  cout << "================================================================================" << endl;
  cout << ss.str() << endl;
  cout << '=' << " FECs:"; printFEC();
  cout << '=' << " Value: 00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000" << endl;
  cout << "================================================================================" << endl;
}

void
UNDEFGate::reportGate() const
{
  stringstream ss;
  ss << "= " << getTypeStr() << '(' << _id << ')';
  ss << ", line " << _line;
  cout << "================================================================================" << endl;
  cout << ss.str() << endl;
  cout << '=' << " FECs:"; printFEC();
  cout << '=' << " Value: 00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000" << endl;
  cout << "================================================================================" << endl;
}

void CirGate::printFEC() const {
  if (_fec == NULL) {
    cout << endl;
    return;
  }
  bool check = false;
  for (size_t i = 0; i < _fec->size(); ++i) {
    if (_fec->at(i).second != this) continue;
    check = _fec->at(i).first.isInv();
    break;
  }
  for (size_t i = 0; i < _fec->size(); ++i) {
    if (_fec->at(i).second == this) continue;

    if (!check) {
      cout << ' ';
      if (_fec->at(i).first.isInv()) cout << '!';
      cout << _fec->at(i).second->getId();
    } else {
      cout << ' ';
      if (!_fec->at(i).first.isInv()) cout << '!';
      cout << _fec->at(i).second->getId();
    }

  }
  cout << endl;
}

void reportin (string t, VList& n, int level) {
  if(level < 0) return;
  for (size_t i = 0; i < n.size(); i++) {
    cout << t;
    if (n[i].isInv()) cout << '!';
    cout << n[i].gate()->getTypeStr() << ' ' << n[i].gate()->getId();

    if (n[i].gate()->isGlobalRef()) {
      cout << " (*)" << endl;
      continue;
    } else cout << endl;

    VList v = n[i].gate()->getInList();
    int fuck = level - 1;
    if (v.size() != 0) {
      if (fuck >= 0) n[i].gate()->setToGlobalRef();
      reportin(t + "  ", v, fuck);
    }
  }
}

void
CirGate::reportFanin(int level)
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   VList n = getInList();
   cout << getTypeStr() << ' ' << getId() << endl;
   --level;
   setToGlobalRef();
   reportin("  ", n, level);
}

void reportout (string t, VList& n, int level) {
  if(level < 0) return;
  for (size_t i = 0; i < n.size(); i++) {
    cout << t;
    if (n[i].isInv()) cout << '!';
    cout << n[i].gate()->getTypeStr() << ' ' << n[i].gate()->getId();

    if (n[i].gate()->isGlobalRef() && level > 0) {
      cout << " (*)" << endl;
      continue;
    } else cout << endl;

    VList v = n[i].gate()->getOutList();
    int fuck = level - 1;
    if (v.size() != 0) {
      if (fuck >= 0) n[i].gate()->setToGlobalRef();
      reportout(t + "  ", v, fuck);
    }
  }
}

void
CirGate::reportFanout(int level)
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   VList o = getOutList();
   cout << getTypeStr() << ' ' << getId() << endl;
   --level;
   setToGlobalRef();

   reportout("  ", o, level);
}