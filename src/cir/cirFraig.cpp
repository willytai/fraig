/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "myHashSet.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed

size_t getBucknum(const int&);

void
CirMgr::strash()
{
  vector<int> deletelist;
  if (!_dfs_done) DoDfs();
  size_t buckets = getBucknum(_ANDnum);
  HashMap<HashKey, CirGate*> myHash(buckets);
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;

    CirGate* g = NULL;
    size_t key0 = _dfsList[i]->getfanin(0)();
    size_t key1 = _dfsList[i]->getfanin(1)();
    HashKey k(key0, key1);
    
    if (myHash.query(k, g)) { assert(g != NULL);
      merge(g, _dfsList[i], 0);

      cout << "Strashing: " << g->getId() << " merging " << _dfsList[i]->getId() << "..." << endl;
      
      deletelist.push_back(_dfsList[i]->getId());
      if (_dfs_done) _dfs_done = false;
    } 
    else {
      myHash.insert(k, _dfsList[i]);
    }
    for (size_t i = 0; i < deletelist.size(); ++i) {
      delete _list[deletelist[i]]; _list[deletelist[i]] = 0; --_ANDnum;
    }
  }
  if (!_dfs_done) DoDfs();
}

void
CirMgr::fraig()
{
  // createFraigList();
  // SatSolver* solver;
  if (!FECs.size()) return;
  solver = new SatSolver;
  solver->initialize();
  genProofModel(solver);
  
  vector<GateList>         hash;
  vector<vector<bool> >    Inv;
  vector<vector<char*> >  pattern; // something is wrong
  hash.resize(FECs.size());
  Inv.resize(FECs.size());
  pattern.resize(FECs.size());


  for (size_t i = 0; i < _dfsList.size(); ++i) {
    const int id = _dfsList[i]->getFecId();
    if (_dfsList[i]->getType() == PI_GATE) continue;
    if (_dfsList[i]->getType() == PO_GATE) continue;
    if (_dfsList[i]->getType() == CONST_GATE) continue;
    if (id == -1) continue;
    if (!_dfsList[i]->getFec()->size()) continue;

    // cout << '(' << _dfsList[i]->getId() << ')' << endl;

    if (FECs[0][0].second == _list[0] && hash[0].size() == 0) { // insert const0
      hash[0].push_back(_list[0]);
      Inv[0].push_back(false);
    }

    // cout << "check const" << endl;
    // for (int r = 0; r < hash[0].size(); ++r)
    //   cout << ' ' << hash[0][r]->getId();
    // cout << endl;

    bool simkeyInv;
    int temp = -1;
    for (size_t j = 0; j < _dfsList[i]->getFec()->size(); ++j) {
      if (_dfsList[i]->getFec()->at(j).second == _dfsList[i]) {
        simkeyInv = _dfsList[i]->getFec()->at(j).first.isInv();
        temp = j;
      }
    }

    assert(temp != -1);
    FECs[id].at(temp) = FECs[id].at(FECs[id].size()-1);
    FECs[id].pop_back();

    bool result;
    if (!hash[id].size() && !Inv[id].size()) {
      hash[id].push_back(_dfsList[i]);
      Inv[id].push_back(simkeyInv);
      continue;
    }
    else {
      for (size_t f = 0; f < hash[id].size(); ++f) {
        if (Inv[id][f] != simkeyInv)
          result = prove(hash[id][f], 0, _dfsList[i], 1, solver);
        else
          result = prove(hash[id][f], 0, _dfsList[i], 0, solver);
        if (!result) break; // merge
        if (_dfsList[i]->getFec()->size() > 2) {
          if (record(result, solver, pattern, id)) {
            if (FECs[id].size() > 2) {
              updateBySim(hash, Inv, pattern, id);
              merge();
              hash.resize(FECs.size());
              Inv.resize(FECs.size());
              pattern.resize(FECs.size());
            }
          }
        }
      }
      if (result) {
        hash[id].push_back(_dfsList[i]);
        Inv[id].push_back(simkeyInv);
      }
    }
  }

  merge();
  FECs.clear();
  _dfs_done = false;
  // DoDfs();
  _renewfec = true;
  cout << "Updating by UNSAT... Total #FEC Group = 0" << endl;
  delete solver; solver = NULL;
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
inline size_t getBucknum(const int& total) {
  // return size_t(total * 100 / 75);
  return getHashSize(total);
}

inline bool CirMgr::prove(CirGate* g, bool ginv, CirGate* h, bool hinv, SatSolver*& s) {
  cout << '\r' << "Proving(" << g->getId() << ", " << h->getId() << ")..." << flush;
  s->assumeRelease();
  if (_list[0]->getVar() == -1) _list[0]->setVar(s->newVar());
  s->assumeProperty(_list[0]->getVar(), false);

  Var newV = s->newVar();
  s->addXorCNF(newV, g->getVar(), ginv, h->getVar(), hinv);
  s->assumeProperty(newV, true);
  bool result = s->assumpSolve();
  if (!result) {
    ToBeMerge.push_back(MergeNode(g, h));
    mergePhase.push_back(hinv);
  }
  return result;
}

void CirMgr::merge() {
  assert(ToBeMerge.size() == mergePhase.size());
  for (size_t i = 0; i < ToBeMerge.size(); ++i) {
    merge(ToBeMerge[i].first, ToBeMerge[i].second, mergePhase[i]);
    cout << '\r' << "Fraig: " << ToBeMerge[i].first->getId() << " merging ";
    if (mergePhase[i]) cout << '!';
    cout << ToBeMerge[i].second->getId() << "..." << endl;
    // delete _list[ToBeMerge[i].second->getId()];
    _list[ToBeMerge[i].second->getId()] = 0; --_ANDnum;
  }
  ToBeMerge.clear(); mergePhase.clear();
}

void CirMgr::merge(CirGate* g, CirGate* h, int check = 2) { // g merges h, which means h will be replace by g
  if (check == 2) { // for merging after simulation (fraig) better not do this
    if (g->value() == h->value()) check = 0;
    else if (g->value() == ~(h->value())) check = 1;
    else assert(false);
  }

  VList h_out = h->getOutList();
  for (size_t i = 0; i < h_out.size(); ++i) {
    if (check) h_out[i].changePhase();
    g->setfanout(h_out[i]);
    for (int u = 0; u < 2; ++u) {
      if (h_out[i].gate()->getfanin(u).gate() != h) continue;
      h_out[i].gate()->replaceFanin(u, g, h_out[i].isInv());
    }
  }
  for (int id = 0; id < 2; ++id) {
    for (int i = 0; i < h->getfanin(id).gate()->getOutList().size(); ++i) {
      if (h->getfanin(id).gate()->getOutList()[i].gate() != h) continue;
      h->getfanin(id).gate()->RemoveFanout(i);
    }
  }
}

inline bool CirMgr::record(const bool& result, SatSolver*& s, vector<vector<char*> >& pat, const size_t& id) {
  if (!result) return false;
  // stringstream ss;
  char* sim = new char[_PI.size()];
  for (size_t i = 0; i < _PI.size(); ++i) {
    Var var = _PI[i]->getVar();
    if (var == -1)
      sim[i] = '0';
      // ss << 0;
    else
      sim[i] = var + '0';
      // ss << (s->getValue(var));
  }
  // pat[id].push_back(ss.str());
  pat[id].push_back(sim);
  if (pat[id].size() > 255) return true;
  else return false;
}

inline void CirMgr::initVar() {
  for (size_t i = 0; i < _list.size(); ++i) {
    if (_list[i] == NULL) continue;
    if (_list[i]->getType() != AIG_GATE || _list[i]->getType() != PI_GATE) continue;
    _list[i]->setVar(-1);
  }
}

inline void CirMgr::updateBySim(vector<GateList>& hash, vector<vector<bool> >& Inv, vector<vector<char*> >& pat, const int& id) {
  // assert(pat[id][0].size() == _PI.size());
  int top = pat[id].size() / 64;
  while (top > 0) {

  for (size_t i = 0; i < _PI.size(); ++i) {
    Simtype s = Simtype(0);
    for (size_t j = (top-1)*64; j < top*64; ++j) {
      s << 1;
      s += Simtype(pat[id][j][i] - '0');
    }
    _PI[i]->setSim(s);
  }
  
  // pat[id].clear();
  for (size_t i = 0; i < pat[id].size(); ++i) {
    delete pat[id][i];
  } pat[id].clear();

  HashMap<SimKey, CirGate*> newGrps(getHashSize(FECs[id].size()));
  for (size_t i = 0; i < FECs[id].size(); ++i) {
    CirGate::setGlobalRef();
    Simtype value = FECs[id][i].second->sim();
    FECs[id][i].first.update(value);
    newGrps.insert(FECs[id][i]);
  }
  // for (size_t i = 0; i < hash[id].size(); ++i) {
  //   CirGate::setGlobalRef();
  //   Simtype value = hash[id][i]->sim();
  //   SimNode node(SimKey(value), hash[id][i]);
  //   newGrps.insert(node);
  //   hash[id].clear(); Inv[id].clear();
  // }
  vector<FEC> valid; newGrps.valid(valid);
  if (valid.size() == 1) {  if (top > 0) {--top; continue;} else return; }
  FEC check = FECs[id];
  FECs[id] = valid[0];
  for (size_t i = 1; i < valid.size(); ++i) {
    int fecid = FECs.size();
    FECs.push_back(valid[i]);
    assert(valid[i].size() > 1);
    for (size_t f = 0; f < valid[i].size(); ++f) {
      FEC* t = &FECs[fecid];
      valid[i][f].second->setFec(t ,fecid);
    }
  }
  cout << '\r' << "Updating by SAT... Total #FEC Group = " << FECs.size() << endl;

  --top;
  }
}

void CirMgr::reportResult(const SatSolver& solver, bool result)
{
   solver.printStats();
   cout << (result ? "SAT" : "UNSAT") << endl;
   if (result) {
      for (size_t i = 0, n = _PI.size(); i < n; ++i)
         cout << solver.getValue(_PI[i]->getVar()) << endl;
   }
}

void CirMgr::genProofModel(SatSolver*& s) { // recusively generate model from po
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() == PI_GATE) {
      Var v = s->newVar();
      _dfsList[i]->setVar(v);
    }
    else if (_dfsList[i]->getType() == CONST_GATE) {
      Var v = s->newVar();
      _dfsList[i]->setVar(v);
    }
    else if (_dfsList[i]->getType() == AIG_GATE) {
      Var v = s->newVar();
      CirGateV v0 = _dfsList[i]->getfanin(0);
      CirGateV v1 = _dfsList[i]->getfanin(1);
      _dfsList[i]->setVar(v);
      s->addAigCNF(v, v0.gate()->getVar(), v0.isInv(), v1.gate()->getVar(), v1.isInv());
    }
  }
}