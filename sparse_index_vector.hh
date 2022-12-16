
#pragma once

#include <vector>
#include <vector>
#include <iostream>
#include <algorithm>

namespace rhm {

/** std::vector but also stores separate set of arbitrary "sparse" indices or priorities for values that determines its order. Indices do not need to be unique, they are used to order the values not identify them.  Use this instead of a std::map<size_t, value> or std::multimap<size_t, value> if insertions or changes are not frequent.  E.g. you will be setting up a list of values at startup but will otherwise only be iterating over the list, or only with occasional additions and removals.  Insertions and removals currently require linear search of the indices (could be changed in the future.).  Insertions and removals may also result in reallocation of both the values and indices containers (std::vector objects).
  @todo an iterator adapter of some kind that creates and returns std::pair<size_t, ValueT>'s (in Aria this would really just be for compatability with existing std::maps and std::multimaps)
  @todo most accessors could be made constexpr, if only supporting C++20+  (or multiple versions of them?)
  @todo insert() does a linear search to find the right position to insert the new value. we could check the size of the vector and do
   this for small vectors, and do a binary search of the indices for larger vectors.  Or what about a more parallel search (e.g. simd comparisons)?
*/


template<typename T>
class sparse_index_vector {
public:
  using ValueVecT = std::vector<T>;
  using iterator = typename ValueVecT::iterator;
  using const_iterator = typename ValueVecT::const_iterator;
  using size_type = typename ValueVecT::size_type;
  using IndexT = size_t;
  using IndexVecT = std::vector<IndexT>;

private:
  ValueVecT values;
  IndexVecT indices;

public:
  iterator begin() noexcept { return values.begin(); }
  iterator begin() const noexcept { return values.begin(); }
  const_iterator cbegin() const noexcept { return values.cbegin(); }
  iterator end() noexcept { return values.end(); }
  iterator end() const noexcept { return values.end(); }
  const_iterator cend() const noexcept { return values.cend(); }

  size_type size() const noexcept { return values.size(); }

  void clear() noexcept
  {
    values.clear();
    indices.clear();
  }

  iterator insert(size_t index, const T& value)
  {
    // todo we could do a more efficient search, maybe worth doing for larger lists
    auto i = indices.begin();
    while(i != indices.end())
    {
      if(*i >= index) break;
      ++i;
    }
    
    auto dist = std::distance(indices.begin(), i);
    auto d = values.begin();
    std::advance(d, dist);
    indices.insert(i, index);
    return values.insert(d, value);
  }

  // todo:
  // iterator insert(size_t index, T&& value)
  // {
  // }

  // todo:
  void erase(const T& value)
  {
    auto f = find(values.begin(), values.end(), value);
    if(f == values.end()) return;
    auto dist = std::distance(values.begin(), f);
    values.erase(f);
    auto e = indices.begin();
    std::advance(e, dist);
    indices.erase(e);
  }

  iterator erase(const T* const value)
  {
    return erase(*value);
  }

  template<typename T_> friend std::ostream& operator<<(std::ostream& os, const sparse_index_vector<T_>& v);
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const sparse_index_vector<T>& v)
{
  for(auto value_i = v.values.cbegin(), index_i = v.indices.cbegin(); 
      value_i != v.values.end(); ++value_i, ++index_i)
  {
    if(value_i != v.values.begin())
      s << ", "; 
    s << "(" << *index_i << ": " << *value_i <<")";
  }
  return s;
}

} // namespace rhm
