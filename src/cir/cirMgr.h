/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

// #include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

class AigVs
{
public:
  AigVs() {}
  ~AigVs() {}
  void set(int i, int j, int k) { _ref.push_back(i), _in0.push_back(j), _in1.push_back(k); }
  size_t size() { return _ref.size(); }
  int getRef(int i) { return _ref[i]; }
  int getIn(int id, int i) { if (!i) return _in0[id]; return _in1[id]; }
private:
  vector<int> _ref, _in1, _in0;
};


class CirMgr
{
public:
   CirMgr() : _dfs_done(false), solver(NULL), _renewfec(false) {}
   ~CirMgr() {
     for (size_t i = 0; i < _PO.size(); ++i) {
       if (_PO[i] != NULL) {
         delete _PO[i]; _PO[i] = NULL;
       }
     } _PO.clear();

     for (size_t i = 0; i < _list.size(); ++i) {
       if (_list[i] != NULL) {
         delete _list[i]; _list[i] = NULL;
       }
     } _list.clear(); _dfsList.clear(); _PI.clear(); comment.clear();
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const;

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs();
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

   // member functions about debigging
   void testPI();
private:  
   ofstream           *_simLog;
   SatSolver          *solver;

   int                 _PInum;
   int                 _POnum;
   int                 _MaxVarnum;
   int                 _Latchnum;
   int                 _ANDnum;
   int                 _patterns;

   GateList            _PO;
   GateList            _PI;
   GateList            _list;
   GateList            _fltList;
   GateList            _def_not_use_list;
   mutable GateList    _dfsList;
   
   mutable bool        _dfs_done;
   mutable bool        _renewfec;
   
   vector<Simtype>     _simPat;
   vector<Simtype>     _simResult;
   vector<string>      comment;

   
   vector<FEC>         FECs;
   vector<int>         FECnotChange;

   vector<int>         Err;
   vector<MergeNode>   ToBeMerge;
   vector<bool>        mergePhase;

   inline CirGate* setGate(const unsigned&, int&, const GateType&);
   inline void setinput(const unsigned&, CirGate*);
   void connect();
   void DoDfs() const;
   void dfs(const VList&) const;
   
   
   // =====================================================================
   void merge();
   void merge(CirGate*, CirGate*, int); 
   // the last int is to determine which kind of merge to be performed
   // if int = 0 these two gates have same value
   // if int = 1 these two gates have inverted value
   // if int = 2 to be determined
   // =====================================================================

   // private sweeping method
   void ModifyOut();
   
   // private method for optimization

   // pravate method for simulation
   inline void setSimPattern(const string& buf);
   inline void Initsim();
   inline bool InitFec();
   inline void updateFec();
   inline void updateFec(int);
   inline void FECsort();
   inline void genPattern();
   inline void setPat();
   inline void simFEC(size_t&);
   inline void specialFECsim(const size_t&);
   void LinkFecToGate();
   void SimWrite();
   void sim();
   bool checkSim(const string&);
   void printFECnum() const;

   // private method for fraig
   void genProofModel(SatSolver*& s);
   void reportResult(const SatSolver& solver, bool result);
   bool simEachFec(size_t);

   //private method for fraig
   inline bool prove(CirGate*, bool, CirGate*, bool, SatSolver*&);
   inline bool record(const bool&, SatSolver*&, vector<vector<char*> >&, const size_t&);
   inline void renewFec() const;
   inline void initVar();
   inline void updateBySim(vector<GateList>& hash, vector<vector<bool> >&, vector<vector<char*> >&, const int&);
};

#endif // CIR_MGR_H
