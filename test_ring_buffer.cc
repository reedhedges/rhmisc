
#include <array>
#include <vector>
#include <list>
#include <cassert>
#include <cstdio>

#include "ring_buffer.hh"

template <typename ContainerT, size_t Cap>
void basic_test()
{
  rhm::ring_buffer<Cap, ContainerT> buf;
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


void test_copy()
{
  auto rb1 = new rhm::ring_buffer<10, std::vector<int>>;
  rb1->fill(0);
  rb1->push(1); // rb1 should be all 0, except for first item, which should be 1. rb1's front should be 1, back should be at second item (0).
  puts("First buffer, all 0 except first:");
  rb1->print();
  rhm::ring_buffer<10, std::vector<int>> rb2(*rb1);
  puts("Copy of first buffer, all 0 except first:");
  rb2.print();
  // how to test that rb2's iterators are valid for rb2's container:
  puts("Fill first buffer with 9...");
  std::fill_n(rb1->container.begin(), 10, 9);
  rb1->print();
  puts("Copy should still be all 0:");
  rb2.print();
  puts("Destroy first buffer...");
  delete rb1;
  puts("Copy should still be ok...");
  rb2.print();
  assert(*(rb2.front()) == 0);
  assert(rb2.full());
  assert(rb2.back() == rb2.nil()); // full

  auto rb3 = rb2;
  std::cout << "Copy assignment : " << rb3 << '\n';

  auto rb4 = std::move(rb2);
  std::cout << "Move assignment : " << rb4 << '\n';
  auto rb5(std::move(rb3));
  std::cout << "Move construction : " << rb5 << '\n';

  rhm::ring_buffer<5, std::vector<int>> empty1;
  std::cout << "Empty buffer: " << empty1 << '\n';
  assert(empty1.empty());

  rhm::ring_buffer<5, std::vector<int>> empty2(empty1);
  std::cout << "Copy of empty buffer: " << empty2 << '\n';
  assert(empty2.empty());
}


int main()
{
  puts("std::array:");
  basic_test<std::array<int, 10>, 10>();
  puts("\nstd::vector:");
  basic_test<std::vector<int>, 10>();
  puts("\nstd::list:");
  basic_test<std::list<int>, 10>();
  puts("\ncopies and moves:");
  test_copy();
  return 0;
}
