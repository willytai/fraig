/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ==" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _size(0) { if (b != 0) init(b); }
   ~HashSet() {}

   void reset() {
     _size = 0;
     for (size_t i = 0; i < _buckets.size(); ++i) {
       _buckets[i].clear();
     }
     _buckets.clear();
   }

   void init(const size_t& b) {
     reset();
     _buckets.resize(b);
     _size = b;
   }

   void insert(size_t i, const Data& d) {
     if (i >= _size) _buckets.resize(i+1);
     _buckets[i].push_back(d);
   }

private:
   size_t                 _size;
   vector<vector<Data> >  _buckets;
};

#endif // MY_HASH_SET_H
