
#include <array>
#include <vector>
#include <list>
#include <cassert>
#include <cstdio>

#include "ring_buffer.hh"

template <typename ContainerT, size_t Cap>
void basic_test()
{
  rhm::ring_buffer<int, Cap, ContainerT> buf;
  printf("Empty: ");
  std::cerr << buf << '\n';
  //std::cerr << buf << '\n';
  assert(buf.empty());
  for(size_t i = 1; i <= Cap; ++i)
  {
    buf.push((int)i);
    printf("After pushing %2d: ", (int)i);
    std::cerr << buf << '\n';
    assert(buf.front() != buf.nil());
    assert(*(buf.front()) == 1);
    assert(buf.size() == i); 
  }

  puts("Buffer should now be full.");
  assert(buf.size() == buf.capacity());
  assert(buf.full());

  int val = (int)(Cap+1);
  buf.push(val);
  printf("After pushing %d: ", val);
  std::cerr << buf << '\n';
  assert(*(buf.front()) == 2);

  ++val;
  buf.push(val);
  printf("After pushing %d: ", val);
  std::cerr << buf << '\n';
  assert(*(buf.front()) == 3);

  ++val;
  buf.push(val);
  printf("After pushing %d: ", val);
  std::cerr << buf << '\n';
  assert(*(buf.front()) == 4);

  buf.reset();
  printf("After reset: ");
  std::cerr << buf << '\n';
  assert(buf.empty());
  assert(buf.size() == 0);
  assert(buf.capacity() == Cap);

  for(size_t i = 1; i <= Cap; ++i)
  {
    int val = 20 + (int)i;
    buf.push(val);
    printf("After pushing %2d: ", val);
    std::cerr << buf << '\n';
    assert(buf.front() != buf.nil());
    assert(*(buf.front()) == 21);
    assert(buf.size() == i); 
  }

  printf("Buffer should now be full again. (size=%lu, capacity=%lu)\n", buf.size(), buf.capacity());
  assert(buf.size() == buf.capacity());
  assert(buf.full());

  buf.push(42);
  printf("After pushing 42: ");
  std::cerr << buf << '\n';
  assert(*(buf.front()) == 22);
}



int main()
{
  //puts("std::array:");
  //basic_test<std::array<int, 10>, 10>();
  puts("\nstd::vector:");
  basic_test<std::vector<int>, 10>();
  puts("\nstd::list:");
  basic_test<std::list<int>, 10>();
  return 0;
}
