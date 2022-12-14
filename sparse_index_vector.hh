
#pragma once

#include <vector>
#include <vector>
#include <iostream>
#include <algorithm>

namespace rhm {

/* std::vector but also stores separate set of arbitrary "sparse" indices or priorities for items that determines its order. Items may have the same ordering index.  Use this instead of a std::map<size_t, item> or std::multimap<size_t, item> if insertions or changes are not frequent.  E.g. you will be setting up a list of items at startup but will otherwise only be iterating over the list, or only with occasional additions and removals.  Insertions and removals require linear search of the indices (though it should be sorted and could have a more efficient search for the nearest value) Modifications may also result in reallocation of both the items and indices containers.
  TODO an iterator adapter of some kind that creates and returns std::pair<size_t, ItemT>'s (in Aria this would really just be for compatability with existing std::maps and std::multimaps)
  todo most accessors could be constexpr after C++20
*/


template<typename T>
class sparse_index_vector {
public:
  using ItemVecT = std::vector<T>;
  using iterator = typename ItemVecT::iterator;
  using const_iterator = typename ItemVecT::const_iterator;
  using size_type = typename ItemVecT::size_type;
  using IndexT = size_t;

private:
  ItemVecT items;
  std::vector<IndexT> indices;

public:
  iterator begin() noexcept { return items.begin(); }
  iterator begin() const noexcept { return items.begin(); }
  const_iterator cbegin() const noexcept { return items.cbegin(); }
  iterator end() noexcept { return items.end(); }
  iterator end() const noexcept { return items.end(); }
  const_iterator cend() const noexcept { return items.cend(); }

  size_type size() const noexcept { return items.size(); }

  void clear() noexcept
  {
    items.clear();
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
    auto d = items.begin();
    std::advance(d, dist);
    indices.insert(i, index);
    return items.insert(d, value);
  }

  // todo:
  // iterator insert(size_t index, T&& value)
  // {
  // }

  // todo:
  void erase(const T& item)
  {
    auto f = find(items.begin(), items.end(), item);
    if(f == items.end()) return;
    auto dist = std::distance(items.begin(), f);
    items.erase(f);
    auto e = indices.begin();
    std::advance(e, dist);
    indices.erase(e);
  }

  iterator erase(const T* const item)
  {
    return erase(*item);
  }

  template<typename T_> friend std::ostream& operator<<(std::ostream& os, const sparse_index_vector<T_>& v);
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const sparse_index_vector<T>& v)
{
  for(auto item_i = v.items.cbegin(), index_i = v.indices.cbegin(); 
      item_i != v.items.end(); ++item_i, ++index_i)
  {
    if(item_i != v.items.begin())
      s << ", "; 
    s << "(" << *index_i << ": " << *item_i <<")";
  }
  return s;
}

} // namespace rhm
