
#include "sparse_index_vector.hh"
#include <iostream>

int main()
{
  rhm::sparse_index_vector<int> v;
  puts("empty: ");
  std::cout << v << '\n';
  
  v.insert(0, 0);
  puts("after insert 0: ");
  std::cout << v << '\n';

  v.insert(10, 10);
  v.insert(20, 20);
  v.insert(5, 5);
  puts("after inserting a  few: ");
  std::cout << v << '\n';

  v.clear();
  puts("after clear: ");
  std::cout << v << '\n';

  v.insert(10, 10);
  v.insert(10, 11);
  v.insert(100, 100);
  v.insert(5, 5);
  v.insert(200, 200);
  puts("after inserting a few: ");
  std::cout << v << '\n';

  v.erase(5);
  puts("after erasing 5: ");
  std::cout << v << '\n';

  v.erase(11);
  puts("after erasing 11: ");
  std::cout << v << '\n';

  return 0;
}
