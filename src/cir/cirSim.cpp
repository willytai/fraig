/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

#define MAX_BIT 64
#define Simtype_MAX 0xffffffffffffffff

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
enum CirSimError{
  PAT_NUM_MISMATCH,
  CHARACTER,
};
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static string errPat;
static char errChar;
static int correct_num;
static int limit;
static int leave;

static bool parseSimError(CirSimError err) {
  cout << endl;
  switch(err) {
    case PAT_NUM_MISMATCH:
      cerr << "Error: Pattern(" << errPat << ") length(" << errPat.size()
           << ") does not match the number of inputs(" << correct_num << ")"
           << " in a circuit!!" << endl; break;
    case CHARACTER:
      cerr << "Error: Pattern(" << errPat << ") contains a non-0/1 character(\'"
           << errChar << "\')." << endl; break;
  }
  cout << "0 patterns simulated." << endl;
  return false;
}
static void getlimit(const size_t& size) {
  if (size <= 1) cout << "wrong size: " << size << endl;
  assert(size > 1);
  if (size < 10) limit = 3;
  else limit = 5;
}
/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   unsigned patcount = 0;
   leave = 20;
   if (!FECs.size()) {
     if (!InitFec()) return;
     cout << '\r' << "Total #FEC Group = " << FECs.size() << flush;
     while (_PI.size() > 1000) {
       genPattern();
       sim();
       ++patcount;
       cout << '\r' << "Total #FEC Group = " << FECs.size() << flush;
       if (simEachFec(FECs.size())) break;
     }
   }
   for (size_t i = 0; i < FECs.size(); ++i) {
     cout << '\r' << "Total #FEC Group = " << FECs.size() << flush;
     simFEC(i); ++patcount;
   }
   cout << '\r' << patcount*MAX_BIT << " patterns simulated." << endl;
   FECsort();
   LinkFecToGate();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  if (!FECs.size()) {
    if (!InitFec()) return;
  }
  string buf; _simPat.clear(); _patterns = 0;
  int count = 0;
  while (getline(patternFile, buf)) {
    if(buf == "") continue;
    if (!checkSim(buf)) return;
    ++_patterns;
    if (!_simPat.size()) _simPat.resize(buf.size());
    setSimPattern(buf);
    if (_patterns == MAX_BIT) {
      sim();
      Initsim(); ++count; _patterns = 0;
    }
  }
  if (_patterns) sim();
  cout << '\r' << count*MAX_BIT + _patterns << " patterns simulated." << endl;
  FECsort();
  LinkFecToGate();
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
inline void CirMgr::genPattern() {
   Initsim();
   for (size_t i = 0; i < _PI.size(); ++i) {
     Simtype test = Simtype(rnGen(0));
     for (int j = 0; j < 64; ++j) {
       test = test << 1;
       test += Simtype(rnGen(2));
     }
     _simPat[i] = test;
   }
}

inline void CirMgr::setPat() {
  for (size_t i = 0; i < _PI.size(); ++i) {
    _PI[i]->setSim(_simPat[i]);
  }
}

inline void CirMgr::simFEC(size_t& i) {
  getlimit(FECs[i].size());
  if (FECs[i].size() < 5 && _PI.size() > 2000) specialFECsim(i);
  else {
    genPattern();
    setPat();
  }
  size_t check = FECs.size();
  for (size_t j = 0; j < FECs[i].size(); ++j) {
    CirGate::setGlobalRef();
    // cout << "check: " << FECs[i][j].second->getId() << endl;
    FECs[i][j].second->sim();
  }
  updateFec(i);
  if (check > FECs.size()) --i;
  else if (FECnotChange[i] < limit) --i;
}

inline void CirMgr::specialFECsim(const size_t& i) {
  vector<unsigned> InID;
  CirGate::setGlobalRef();
  FECs[i][0].second->FindIn(InID);
  for (size_t k = 0; k < InID.size(); ++k) {
    Simtype test = Simtype(rnGen(0));
    for (int j = 0; j < 64; ++j) {
      test = test << 1;
      test += Simtype(rnGen(2));
    }
    _list[InID[k]]->setSim(test);
    assert(_list[InID[k]]->getType() == PI_GATE);
  }
  limit = 3;
}

bool CirMgr::checkSim(const string& buf) {
  if (buf.size() != _PI.size()) {
    errPat = buf; correct_num = _PI.size();
    return parseSimError(PAT_NUM_MISMATCH);
  }
  for (size_t i = 0; i < buf.size(); ++i) {
    if (buf[i] != '0' && buf[i] != '1') {
      errPat = buf; errChar = buf[i];
      return parseSimError(CHARACTER);
    }
  }
  return true;
}

bool CirMgr::simEachFec(size_t size) {
  if (Err.size() != 3) Err.resize(3);
  if (size < Err[1] || Err[1] == 0) {
    ++Err[2];
    Err[1] = size;
    Err[0] = 0;
  } else ++Err[0];
  if (Err[0] > leave && Err[0] > 2) return true;
  if (Err[2] > 10) {
    if (leave > 0) leave -= 3;
    return false;
  }
  return false;
}

void CirMgr::sim() {
  for (size_t i = 0; i < _PI.size(); ++i) {
    _PI[i]->setSim(_simPat[i]);
  }
  CirGate::setGlobalRef();
  _simResult.clear(); _simResult.resize(_PO.size());
  for (size_t i = 0; i < _PO.size(); ++i) {
    _simResult[i] = (_PO[i]->sim());
  }
  if(_simLog != NULL) SimWrite();
  updateFec();
  cout << '\r' << "Total #FEC Group = " << FECs.size() << flush;
}

inline void CirMgr::Initsim() {
  if (_simPat.size() != _PI.size()) _simPat.resize(_PI.size());
  for (size_t i = 0; i < _simPat.size(); ++i)
    _simPat[i] = 0;
  for (size_t i = 0; i < _simResult.size(); ++i)
    _simResult[i] = 0;
}

inline void CirMgr::setSimPattern(const string& buf) {
  for (size_t i = 0; i < buf.size(); ++i) {
    Simtype tmp = buf[i] - '0';
    _simPat[i] += (tmp << _patterns-1);
  }
}

inline bool CirMgr::InitFec() {
  FECs.clear();
  FECnotChange.clear();
  Err.clear();
  if (!_dfs_done) DoDfs();
  SimKey k(0);
  FEC newFec;
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;
    newFec.push_back(SimNode(k, _dfsList[i]));
  }
  newFec.push_back(SimNode(0, _list[0]));
  if (newFec.size() > 1) {
    FECs.push_back(newFec);
    FECnotChange.push_back(0);
    return true;
  } return false;
}

inline void CirMgr::updateFec(int id) {
    HashMap<SimKey, CirGate*> newGrps(getHashSize(FECs[id].size()));
    // check if fec has different values
    bool insert = false;
    FECs[id][0].first.update(FECs[id][0].second->value());
    Simtype boost = (FECs[id][0].first)();
    for (size_t j = 1; j < FECs[id].size(); ++j) {
      FECs[id][j].first.update(FECs[id][j].second->value());
      if ((FECs[id][j].first)() != boost && (FECs[id][j].first)() != ~boost) insert = true;
    }
    if (!insert) { ++FECnotChange[id]; return;}
    for (size_t j = 0; j < FECs[id].size(); ++j) {
      newGrps.insert(FECs[id][j]);
    }
    vector<FEC> valid;
    newGrps.valid(valid); // collecting valid groups
    
    if (valid.size()) { FECs[id] = valid[0]; FECnotChange[id] = 0; }
    else {
      // vector<FEC>::iterator it = FECs.begin()+i;
      // vector<int>::iterator itt = FECnotChange.begin()+i;
      // FECs.erase(it); --i; FECnotChange.erase(itt);
      FECs[id] = FECs[FECs.size()-1]; FECs.pop_back();
      FECnotChange[id] = FECnotChange[FECnotChange.size()-1]; FECnotChange.pop_back();
      --id;
    }
    
    for (size_t k = 1; k < valid.size(); ++k) {
      // FECs.push_back(valid[k]); FECnotChange.push_back(0);
      FECs.insert(FECs.begin()+id+1, valid[k]);
      FECnotChange.insert(FECnotChange.begin()+id+1, 0); ++id;
    }
}

inline void CirMgr::updateFec() {
  for (size_t i = 0; i < FECs.size(); ++i) {
    HashMap<SimKey, CirGate*> newGrps(getHashSize(FECs[i].size()));
    // check if fec has different values
    bool insert = false;
    FECs[i][0].first.update(FECs[i][0].second->value());
    Simtype boost = (FECs[i][0].first)();
    for (size_t j = 1; j < FECs[i].size(); ++j) {
      FECs[i][j].first.update(FECs[i][j].second->value());
      if ((FECs[i][j].first)() != boost && (FECs[i][j].first)() != ~boost) insert = true;
    }
    if (!insert) { ++FECnotChange[i]; continue; }
    for (size_t j = 0; j < FECs[i].size(); ++j) {
      newGrps.insert(FECs[i][j]);
    }
    vector<FEC> valid;
    newGrps.valid(valid); // collecting valid groups

    if (valid.size()) { FECs[i] = valid[0]; FECnotChange[i] = 0; }
    else {
      // vector<FEC>::iterator it = FECs.begin()+i;
      // vector<int>::iterator itt = FECnotChange.begin()+i;
      // FECs.erase(it); --i; FECnotChange.erase(itt);
      FECs[i] = FECs[FECs.size()-1]; FECs.pop_back();
      FECnotChange[i] = FECnotChange[FECnotChange.size()-1]; FECnotChange.pop_back();
      --i;
    }
    
    for (size_t k = 1; k < valid.size(); ++k) {
      // FECs.push_back(valid[k]); FECnotChange.push_back(0);
      FECs.insert(FECs.begin()+i+1, valid[k]);
      FECnotChange.insert(FECnotChange.begin()+i+1, 0); ++i;
    }
  }
}

void CirMgr::LinkFecToGate() {
  for (size_t i = 0; i < FECs.size(); ++i) {
    FEC* f = &FECs[i];
    for (size_t j = 0; j < FECs[i].size(); ++j) {
      FECs[i][j].second->setFec(f, i);
    }
  }
}

void CirMgr::SimWrite() {
  char* endline = new char('\n');
  char* space = new char(' ');
  char* tmp = new char[_simPat.size()];
  char* tmp1 = new char[_simResult.size()];
  for (int i = _patterns; i > 0; --i) {
    for (size_t j = 0; j < _simPat.size(); ++j) { 
      Simtype keyed = _simPat[j] & Simtype(KEY);
      _simPat[j] = _simPat[j] >> 1;
      tmp[j] = (keyed) + '0';
    }
    _simLog->write(tmp, _simPat.size());
    _simLog->write(space, 1);
    for (size_t k = 0; k < _simResult.size(); ++k) {
      Simtype keyedcode = _simResult[k] & Simtype(KEY);
      _simResult[k] = _simResult[k] >> 1;
      tmp1[k] = (keyedcode + '0');      
    }
    _simLog->write(tmp1, _simResult.size());
    _simLog->write(endline, 1);
  }
  delete endline; delete space; delete tmp; delete tmp1;
}

inline void CirMgr::FECsort() {
  if (FECs.size() == 0) return;
  for (size_t i = 0; i < FECs.size(); ++i) {
    for (size_t j = 0; j < FECs[i].size()-1; ++j) {
      for (size_t k = j+1; k < FECs[i].size(); ++k) {
        if (FECs[i][j].second->getId() > FECs[i][k].second->getId())
          ::swap(FECs[i][j], FECs[i][k]);
      }
    }
  }
  for (size_t i = 0; i < FECs.size()-1; ++i) {
    for (size_t j = i+1; j < FECs.size(); ++j) {
      if (FECs[i][0].second->getId() > FECs[j][0].second->getId())
        ::swap(FECs[i], FECs[j]);
    }
  }
}

void CirMgr::testPI() {
  stringstream ss;
  for (int i = 0; i < 2; ++i) {
    genPattern();
    for (int j = 0; j < 64; ++j) {
      ss.str("");
      for (int k = 0; k < _simPat.size(); ++k) {
        ss << (_simPat[k] & Simtype(1));
        _simPat[k] = _simPat[k] >> 1;
      }
      cout << ss.str() << endl;
    }    
  }
  getlimit(_PI.size());
}