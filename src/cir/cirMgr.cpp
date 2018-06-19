/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
// static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/

inline CirGate* CirMgr::setGate(const unsigned& l, int& id, const GateType& type) {
  CirGate* mgr;

  switch (type) {
    case PI_GATE:
      id /= 2;
      mgr = new PIGate(id, l);
      _list[id] = mgr;
      _PI.push_back(mgr);
      break;
    case PO_GATE:
      mgr = new POGate(id ,l);
      _PO.push_back(mgr);
      break;
    case AIG_GATE:
      id /= 2;
      if (!_list[id]) {
        mgr = new AIGGate(id, l);
        _list[id] = mgr;
      }
      else assert(0);//_list[id]->settype(AIG_GATE);
      break;
    default:
      cerr << "Something's wrong in cirMgr::setGate(...)";
      assert(false);
  }
  return mgr;
}

bool
CirMgr::readCircuit(const string& fileName)
{
   ifstream file;
   file.open(fileName.c_str());
   if (!file.is_open()) { cout << "Cannot open design \"" << fileName << "\"!!" << endl; return false; }

   bool storecomment = false;
   int checkin = 0;
   lineNo = 1;
   string buf;
   AigVs aigin;

   while (getline(file, buf)) {

     if (buf == "c") storecomment = true;

     if (storecomment) {
      comment.push_back(buf);
      continue;
     }

     vector<string> str;
     string tok;
     size_t n = myStrGetTok(buf, tok);
     while (tok.size()) {
       str.push_back(tok);
       n = myStrGetTok(buf, tok, n);
     }

     if (str[0] == "aag") { // header line
       if (myStr2Int(str[1], _MaxVarnum)) {}
       if (myStr2Int(str[2], _PInum)) {}
       if (myStr2Int(str[3], _Latchnum)) {}
       if (myStr2Int(str[4], _POnum)) {}
       if (myStr2Int(str[5], _ANDnum)) {}
       _list.resize(_MaxVarnum + 1);
       _list[0] = new CONSTGate(0, 0);
     }
     else if (str.size() == 1) { // PI & PO
       checkin++;
       int id;
       myStr2Int(str[0], id);
       GateType type = ((checkin <= _PInum) ? PI_GATE : PO_GATE);
       setGate(lineNo, id, type);
     }
     else if (str[0][0] == 'i' || str[0][0] == 'o') {
       myStrGetTok(buf, str[1], str[0].length() + 1, '\n');
       char c = str[0][0];
       str[0] = str[0].substr(1);
       int id;
       myStr2Int(str[0], id);
       if (c == 'i') _PI[id]->setname(str[1]);
       else if (c == 'o') _PO[id]->setname(str[1]);
     }
     else if (str.size() == 3) {
       int id, id1, id2;
       myStr2Int(str[0], id);
       myStr2Int(str[1], id1);
       myStr2Int(str[2], id2);
       aigin.set(id, id1, id2);
       GateType type = AIG_GATE;
       setGate(lineNo, id, type);
     }
     ++lineNo;
   }
   for (size_t i = 0; i < aigin.size(); ++i) {

    int id, id1, id2;
    id = aigin.getRef(i);
    id1 = aigin.getIn(i, 0);
    id2 = aigin.getIn(i, 1);

    id /= 2;
    CirGate* thisgate = _list[id];
    assert(thisgate != 0);
    setinput(id1, thisgate);
    setinput(id2, thisgate);
   }
   connect();
   DoDfs();
   return true;
}

void CirMgr::connect() {
  for (size_t i = 0; i < _PO.size(); ++i) {
    if (_PO[i]->getId() == 0 || _PO[i]->getId() == 1) {
      CirGateV v(_list[0], _PO[i]->getId());
      CirGateV v2(_PO[i], _PO[i]->getId());

      _PO[i]->setfanin(v);
      _list[0]->setfanout(v2);
      _PO[i]->setid(_MaxVarnum + 1 + i);
      continue;
    }

    size_t phase = _PO[i]->getId() % 2;
    unsigned id = _PO[i]->getId() / 2;

    if (!_list[id]) {
      _list[id] = new UNDEFGate(id);
    }
    CirGateV v2(_PO[i], phase);
    _list[id]->setfanout(v2);
    CirGate* tmp = _list[id];
    CirGateV v(tmp, phase);

    _PO[i]->setfanin(v);
    _PO[i]->setid(_MaxVarnum + 1 + i);
  }
}

inline void CirMgr::setinput(const unsigned& a, CirGate* gate) {assert(gate != NULL);
  int id = a / 2;
  size_t phase = a % 2;
  if (!id) {
    CirGateV v2(gate, phase);
    _list[0]->setfanout(v2);
    CirGateV v(_list[0], phase);

    gate->setfanin(v);
    return;
  }

  if (!_list[id]) {
    id = a;
    _list[id/2] = new UNDEFGate(id/2);
    CirGateV v2(gate, phase);
    _list[id/2]->setfanout(v2);
    CirGateV v(_list[id/2], phase);
    gate->setfanin(v);
    return;
  }
  CirGateV v2(gate, phase);
  _list[id]->setfanout(v2);
  CirGateV v(_list[id], phase);
  gate->setfanin(v);
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
  cout << endl;
  cout << "Circuit Statistics" << endl;
  cout << "==================" << endl;
  cout << "  PI" << setw(12) << right << _PInum << endl;
  cout << "  PO" << setw(12) << right << _POnum << endl;
  cout << "  AIG" << setw(11) << right << _ANDnum << endl;
  cout << "------------------" << endl;
  cout << "  Total" << setw(9) << right << _PInum + _POnum + _ANDnum << endl;
}

unsigned CirGate::_globalRef = 0;

void
CirMgr::printNetlist() const
{
  cout << endl;
  if (!_dfs_done) DoDfs();

  for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
     cout << "[" << i << "] ";
     _dfsList[i]->printGate();
     cout << endl;
  }
}

void CirMgr::DoDfs() const {
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    _dfsList[i]->resetDfs();
    _dfsList[i] = NULL;
  }
  _dfsList.resize(0);
  CirGate::setGlobalRef();
    VList o;
    for (size_t i = 0; i < _PO.size(); ++i) {
      _PO[i]->setInDfs();
      CirGateV v(_PO[i], 0);
      o.push_back(v);
    }
    dfs(o);
    _dfs_done = true;
}

void CirMgr::dfs(const VList& srcList) const {
  for (size_t i = 0; i < srcList.size(); ++i) {
    CirGate* g = srcList[i].gate();
    if (g->isGlobalRef()) continue;
    
    if (g->getType() == UNDEF_GATE) {
      g->setInDfs(); continue;
    }

    if (g->getInList().size() == 0)
      g->setToGlobalRef();
    else dfs(g->getInList());

    _dfsList.push_back(g);
    if (g->getType() != PO_GATE) _list[g->getId()]->setInDfs();

    if (!(g->isGlobalRef())) g->setToGlobalRef();
  }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (size_t i = 0; i < _PI.size(); ++i) {
     cout << ' ' << _PI[i]->getId();
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = 0; i < _PO.size(); ++i) {
    cout << ' ' << _PO[i]->getId();
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
  int countf = 0;
  for (size_t i = 1; i < _list.size(); ++i) {
    if (_list[i] == NULL) continue;
    if (_list[i]->getType() == UNDEF_GATE) continue;
    if (_list[i]->getType() == PI_GATE) continue;
    if (!countf && _list[i]->faninNO() < 2) {
      ++countf;
      cout << "Gates with floating fanin(s):";
    }
    else if (!countf && _list[i]->getfanin(0).gate()->getType() == UNDEF_GATE) {
      ++countf;
      cout << "Gates with floating fanin(s):";
    }
    else if (!countf && _list[i]->getfanin(1).gate()->getType() == UNDEF_GATE) {
      ++countf;
      cout << "Gates with floating fanin(s):";
    }

    if (_list[i]->faninNO() < 2) {
      cout << " " << _list[i]->getId();
    }
    else if (_list[i]->getfanin(0).gate()->getType() == UNDEF_GATE) {
      cout << " " << _list[i]->getId();
    }
    else if (_list[i]->getfanin(1).gate()->getType() == UNDEF_GATE) {
      cout << " " << _list[i]->getId();
    }
  }
  for (size_t i = 0; i < _PO.size(); ++i) {
    if (_PO[i]->getfanin().gate()->getType() == UNDEF_GATE) {
      if (!countf) {
        ++countf;
        cout << "Gates with floating fanin(s):";
      }
      cout << " " << _PO[i]->getId();
    }
  }
  if (countf) cout << endl;
  int countd = 0;
  for (size_t i = 1; i < _list.size(); ++i) {
    if (_list[i] == NULL) continue;
    if (!countd && _list[i]->getfanout().gate() == NULL) {
      ++countd;
      cout << "Gates defined but not used  :";
    }

    if (_list[i]->getfanout().gate() == NULL) {
      cout << " " << _list[i]->getId();
    }
  }
  if (countd) cout << endl;
}

void
CirMgr::printFECPairs()
{
  if (!FECs.size()) return;
  // FECsort();
  for (size_t i = 0; i < FECs.size(); ++i) {
    cout << "[" << i << "]";
    bool check = FECs[i][0].first.isInv();
    for (size_t j = 0; j < FECs[i].size(); ++j) {
      cout << ' ';
      if (FECs[i][j].first.isInv() && !check) cout << '!';
      else if (!(FECs[i][j].first.isInv()) && check) cout << '!';
      cout << FECs[i][j].second->getId();
    }
    cout << endl;
  }
}

void
CirMgr::writeAag(ostream& outfile) const
{
  outfile << "aag" << ' ' << _MaxVarnum << ' ' << _PInum << ' ' << _Latchnum << ' ' << _POnum << ' ' << _ANDnum << endl;
  for (size_t i = 0; i < _PI.size(); ++i) {
    outfile << 2*(_PI[i]->getId()) << endl;
  }
  for (size_t i = 0; i < _PO.size(); ++i) {
    int id = _PO[i]->getfanin().gate()->getId();
    id *= 2;
    if (_PO[i]->getfanin().isInv()) ++id;
    outfile << id << endl;
  }

  if (!_dfs_done) { DoDfs(); }

  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;
    int in1 = _dfsList[i]->getfanin(0).gate()->getId();
    int in2 = _dfsList[i]->getfanin(1).gate()->getId();
    int in11 = (_dfsList[i]->getfanin(0).isInv() ? 1 : 0);
    int in22 = (_dfsList[i]->getfanin(1).isInv() ? 1 : 0);
    outfile << 2*(_dfsList[i]->getId()) << ' ' << 2*in1 + in11 << ' ' << 2*in2 + in22 << endl;
  }

  for (size_t i = 0; i < _PI.size(); ++i) {
    if (_PI[i]->getName() != "") outfile << 'i' << i << ' ' << _PI[i]->getName() << endl;
  }
  for (size_t i = 0; i < _PO.size(); ++i) {
    if (_PO[i]->getName() != "") outfile << 'o' << i << ' ' << _PO[i]->getName() << endl;
  }
  for (size_t i = 0; i < comment.size(); ++i) {
    outfile << comment[i] << endl;
  }
}

void dfswrite(const VList& srcList, unsigned& pi, GateList& new_dfs_aig, GateList& new_dfs_pi) {
  for (size_t i = 0; i < srcList.size(); ++i) { 
    CirGate* g = srcList[i].gate();
    if (g->isGlobalRef()) continue;
    if (g->getType() == UNDEF_GATE) { continue; }

    if (g->getInList().size() == 0)
      g->setToGlobalRef();
    else dfswrite(g->getInList(), pi, new_dfs_aig, new_dfs_pi);

    if (g->getType() == PI_GATE) { ++pi;
      for (size_t i = 0; i < new_dfs_pi.size(); ++i) {
        if (new_dfs_pi[i] == 0) continue;
        if (new_dfs_pi[i]->getId() == g->getId()) {
          new_dfs_pi[i] = 0; break;
        }
      }
    }
    else if (g->getType() == AIG_GATE) new_dfs_aig.push_back(g);

    if (!(g->isGlobalRef())) g->setToGlobalRef();
  }
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{

  // vector<unsigned> ID;
  // CirGate::setGlobalRef();
  // g->FindIn(ID);
  // for (int i = 0; i < ID.size(); ++i) cout << ID[i] << endl;

  unsigned pi = 0;
  GateList new_dfs_aig;
  GateList new_dfs_pi = _PI;
  VList po; po.push_back(CirGateV(g, 0));
  CirGate::setGlobalRef();
  dfswrite(po, pi, new_dfs_aig, new_dfs_pi);

  outfile << "aag " << g->getId() << ' ' << pi << " 0 1 " << new_dfs_aig.size() << endl;
  for (size_t i = 0; i < new_dfs_pi.size(); ++i) {
    if (new_dfs_pi[i] == 0)
      outfile << 2*(_PI[i]->getId()) << endl;
  }
  outfile << 2*(g->getId()) << endl;
  for (size_t i = 0; i < new_dfs_aig.size(); ++i) {
    unsigned id0 = (new_dfs_aig[i]->getfanin(0).isInv() ? 1 : 0);
    unsigned id1 = (new_dfs_aig[i]->getfanin(1).isInv() ? 1 : 0);
    id0 += 2*(new_dfs_aig[i]->getfanin(0).gate()->getId());
    id1 += 2*(new_dfs_aig[i]->getfanin(1).gate()->getId());
    outfile << 2*(new_dfs_aig[i]->getId()) << ' ' << id0 << ' ' << id1 << endl;
  }
  outfile << "o0 " << g->getId() << endl << 'c' << endl << "Write gate (" << g->getId() << ')' << endl;
}

CirGate* CirMgr::getGate(unsigned gid) const {
  if (_renewfec) {
    _renewfec = false;
    renewFec();
  }
  if (gid <= unsigned(_MaxVarnum)) return _list[gid];
  for (size_t i = 0; i < _PO.size(); i++) {
    if (gid == _PO[i]->getId()) return _PO[i];
  }
  return 0;
}

inline void CirMgr::renewFec() const {
  if (!_dfs_done) DoDfs();
  for (size_t i = 0; i < _dfsList.size(); ++i)
    _dfsList[i]->resetFEC();
  _list[0]->resetFEC();
}