#pragma once

#include <iostream>
#include <iterator>

namespace rhm {


  // Test that type T has begin() and end() that return iterators, and a size() method that returns size_t.
  // TODO check if there is an equivalent concept already defined in the standard library.
  template<typename T>
  concept StdContainerType = requires (T c) {
    // todo could check for more abstract concepts than just the iterator type. 
    {c.begin()} -> std::same_as<typename T::iterator>;
    {c.end()} -> std::same_as<typename T::iterator>;
    {c.size()} -> std::convertible_to<size_t>;
  };

  /* Not used. See discussion at ring_buffer::push_imp() below.
  template<typename CT> // todo could constrain CT to StdContainerType, and also check for CT::value_type
  concept ContainerTypeHasPushBack = requires (CT c, typename CT::value_type item) { c.push_back(item); };
  */

/** @brief Adapts a container type (see requirements below) into a very simple FIFO ring buffer.

    A maximum size (capacity) is maintained, and memory can be reused as items are removed (popped) 
    from the front and added (pushed) to the back. If the buffer is full (at capacity), then old 
    items are replaced by new items (via the assignment operator).   

    Specify a standard container type as the ContainerT parameter such as std::array, std::vector,
    or any other type providing standard iterator interface as the underlying storage.

    For example, if std::array is used as ContainerT, then all memory is stored in a fixed size 
    contiguous array inside this object, and no heap allocation is needed. The ring buffer Capacity
    parameter should be specified as the same as the std::array size, and new items will begin to
    replace old items as soon as the buffer reaches the capacity.  If std::array is used, then a
    small ring buffer can be efficiently kept in a stack object rather than allocated on the heap.
    Or, if std::vector is used, or any other container whose size increases as items are added with
    push_back(), new items are added with push_back() until the size of the container reaches the
    given Capacity, in which case the ring buffer will begin reusing the memory previously used in
    the std::vector, replacing old items.  I.e. the size of the std::vector will be expanded as
    usual using std::vector::push_back() until reaching Capacity.  

    The container type given as ContainerT should support the following:
      * Must define the contained value type as value_type. E.g. std::array<int, 10> defines value_type as int.
      * Provide iterators via begin() and end().
      * Provide a size via size().
      * Optionally provide a push_back() method if it can be expanded (until size() == Capacity).  
        If it does not have a push_back() method, then this feature will not be used and only the
        container's current size is used; will begin replacing items when the container's size is
        reached.  (For example, a std::array with size N should be used with ring_buffer capacity N.)

    The underlying container object can be accessed as @a container after the ring_buffer is created.
    (For example, to choose an initial size with reserve(), or similar.)  Do not perform any operations
    with @a container that may invalidate iterators stored by ring_buffer after you have begun 
    adding/removing items to/from the ring_buffer.

    Items may be added to the ring_buffer with push() or emplace(), and the oldest item still available
    can be accessed from the front with pop() or front().  (But most recent item cannot be accessed
    from the back.) If an old item is replaced by a new item by push(), then T's copy or move
    assignment operator is used to replace the item. 
  
    @note If reset() is called or items are popped(), old items are not destroyed (destructors not
    called).  Destructors may be called if the item is later replaced by a new item. 
  
    @note This implementation is not thread safe; it is not designed or optimized for simultaneous
    reads and writes from different threads. (A mutex lock or similar may be required.) A future
    version could do this. E.g. use atomics (e.g. see <https://rigtorp.se/ringbuffer/>)

    @note Requires C++20 mode when compiling.

    @todo front/back naming might be confusing for a FIFO buffer since we add to back and pop from front, but this matches other containers.

 */
template<size_t Capacity, StdContainerType ContainerT>
class ring_buffer 
{

  using ItemT = typename ContainerT::value_type;

public:

  ring_buffer() : curSize(0), front_it(container.end()), back_it(container.begin())
  {}

  ring_buffer(const ring_buffer<Capacity, ContainerT>& other) :
    container(other.container),
    curSize(other.curSize), 
    front_it(other.front_it == other.container.end() ? container.end() : (container.begin() + std::distance(other.container.begin(), typename ContainerT::const_iterator{other.front_it}))),
    back_it(container.begin() + std::distance(other.container.begin(), typename ContainerT::const_iterator{other.back_it}))
  { }

  ring_buffer<Capacity, ContainerT> & operator=(const ring_buffer<Capacity, ContainerT>& other) 
  {
    if(&other == this) [[unlikely]] return *this;
    container = other.container;
    curSize = other.curSize;
    front_it = other.front_it == other.container.end() ? container.end() : (container.begin() + std::distance(other.container.begin(), typename ContainerT::const_iterator{other.front_it}));
    back_it = other.front_it == other.container.end() ? container.end() : (container.begin() + std::distance(other.container.begin(), typename ContainerT::const_iterator{other.back_it}));
    return *this;
  }

  ring_buffer<Capacity, ContainerT> & operator=(ring_buffer<Capacity, ContainerT>&& old) 
  {
    if(&old == this) [[unlikely]] return *this;
    auto old_front_pos = std::distance(old.container.begin(), old.front_it);
    auto old_back_pos = std::distance(old.container.begin(), old.back_it);
    container = std::move(old.container);
    curSize = old.curSize;
    front_it = container.begin() + old_front_pos;
    back_it = container.begin() + old_back_pos;
    return *this;
  }

  ring_buffer(ring_buffer<Capacity, ContainerT>&& old) 
  { 
    auto old_front_pos = std::distance(old.container.begin(), old.front_it);
    auto old_back_pos = std::distance(old.container.begin(), old.back_it);
    container = std::move(old.container);
    curSize = old.curSize;
    front_it = container.begin() + old_front_pos;
    back_it = container.begin() + old_back_pos;
  }

  // todo allow construction/assignment with different capacities, truncate from front for smaller capacity or copy into front of new container for larger?
  // todo conversions that copy items between ring_buffers with different container types?

  /** Get an iterator for the front item (the item that would
      be returned by pop()). If the buffer is currently empty, nil() will be
      returned.
      
      To remove an item without
      making a copy with pop(), first check if the buffer is empty(). Then use this 
      function to get the data. Then call advance_front(). 
   */
  typename ContainerT::iterator front() {
    if(front_it == container.end() || curSize == 0) // empty 
      return nil();
    return front_it;
  }

  /** Get an iterator for the back of the bufer (the item that would be
      replaced by push()). This is not the last item, rather it is the
      next, unused, "slot".  If the buffer is full, an iterator equivalent to that
      returned by nil() is returned.
      @TODO maybe rename to end. maybe also provide a last() accessor?
    
      To add an item to the buffer without pushing
      a copy with push(), first check if the buffer is full (in which case you
      must push() your item). Then use this function to write the data into the
      next unused 'slot'. Then call advance_back() to advance the back of the buffer
      to your new item. 
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


private:
  /* Two different push_imp() implementations The commented out onen uses the HasPushBack concept defined above, chooses a push_imp() template via sfinae 
     and matching ContainerT to HasPushBack or note.  The second one just uses constexpr to require push_back().

  // This template function is used only if the container type (ContainerT) has a push_back() method. 
  template<ContainerTypeHasPushBack CT>
  void push_imp(CT& cont, const typename CT::value_type& item)
  {
    if(cont.size() < Capacity) // todo also need consteval check for push_back function, throw or assert if we don't
    {
      cont.push_back(item);
      front_it = cont.begin();
      back_it = cont.end();
      ++curSize;
      return;
    }

    // otherwise same as default (no push_back) implementation
    default_push_imp(cont, item);
  }

    
  // this template is used for any container type (i.e. doesn't have a push_back() method)
  template<typename CT>
  void push_imp(CT& cont, const typename CT::value_type& item)
  {
    default_push_imp(cont, item);
  }

  template<typename CT>
  void default_push_imp(CT& cont, const typename CT::value_type& item)
  {
    if(curSize == Capacity)
      advance_front(); // throw away the item at the front, no longer full, curSize == Capacity-1; when we advance_back(), then back_it will again be correct.
    if(back_it == cont.end()) // no longer filling container to capacity, need to "wrap around"
      back_it = cont.begin(); 
    *back_it = item;
    advance_back();
  }
  */

  void push_imp(ContainerT& cont, const ItemT& item)
  {
    constexpr bool has_push_back = requires(ContainerT c, ItemT v) { c.push_back(v); };
    if constexpr(has_push_back) 
    {
      if(cont.size() < Capacity) // todo also need consteval check for push_back function, throw or assert if we don't
      {
        cont.push_back(item);
        front_it = cont.begin();
        back_it = cont.end();
        ++curSize;
        return;
      }
    }
    if(curSize == Capacity)
      advance_front(); // throw away the item at the front, no longer full, curSize == Capacity-1; when we advance_back(), then back_it will again be correct.
    if(back_it == cont.end()) // no longer filling container to capacity, need to "wrap around"
      back_it = cont.begin(); 
    *back_it = item;
    advance_back();
  }



public:
  /** Push a new item.  If the buffer is full, then the oldest item is replaced with the new item.  If the current size of the container is not yet at capacity, and the container type (ContainerT) has a push_back() method, then push the item (and increase the size of the container) with push_back(). 
  */
  void push(const ItemT& item) 
  {
    push_imp(container, item);
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

  /** Fill buffer to capacity with given value. */
  void fill(const ItemT& value)
  {
    reset();
    while(!full())
       push(value);
  }

  /** Print contents to stderr. 
      @see operator<<(std::ostream&, const ring_buffer&)
      @pynote use printQueue() instead of print() (which is a reserved word in Python)
  */
  void print() const
  {
    std::cerr << *this << '\n';
  }
  template<size_t Cap, class CT> 
  friend std::ostream& operator<<(std::ostream& os, const ring_buffer<Cap, CT>& rb);

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
      The contents are not
      destroyed, and will remain in the container (but inaccessible through ring_buffer::pop()), and will be replaced as new items are pushed into the ring_buffer.
      The current allocated capacity of the container is retained.
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
      values of front(), back(), pop(), etc. */
  typename ContainerT::iterator nil() {
    return container.end();
  }


public:
  ContainerT container;

private:
  size_t curSize;
  typename ContainerT::iterator front_it, back_it;   // note need to be after container so that copy/move constructors/operators can refer to container member when assigning these
  // push to back, pop from front; front will point to first item, 
  // back to one past last. 

};


/** Output the current contents of the buffer.   Items are printed as they appear in the container, with the current position and size of the ring buffer marked with square brackets.
*/
template<size_t Capacity, class ContainerT>
inline std::ostream& operator<<(std::ostream& os, const ring_buffer<Capacity, ContainerT>& rb)
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

