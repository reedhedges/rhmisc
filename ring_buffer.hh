#pragma once

#include <iostream>
#include <array>
#include <iterator>

namespace rhm {

/** @brief Adapts a container type (see requirements below) into a very simple ring buffer.

    A maximum size (capacity) is maintained, and 
    memory can be reused as items are removed (popped) from the front and added (pushed)
    to the back. If the buffer is full (at capacity), then old items are replaced by new items (via the assignment operator).   

    Specify a standard container type as the ContainerT parameter such as std::array, std::vector, or any other type providing standard iterator interface as the underlying storage.
    For example, if std::array is used as ContainerT, then all memory is stored in a fixed size condiguous array inside this object, and no heap allocation is needed. The 
    ring buffer Capacity parameter should be specified as the same as the std::array size, and new items will begin to replace old items as soon as the buffer reaches the capacity.
    If std::array is used, the a small ring buffer can be efficiently kept in a stack object rather than allocated on the heap.
    Or, if std::vector is used, or any other item whose size increments as items are added with push_back(),  new items are added with push_back() until the size of the 
    container reaches the given Capacity, in which case the ring buffer will begin reusing the memory previously used in the std::vector, replacing old items.  I.e. the size of
    the vector will be expanded as usual until reaching Capacity.  

    The underlying container object can be accessed as @a container after the ring_buffer is created. (For example,
    to choose an initial size with reserve(), or similar.)   The container type given as ContainerT should support the following:
      * Provide iterators via begin() and end().
      * Provide a size via size().
      * Optionally provide a push_back() method if it can be expanded (until size() == Capacity).  If it does not have a push_back() method, then this feature will not be used and only the container's current size is used.

    Items may be added to the ring_buffer with push() or emplace(), and
    the oldest item still available can be accessed from the front with pop() or front().  (But most recent item cannot be accessed from the back.)
    If an old item is replaced by a new item by push(), then T's copy or move assignment operator is used to replace the item.   If replaced by emplace(), then the copy or move constructor is used.
  
    Note: a std::containeray is used internally, so the size of an ring_buffer object is fixed at Capacity * sizeof(T) + a few pointers and size. (two iterators and size_t).  So a very large ring_buffer should be allocated and stored as a pointer (e.g. use std::make_unique or std::make_shared.) But smaller ring_bufferObjects can be stored on the stack (e.g. as regular data members of a class.) 
  
    Note: currently if reset() is called or items are popped(), old items are not 
    destroyed (destructors not called).  Destructors may be called if the item is later replaced by a new item. 
  
    Note: This implementation is not thread safe; it is not designed or optimized for simultaneous reads and writes from different threads. (A mutex lock or similar may be required.) A future version could do this.
  
    @todo Ideally, this class would be fully threadsafe/non-locking (or with occasional
    mutex locking in certain cases), but it is not currently. Should be updated to use
    atomic support in modern C++ standard library if possible, e.g. <https://rigtorp.se/ringbuffer/>
  
 */
template<class ItemT, size_t Capacity, class ContainerT = std::array<ItemT, Capacity>>
class ring_buffer {
public:

  ring_buffer() : curSize(0), front_it(container.end()), back_it(container.begin())
    // note front_it at end indicates empty state
  {
  }


  /** Get an iterator for the front item (the item that would
   * be returned by pop()). If the buffer is currently empty, nil() will be
   * returned.
   * 
   * To remove an item without
   * making a copy with pop(), first check if the buffer is empty(). Then  use this 
   * function to get the data. Then call advance_front(). 
   */
  typename ContainerT::iterator front() {
    if(front_it == container.end() || curSize == 0) // empty 
      return nil();
    return front_it;
  }

  /** Get an iterator for the back of the bufer (the item that would be
   * replaced by push()). This is not the last item, rather it is the
   * next, unused, "slot".  If the buffer is full, an iterator equivalent to that
   * returned by nil() is returned.
   * @TODO maybe rename to end. maybe also provide a last() accessor?
   *
   * To add an item to the buffer without pushing
   * a copy with push(), first check if the buffer is full (in which case you
   * must push() your item). Then use this function to write the data into the
   * next unused 'slot'. Then call advance_back() to advance the back of the buffer
   * to your new item. 
   */
  typename ContainerT::iterator back() {
    if(front_it == back_it)
    {
      //std::cerr << "ring_buffer: back(): 0-capacity or full, returning nil.\n";
      return nil();
    }
    return back_it;
  }

  /** Advance the front of the buffer. 'Used' size will be decremented.  */
  void advance_front() {
    if(front_it == container.end())  // initial or  empty state.
      front_it = container.begin();
    else if(++front_it == container.end()) 
      front_it = container.begin();
    if(front_it == back_it) { // it's now empty (not full)
      front_it = container.end();
      back_it = container.begin();
    }
    --curSize;
  }

  /** Same as advance_front() */
  void pop_front() { advance_front(); };

  /** Advance the back (an 'empty' push), if the buffer is not full.  'Used' size will be incremented.  */
  void advance_back() 
  {
    if(curSize == Capacity)
    {
      std::cerr << "ring_buffer: advance_back(): buffer is full, can't advance back.\n" << std::endl;
    }
    assert(curSize < Capacity); // size should either have been adjusted by advance_front(), or container stil has spare capacity.
    // throw?

    ++back_it;

    if(back_it == container.end())
    {
      back_it = container.begin();
    }

    if(front_it == container.end()) // was in initial or empty state, fix front now that we have something
    {
      front_it = container.begin();
    }
    assert(front_it != container.end());
    assert(std::distance(container.begin(), container.end()) > 0);

    ++curSize;
  }

  /** Push a new item.  If the buffer is full, then the oldest item is replaced with the new item.  */
  void push(const ItemT& item) 
  {
    // push_back to expand container if neccesary and possible
    if(container.size() < Capacity) // todo also need consteval check for push_back function, throw or assert if we don't
    {
      container.push_back(item);
      front_it = container.begin();
      back_it = container.end();
      ++curSize;
      return;
    }
    
    if(curSize == Capacity)
      advance_front(); // throw away the item at the front, no longer full, curSize == Capacity-1; when we advance_back(), then back_it will again be correct.
    if(back_it == container.end()) // no longer filling container to capacity, need to "wrap around"
      back_it = container.begin(); 
    *back_it = item;
    advance_back();
  }

  /** Push a new item.  If the buffer is full, then the oldest item is replaced with the new item.  */
/*
  void push(ItemT&& item) 
  {
    // push_back to expand container if neccesary
    if(container.size() < Capacity) // todo also need consteval check for push_back function
    {
      container.push_back(item);
      advance_back();
      return;
    }
    
    if(full())
      advance_front(); // throw away the item at the front, when we advance_back(), then back will be pointing to it.
    *back_it = item;
    advance_back();
  }
*/

  /** Print contents to stderr. 
      @see operator<<(std::ostream&, const ring_buffer&)
      @pynote use printQueue() instead of print() (which is a reserved word in Python)
  */
  void print() const
  {
    std::cerr << *this << '\n';
  }
  template<typename T, size_t C, class CT> 
  friend std::ostream& operator<<(std::ostream& os, const ring_buffer<T, C, CT>& rb);

  /** Get the number of items currently in the buffer. */
  size_t size() const {
    return curSize;
  }

  /** Get the maximum capacity of the buffer (same as Capacity parameter). */
  constexpr size_t capacity() const {
    return Capacity;
  }

  /** Return true if the buffer is empty (has no 'used' items), false otherwise.  */
  bool empty() const {
    return (curSize == 0);
    //return (front_it == container.end());
  }

  /** Logically clear the bufer, resetting to initial empty state
   * The contents are not
   * destroyed, and will remain in the container (but inaccessible through ring_buffer::pop()), and will be replaced as new items are pushed into the ring_buffer.
   * The current allocated capacity of the container is retained.
   */
  void reset() {
    // todo should we also have a clear()?
    front_it = container.end();
    back_it = container.begin();
    curSize = 0;
  }

  /** Return true if the buffer is full, false otherwise. */
  bool full() const {
    return(curSize == Capacity);
  }

  /** Return an iterator representing an invalid item. Compare to the return
   * values of front(), back(), pop(), etc. */
  typename ContainerT::iterator nil() {
    return container.end();
  }


private:
  size_t curSize;
  typename ContainerT::iterator front_it, back_it;   
  // push to back, pop from front; front will point to first item, 
  // back to one past last. 


public:

  ContainerT container;

};


/** Output the current contents of the buffer.   Items are printed as they appear in the container, with the current position and size of the ring buffer marked with square brackets.
*/
template<typename ItemT, size_t Capacity, class ContainerT>
inline std::ostream& operator<<(std::ostream& os, const ring_buffer<ItemT, Capacity, ContainerT>& rb)
{
  //printf("container.begin()=0x%p container.end()=0x%p container.size()=%d curSize=%lu\n", container.begin(), container.end(), container.size(), curSize);
  if(rb.container.begin() == rb.container.end())
  {
    os << "[empty ring_buffer]";
    return os;
  }
  for(typename ContainerT::const_iterator i = rb.container.begin(); i != rb.container.end(); ++i) {
    if(i == rb.back_it)
      os << "]";
    if(i != rb.container.begin())
      os << ",";
    if(i == rb.front_it || (i == rb.container.begin() && rb.front_it == rb.container.end()) )
      os << "[";
    os << (*i); // << "," ;
  }
  return os;
}


} // end namespace rhm

