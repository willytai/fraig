/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  if (!_dfs_done) DoDfs();
  ModifyOut();
  for (size_t i = 1; i < _list.size(); ++i) {
    if (_list[i] == NULL) continue;
    if (_list[i]->getType() == PI_GATE) continue;
    if (_list[i]->getType() == PO_GATE) continue;
    if (_list[i]->InDfs()) continue;

    cout << "Sweeping: " << _list[i]->getTypeStr() << "(" << _list[i]->getId() << ") removed..." << endl;
    if (_list[i]->getType() == AIG_GATE) --_ANDnum;
    // delete _list[i];
    _list[i] = NULL;
    _dfs_done = false;  
  }
}

/***************************************************/
/*   Private member functions about sweeping       */
/***************************************************/
void CirMgr::ModifyOut() {
  for (size_t i = 0; i < _PI.size(); ++i) {
    for (size_t j = 0; j < _PI[i]->getOutList().size(); ++j) {
      if (!_PI[i]->getOutList().at(j).gate()->InDfs()) { _PI[i]->RemoveFanout(j); --j; }
    }
  }

  for (size_t i = 0; i < _list[0]->getOutList().size(); ++i)
    if (!_list[0]->getOutList().at(i).gate()->InDfs()) { _list[0]->RemoveFanout(i); --i; }

  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;
    for (size_t j = 0; j < _dfsList[i]->getOutList().size(); ++j) {
      if (_dfsList[i]->getOutList().at(j).gate()->getType() != AIG_GATE) continue;
      if (!_dfsList[i]->getOutList().at(j).gate()->InDfs()) {
        _dfsList[i]->RemoveFanout(j); --j;
      }
    }
  }
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
inline void print_merging_message(CirGate* g, CirGate*& h, bool inv) {
  cout << "Simplifying: " << g->getId() << " merging ";
  if (inv) cout << '!'; 
  cout << h->getId() << "..." << endl;// << endl;
}

inline void print_merging_message_0(CirGate*& h) {
  cout << "Simplifying: 0 merging ";
  cout << h->getId() << "..." << endl;// << endl;
}

void
CirMgr::optimize() // core dumped after calling destructor in cirMgr
{
  vector<int> deleteList;

  if (!_dfs_done) DoDfs();
  
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() == AIG_GATE) {
      #define gin _dfsList[i]->getfanin
      if (gin(0).gate() == gin(1).gate()) { // same input or inverted input
        if (_dfs_done) _dfs_done = false;

        if (gin(0).isInv() != gin(1).isInv()) { //cout << "inverted inputs..." << endl;
          print_merging_message_0(_dfsList[i]);
          merge(_list[0], _dfsList[i], 0);
        }
        else { //cout << "identical inputs ..." << endl;
          print_merging_message(gin(0).gate(), _dfsList[i], int(gin(0).isInv()));
          merge(gin(0).gate(), _dfsList[i], gin(0).isInv());
        }
        (deleteList).push_back(_dfsList[i]->getId());
      }
      else if (gin(0).gate() == _list[0] || gin(1).gate() == _list[0]) { // const input
        if (_dfs_done) _dfs_done = false;
        int fuck = (gin(0).gate() == _list[0] ? 0 : 1);

        if (gin(fuck).isInv()) { //cout << "input with !const0" << endl;// !const0
          int shit = (fuck == 1 ? 0 : 1);
          print_merging_message(gin(shit).gate(), _dfsList[i], gin(shit).isInv());
          merge(gin(shit).gate(), _dfsList[i], int(gin(shit).isInv()));
        } else { //cout << "input with const0" << endl;// const0
          print_merging_message_0(_dfsList[i]);
          merge(_list[0], _dfsList[i], 0);
        }
        (deleteList).push_back(_dfsList[i]->getId());
      }
    }
  }
  for (size_t i = 0; i < deleteList.size(); ++i) {
    if (_list[deleteList[i]]->getType() == AIG_GATE) --_ANDnum;
    delete _list[deleteList[i]];
    _list[deleteList[i]] = NULL;
  }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
