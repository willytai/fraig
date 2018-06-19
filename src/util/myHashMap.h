/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

typedef unsigned long long int Simtype;

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
class SimKey
{
public:
   SimKey(Simtype key) : _key(key) {
    if (~key < key) {
      _key = ~key;
      inv = true;
      cout << "inverted" << endl;
    } else inv = false;
   }
   ~SimKey() {}
   
   void update(const Simtype& k) {
     inv = false;
     if (~k < k) {
       inv = true;
       _key = ~k;
     } else _key = k;
   }
   Simtype operator () () const { return _key; }
   bool isInv() const { return inv; } 
private:
   bool inv;
   Simtype _key;
};

class HashKey // constructed by the gate's input pointer, cast to size_t
{
public:
   HashKey(size_t& in0, size_t& in1) {
     g0 = in0; g1 = in1;
     if (g0 > g1) g0 ^= g1 ^= g0 ^= g1;
   }
   ~HashKey() {}

   size_t operator() () const { return g0 + g1; }

   size_t get(int i) const { if (!i) return g0; return g1; }

   bool operator == (const HashKey& k) const { return ((g0 == k.get(0)) && (g1 == k.get(1))); }

private:
   size_t g0, g1;
};

template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;

   public:
      iterator(vector<HashNode>* b, typename vector<HashNode>::iterator it, int count, int ref)
              : _b(b), it(it), _count(count), ref(ref) {}
      iterator(const iterator& i) : _b(i._b), it(i.it), _count(i._count), ref(i.ref) {}
      ~iterator() {}

      const HashNode& operator * () const { return *it; }
      iterator& operator ++ () {
        assert(_count >= 0);
        ++it;
        if (it == (*_b).end()) {
          if (!_count) return (*this);
          ++_b; --_count;
          while ((*_b).size() == 0 && _count > 0) { ++_b; --_count; }
          it = (*_b).begin();
        }
        return (*this);
      }
      iterator operator ++ (int) {
        iterator ret = *this;
        ++it;
        if (it == (*_b).end()) {
          if (!_count) return ret;
          ++_b; --_count;
          while ((*_b).size() == 0 && _count > 0) { ++_b; --_count; }
          it = (*_b).begin();
        }
        return ret;
      }
      iterator& operator -- () { // TODO
        if (_count == ref && it == (*_b).begin()) return *this;
        if (it == (*_b).begin()) {
          --_b; ++_count;
          while ((*_b).size() == 0 && _count < ref) { --_b; ++_count; }
          it = (*_b).end(); --it;
        } else --it;
        return *this;
      }
      iterator operator -- (int) { // TODO
        if (_count == ref && it == (*_b).begin()) return *this;
        iterator ret = *this;
        if (it == (*_b).begin()) {
          --_b; ++_count;
          while ((*_b).size() == 0 && _count < ref) { --_b; ++_count; }
          it = (*_b).end(); --it;
        } else --it;
        return ret;
      }

      bool operator != (const iterator& i) const { return it != i.it; }
      bool operator == (const iterator& i) const { return it == i.it; }
      iterator& operator = (const iterator& i) {
        if (this == &i) return *this;
        _b = i._b; it = i.it; _count = i._count; ref = i.ref;
        return *this;
      }

   private:
         vector<HashNode>* _b;
      typename vector<HashNode>::iterator it;
      int _count;
      int ref;
   };

   void init(size_t b) { reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
     size_t i = 0;
     while (!_buckets[i].size()) ++i;
     return iterator(_buckets + i, _buckets[i].begin(), _numBuckets - i - 1, _numBuckets - 1);
   }
   // Pass the end
   iterator end() const { return iterator(_buckets + _numBuckets, _buckets[_numBuckets - 1].end(), 0, _numBuckets - 1); }
   // return true if no valid data
   bool empty() const { if (!size()) return true; return false; }
   // number of valid data
   size_t size() const { return 0; }

   // check if k is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const HashKey& k) const {
     size_t num = bucketNum(k);
     for (size_t i = 0; i < _buckets[num].size(); ++i) {
       if (k == _buckets[num][i].first) return true;
     }
     return false;
   }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(const HashKey& k, HashData& d) const {
     size_t id = bucketNum(k);
     for (size_t i = 0; i < _buckets[id].size(); ++i) {
       if (k == _buckets[id][i].first) { 
        d = _buckets[id][i].second;
        return true;
       }
     }
     return false;
   }

   // update the entry in hash that is equal to k (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const HashKey& k, HashData& d) {
     size_t id = bucketNum(k);
     for (size_t i = 0; i < _buckets[id].size(); ++i) {
       if (k == _buckets[id][i].first) {
         _buckets[id][i].second = d;
         return true;
       }
     }
     HashNode n(k, d);
     _buckets[id].push_back(n);
     return false;
   }

   // return true if inserted d successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) { // check before insertion, 
    //  if (check(k)) return false; this operation is cancled in order to boost the speed
     HashNode n(k, d);
     _buckets[bucketNum(k)].push_back(n);
     return true;
   }

   // for FEC
   void insert(const HashNode& node) {
     int id = bucketNum(node.first);
     _buckets[id].push_back(node);
   }

   /*vector<vector<HashNode> >*/ void valid(vector<vector<HashNode> >& valid) {
    //  vector<vector<HashNode> > valid;
     for (size_t i = 0; i < _numBuckets; ++i) {
       if (_buckets[i].size() == 0) continue;
       if (_buckets[i].size() == 1) {
         _buckets[i][0].second->resetFEC();
         continue;
       }
       while (_buckets[i].size()) {
         vector<HashNode> fec;
         Simtype ref = (_buckets[i][0].first)();
         fec.push_back(_buckets[i][0]);
         _buckets[i].erase(_buckets[i].begin());
         for (size_t j = 0; j < _buckets[i].size(); ++j) {
           if ((_buckets[i][j].first)() != ref) continue;
           fec.push_back(_buckets[i][j]);
           _buckets[i].erase(_buckets[i].begin()+j); --j;
         }
         if (fec.size() == 1) continue;
         valid.push_back(fec);
       }
     }
    //  return valid;
   }

   // return true if removed successfully (i.e. k is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const HashKey& k) {
     size_t id = bucketNum(k);
     #define buck _buckets[id]
     for (size_t i = 0; i < buck.size(); ++i) {
       if (k == buck[i].first) {
        buck[i] = buck[buck.size() - 1];
        buck.pop_back();
        return true;
       }
     }
     return false;
   }

   void debbug() const {
     for (size_t i = 0; i < _numBuckets; ++i) {
       cout << "_buckets[" << i << "]: ";
       for (size_t j = 0; j < _buckets[i].size(); ++j) {
         if ((_buckets[i][j].first).isInv()) cout << '!';
         cout << '(' << (_buckets[i][j].first)() << ", ";
         cout<< _buckets[i][j].second->getId() << ')';
         cout << "  ";
       }
       cout << endl;
     }
   }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
