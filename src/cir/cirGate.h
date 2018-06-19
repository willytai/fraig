/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGateV
{
public:
   #define NEG 0x1
   CirGateV() : _gateV(size_t(0)) {}
   CirGateV(CirGate* g, size_t phase) : _gateV(size_t(g) + phase) {}
   CirGate* gate() const { return (CirGate*)(_gateV & ~size_t(NEG)); }
   void replaceGate(const CirGate* g) {
     if (isInv()) _gateV = size_t(g) + size_t(1);
     else _gateV = size_t(g);
   }
   void changePhase() {
     if (isInv()) _gateV -= 1; else _gateV += 1;
   }
   void setToInv() { if (!isInv()) _gateV += size_t(1); }
   void set() { if (isInv()) _gateV-= size_t(1); }
   bool isInv() const { return (_gateV & NEG); }
   size_t operator () () const { return _gateV; }
private:
   size_t _gateV;
};

class CirGate
{
  // for FDS
   static unsigned _globalRef;
   unsigned _ref;

public:
   CirGate() : _ref(0), in_dfs(false), _value(0), _var(-1), _fec(NULL), _fecid(-1) {}
   virtual ~CirGate() {}

   // Basic access methods
   bool InDfs() { return in_dfs; }
   unsigned getLineNo() const { return _line; }
   unsigned getId() const { return _id; }
   Simtype value() const { return _value; }
   virtual GateType getType() const = 0;
   virtual string getTypeStr() const = 0;
   virtual string getName() const { return ""; }
   virtual CirGateV getfanout(int i = 0) const {}
   virtual CirGateV getfanin(int i = 0) const {}
   virtual VList getInList() const { VList n; return n; }
   virtual VList getOutList() const { VList n; return n; }
   virtual size_t fanoutNO() { return 0; }
   virtual size_t faninNO() { return 0; }

   // Printing functions
   virtual void printGate() const = 0;
   virtual void reportGate() const = 0;
   void reportFanin(int level);
   void reportFanout(int level);

   // setting function
   virtual void setfanin(CirGateV& in) { return; }
   virtual void replaceFanin(int i, const CirGate* in, bool inv = false) { return; }
   virtual void setfanout(CirGateV& out) { return; }
   virtual void replaceFanout(int i, CirGateV& out) { return; }
   virtual void RemoveFanout(int i) { return; }
   virtual void RemoveFanin(int i) { return; }
   virtual void setname(const string& str) { return; }
   void setid(const unsigned& id) { _id = id; }
   void setline(const unsigned& l) { _line = l; }

   // for DFS
   bool isGlobalRef() { return (_ref == _globalRef); }
   void setToGlobalRef() { _ref = _globalRef; }
   void setInDfs() const { in_dfs = true; }
   void resetDfs() const { in_dfs = false; }
   static void setGlobalRef() { ++_globalRef; }

   // for simulation
   void setFec(FEC*& c, unsigned i) { _fec = c; _fecid = i; }
   void resetFEC() { _fec = NULL; _fecid = -1; }
   FEC* getFec() const { return _fec; }
   int getFecId() const { return _fecid; }
   void printFEC() const;
   virtual Simtype sim() { return Simtype(0); }
   virtual void setSim(const Simtype& sim) { return; }
   virtual void FindIn(vector<unsigned>& inlist) { return; }

   // other
   virtual bool isAig() const { return false; }

   // for fraig
   Var getVar() const { return _var; }
   void setVar(const Var& v) { _var = v; }
   virtual Var genProofModel(SatSolver*& s) { return -1; }

protected:
   unsigned     _id;
   unsigned     _line;
   Var          _var;
   Simtype      _value;
private:
   mutable bool in_dfs;
   FEC*         _fec;
   int          _fecid;
};

class PIGate : public CirGate
{
public:
  PIGate(const unsigned& id, const unsigned l) : _name("") { _id = id; _line = l; }
  ~PIGate() {}
  void reportGate() const;
  void printGate() const {
    cout << setw(4) << left << getTypeStr() << _id;
    if (_name != "") cout << " (" << _name << ')';
  }
  void setfanout(CirGateV& out) { _fanout.push_back(out); }
  void replaceFanout(int i, CirGateV& out) { cout << "replceFanout called by PIGate, which is not yet implemented" << endl;}
  void RemoveFanout(int i) {
    VList::iterator it = _fanout.begin(); it += i;
    _fanout.erase(it);
  }
  void setSim(const Simtype& sim) { _value = sim; }
  void setname(const string& str) { _name = str; }
  void FindIn(vector<unsigned>& inlist) { if (isGlobalRef()) return; inlist.push_back(_id); setToGlobalRef(); }
  Var genProofModel(SatSolver*& s) {
    if (isGlobalRef()) return _var;
    // cout << "PI " << _id << endl;
    _var = s->newVar();
    setToGlobalRef();
    return _var;
  }
  Simtype sim() { return _value; }
  size_t fanoutNO() {return _fanout.size(); }
  CirGateV getfanout(int i = 0) const { if (_fanout.size() == 0) return CirGateV(0, 0); return _fanout[i];}
  VList getOutList() const { return _fanout; }
  GateType getType() const { return PI_GATE; }
  string getName() const { return _name; }
  string getTypeStr() const { return "PI"; }
private:
  VList _fanout;
  string _name;
};

class POGate : public CirGate
{
public:
  POGate() {}
  POGate(const unsigned& id, const unsigned& l): _name("") { _id = id; _line = l; _fanin.replaceGate(0); }
  ~POGate() {}
  void reportGate() const;
  void printGate() const {
    cout << setw(4) << left << getTypeStr() << _id << ' ';
    if (_fanin.gate()->getType() == UNDEF_GATE) cout << "*";
    if (_fanin.isInv()) cout << '!';
    cout << _fanin.gate()->getId();
    if (_name != "") cout << " (" << _name << ')';
  }
  void setfanin(CirGateV& in) { _fanin = in; }
  void replaceFanin(int i, const CirGate* in, bool inv = false) { _fanin.replaceGate(in); if (inv) _fanin.setToInv(); else _fanin.set(); }
  void RemoveFanin(int i) { _fanin.replaceGate(0); }
  void setname(const string& str) { _name = str; }
  Var genProofModel(SatSolver*& s) { return (_fanin.gate()->genProofModel(s)); }
  Simtype sim() { 
    if (_fanin.isInv()) {
      _value = (~(_fanin.gate()->sim()));
      return _value;
    } else {
      _value = _fanin.gate()->sim();
      return _value;
    }
  }
  size_t faninNO() { return 1; }
  CirGateV getfanin(int i = 0) const { return _fanin; }
  VList getInList() const { VList n; n.push_back(_fanin); return n;}
  GateType getType() const { return PO_GATE; }
  string getTypeStr() const { return "PO"; }
  string getName() const { return _name; }
private:
  CirGateV _fanin;
  string _name;
};

class AIGGate : public CirGate
{
public:
  AIGGate(){}
  AIGGate(const unsigned& id, const unsigned& l) { _id = id; _line = l; reset(); } // fanin & fanout are empty
  ~AIGGate(){}
  void reportGate() const;
  void reset() { _fanin0.replaceGate(0); _fanin1.replaceGate(0); }
  void printGate() const {
    cout << setw(4) << left << getTypeStr() << _id;
    cout << ' ';
    if (_fanin0.gate()->getType() == UNDEF_GATE) cout << "*";
    if (_fanin0.isInv()) cout << '!';
    cout << _fanin0.gate()->getId();
    cout << ' ';
    if (_fanin1.gate()->getType() == UNDEF_GATE) cout << "*";
    if (_fanin1.isInv()) cout << '!';
    cout << _fanin1.gate()->getId();
  }
  void setfanin(CirGateV& in) { 
    if (_fanin0.gate() == NULL) _fanin0 = in;
    else _fanin1 = in;
  }
  void replaceFanin(int i, const CirGate* in, bool inv = false) {
    if (i == 1) { _fanin1.replaceGate(in); if (inv) _fanin1.setToInv(); else _fanin1.set(); }
    else if (i == 0) { _fanin0.replaceGate(in); if (inv) _fanin0.setToInv(); else _fanin0.set(); }
    else assert(false);
  }
  void setfanout(CirGateV& out) { _fanout.push_back(out); }
  void replaceFanout(int i, CirGateV& out) {
    _fanout.erase(_fanout.begin() + i);
    _fanout.insert(_fanout.begin() + i, out);
  }
  void RemoveFanout(int i) {
    VList::iterator it = _fanout.begin(); it += i;
    _fanout.erase(it);
  }
  void RemoveFanin(int i) {
    if (i == 0) { _fanin0.replaceGate(0); }
    else if (i == 1) { _fanin1.replaceGate(0); }
    else assert(false);
  }
  Var genProofModel(SatSolver*& s) {
    if (isGlobalRef()) return _var; 
    // cout << "AIG " << _id << endl;
    _var = s->newVar();
    Var var0 = _fanin0.gate()->genProofModel(s);
    Var var1 = _fanin1.gate()->genProofModel(s);
    s->addAigCNF(_var, var0, _fanin0.isInv(), var1, _fanin1.isInv());
    setToGlobalRef();
    return _var;
  }
  Simtype sim () {
    // if (_id == 11) cout << "is ref: " << isGlobalRef() << endl;
    if (isGlobalRef()) return _value;

    Simtype right = _fanin0.gate()->sim();
    if (_fanin0.isInv()) right = (~right);
    Simtype left = _fanin1.gate()->sim();
    if (_fanin1.isInv()) left = (~left);
    setToGlobalRef();
    _value = (right & left);
    return _value;
  }
  void FindIn(vector<unsigned>& inlist) {
    if (isGlobalRef()) return;
    _fanin0.gate()->FindIn(inlist);
    _fanin1.gate()->FindIn(inlist);
    setToGlobalRef();
  }
  size_t fanoutNO() { return _fanout.size(); }
  size_t faninNO() {
    if (_fanin1.gate() != NULL) return 2;
    else if (_fanin0.gate() != NULL) return 1;
    else return 0;
  }

  CirGateV getfanout(int i = 0) const { if (_fanout.size() == 0) return CirGateV(0, 0); return _fanout[i]; }
  CirGateV getfanin(int i = 0) const { assert(i < 2);
    if (!i) return _fanin0;
    else return _fanin1;
  }

  VList getInList() const {
    VList n;
    if (_fanin0.gate() != NULL) n.push_back(_fanin0);
    if (_fanin1.gate() != NULL) n.push_back(_fanin1);
    return n;
  }
  VList getOutList() const { return _fanout; }

  GateType getType() const { return AIG_GATE; }
  string getTypeStr() const { return "AIG"; }

  bool isAig() const { return true; }
private:
  VList _fanout;
  CirGateV _fanin0;
  CirGateV _fanin1;
};

class CONSTGate : public CirGate
{
public:
  CONSTGate(){}
  CONSTGate(const unsigned& id, const unsigned& l) { _id = 0; _line = l; _value = Simtype(0); }
  ~CONSTGate(){}
  void reportGate() const;
  void printGate() const {
    cout << setw(4) << left << getTypeStr() << _id;
  }
  void setfanin(CirGateV& in) {}
  void setfanout(CirGateV& out) { _fanout.push_back(out); }
  void RemoveFanout(int i) {
    VList::iterator it = _fanout.begin(); it += i;
    _fanout.erase(it);
  }
  Var genProofModel(SatSolver*& s) {
    if (isGlobalRef()) return _var;
    // cout << "const 0 " << endl;
    _var = s->newVar();
    // s->assumeProperty(_var, false);
    setToGlobalRef();
    return _var;
  }
  Simtype sim() { return Simtype(0); }
  size_t fanoutNO() {return _fanout.size(); }
  CirGateV getfanout(int i = 0) const { if (_fanout.size() == 0) return CirGateV(0, 0); return _fanout[i]; }

  VList getOutList() const { return _fanout; }

  GateType getType() const { return CONST_GATE; }
  string getTypeStr() const { return "CONST"; }
private:
  VList _fanout;
};

class UNDEFGate : public CirGate
{
public:
   UNDEFGate(const unsigned id) { _id = id; _line = 0; _value = Simtype(0); }
   ~UNDEFGate() {}
   void reportGate() const;
   CirGateV getfanout(int i) const { if (_fanout.size() == 0) return CirGateV(0, 0); return _fanout[i];}
   VList getOutList() const { return _fanout; }

   void printGate() const { cout << setw(4) << left << "UNDEF" << _id; }
   void setfanout(CirGateV& out) { _fanout.push_back(out); }
   void RemoveFanout(int i) {
     VList::iterator it = _fanout.begin(); it += i;
     _fanout.erase(it);
   }
   size_t fanoutNO() { return _fanout.size(); }

   GateType getType() const { return UNDEF_GATE; }
   string getTypeStr() const { return "UNDEF"; }
private:
   VList _fanout;
};
#endif // CIR_GATE_H
