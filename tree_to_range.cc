
// Explore, experiment, and benchmark how to gather and write to output strings or other data from a tree structure to an output stream or packets. 
// build with
//      c++ -std=c++20 -Wall -Wextra -o tree_to_range tree_to_range.cc

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <cassert>

struct TreeNode 
{
    std::string data;
    using ChildrenList = std::vector<TreeNode*>;
    ChildrenList children;
    TreeNode* parent;
    size_t size = 1;
    
    explicit TreeNode(std::string_view init_data) : 
        data(init_data)
    {}

    TreeNode(std::string_view init_data, TreeNode *init_parent) : 
        data(init_data), 
        parent(init_parent)
    {
        parent->add_child(this);
    }

    void add_child(TreeNode* n)
    {
        children.push_back(n);
        child_added();
    }

    void child_added()
    {
        ++size;
        if(parent)
            parent->child_added();
    }

};


// todo might need to be a template to allow both TreeNode and TreeNode const.
// maybe it could also be generalized to any tree structure with range projections or other means to select what member is the children list.
struct TreeIterator 
{
    using iterator_category = std::forward_iterator_tag; // could be made bidirectional pretty easily
    using difference_type = std::ptrdiff_t;
    using value_type = TreeNode;
    
    TreeIterator() noexcept :
        ptr(nullptr)
    {}

    TreeIterator(TreeNode *tree_ptr) noexcept : 
        ptr(tree_ptr), 
        next_child(ptr->children.begin()) 
    {}

    TreeIterator(TreeNode &tree) noexcept : 
        ptr(&tree), 
        next_child(ptr->children.begin())  
    {}

    TreeIterator(const TreeIterator& other) = default;

    TreeIterator(TreeIterator&& old) noexcept :
        ptr(std::move(old.ptr)),
        next_child(std::move(old.next_child)),
        prev(std::move(old.prev))
    {}

    TreeIterator& operator=(const TreeIterator& other) 
    { 
        if(&other != this) 
            TreeIterator(std::forward<const TreeIterator&>(other)); 
        return *this; 
    }
    TreeIterator& operator=(TreeIterator&& other) noexcept 
    { 
        if(&other != this) 
            TreeIterator(std::forward<TreeIterator&&>(other)); 
        return *this; 
    }

    TreeNode& operator*() const { return *ptr; }
    TreeNode* operator->() { return ptr; }

    friend bool operator==(const TreeIterator& a, const TreeIterator& b) { return a.ptr == b.ptr; }
    friend bool operator!=(const TreeIterator& a, const TreeIterator& b) { return a.ptr != b.ptr; }

    // advance iterator
    TreeIterator& operator++() {
        // note, perhaps this could also be a generator co-routine to keep previous state instead of maintaining our own stack.
        if(next_child == ptr->children.end()) // end of children list
        {
            if(prev.empty())
            {
                // end of tree (back at root), make this iterator null to signal end of tree
                ptr = nullptr;
            }
            else
            {
                std::tie(ptr, next_child) = prev.back();
                prev.pop_back();
                ++(*this); // next child of parent, recurse
            }
        }
        else
        {  
            TreeNode *child = *next_child;
            ++next_child;
            prev.push_back( {ptr, next_child} );
            ptr = child;
            next_child = ptr->children.begin();
        }
        return *this;
    }


    TreeIterator operator++(int n) { 
        TreeIterator tmp = *this; 
        for(int i = 0; i < n; ++i)
            ++(*this); 
        return tmp; 
    }

private:
    TreeNode *ptr = nullptr; 
    TreeNode::ChildrenList::iterator next_child;
    std::vector< std::tuple<TreeNode*, TreeNode::ChildrenList::iterator> > prev; // stack to store previous state when we traverse into children
};

// Override std::begin and std::end to get iterators for a tree of nodes:
namespace std
{
    TreeIterator begin(TreeNode *treeptr) {
        return TreeIterator(treeptr);
    }
    TreeIterator begin(TreeNode &tree) {
        return TreeIterator(tree);
    }
    TreeIterator end([[maybe_unused]] TreeNode *treeptr) {
        return TreeIterator(); // null iterator means end of tree
    }
    TreeIterator end([[maybe_unused]] TreeNode &tree) {
        return TreeIterator();
    }
}


/* 
    First attempt:
    Take strings from nodes in a tree to produce one stream of characters and write them to output.  
    This is done by defining a custom iterator for the tree nodes above, then use std::ranges::join_view with a projection to get the `data` member from each tree node and join them into one range. 
    To process and write the output we may have to iterate character by character over the final character view. This
    might be fine with a stream output that's doing additional buffering etc. or if we need to examine each value or character for some reason, or if
    we want to do something like copy or write them to a device in fixed sized chunks.  We are relying on the range view adapters
    and iterators to run in a well optimized way; quick benchmarks seem to bear that out. 
*/

#include <ranges>
#include <cstdio>

void test_join_ranges_putchar(TreeNode& tree)
{
    puts("-> test_join_ranges_putchar...");
    auto nodes_begin = std::begin(tree);
    auto nodes_end = std::end(tree);
    auto r = std::ranges::subrange(nodes_begin, nodes_end);  // only need all and the subrange because TreeNode doesn't have begin() or end()
    auto chars_view = r | std::views::transform(&TreeNode::data) | std::views::join; 
    for(auto c : chars_view)
    {
        putchar(c);
    }
    puts("...done.");
}

#include <iostream>

void test_join_ranges_iostream(TreeNode& tree)
{
    puts("-> test_join_ranges_iostream...");
    auto nodes_begin = std::begin(tree);
    auto nodes_end = std::end(tree);
    auto r = std::ranges::subrange(nodes_begin, nodes_end);  // only need all and the subrange because TreeNode doesn't have begin() or end()
    auto chars_view = r | std::views::transform(&TreeNode::data) | std::views::join; 
    
    //std::cout << chars_view;
    std::ranges::copy(chars_view, std::ostream_iterator<char>(std::cout));

    /*for(auto c : chars_view)
    {
        std::cout << c;
    }
    */

    puts("...done.");
}

#include "fmt/format.h"
#include "fmt/ranges.h"

void test_join_ranges_libfmt(TreeNode& tree)
{
    puts("-> test_join_ranges_libfmt...");
    auto nodes_begin = std::begin(tree);
    auto nodes_end = std::end(tree);
    auto r = std::ranges::subrange(nodes_begin, nodes_end);  // only need all and the subrange because TreeNode doesn't have begin() or end()
    auto chars_view = r | std::views::transform(&TreeNode::data) | std::views::join; 
    
    //fmt::print("{}", chars_view); // unfortunately libfmt does additional formating of the range which is useful for debug logging but can't be removed for "raw" output, so we have to use fmt::join to concatenate the data:
    fmt::print("{}", fmt::join(chars_view, ""));

    puts("...done.");
}

/*
    Another approach would be just traversing the tree and writing each string to the stream with fwrite(), fputs(), send() or write() etc.
    Here we rely on the output stream implementation to deal with strings of any size segmenting them up to fit in packets, buffers, etc. as needed,
    but also results in a write() call for each tree node.
*/

#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cassert>
#include <cerrno>
#include <cstring>

void test_iterate_fputs(TreeNode& tree)
{
    puts("-> test_iterate_fputs...");
    auto r = std::ranges::subrange(std::begin(tree), std::end(tree));

    for (auto &node : r)
    {
        int status = fputs(node.data.c_str(), stdout);
        if (status < 0 || status == EOF)
            throw std::runtime_error(std::string("IO Error writing serialization of tree: ") + strerror(errno));
    }

    puts("...done.");
}


/* Instead of a write for every node, we could use writev() to only issue one write syscall: */

#include <sys/uio.h>
#include <unistd.h>

void test_iterate_gather_then_writev(TreeNode& tree)
{
    puts("-> test_iterate_gather_then_writev...");

    // Gather buffers in iovec structs for writev(). For now use static assuming tree doesn't change. This info could all be cached unchanged if tree doesn't change; we could also realloc iov_arr instead of free and malloc if the tree did change.
    static struct iovec *iov_arr = (struct iovec*)malloc(tree.size * sizeof(struct iovec));
    assert(iov_arr != nullptr);
    struct iovec *iovp = iov_arr;
    size_t total_nbytes = 0;
    size_t count = 0;
    auto r = std::ranges::subrange(std::begin(tree), std::end(tree));
    for (auto &n : r)
    {
        iovp->iov_base = (void*) n.data.c_str();
        iovp->iov_len = n.data.size();
        ++iovp;
        total_nbytes +=  n.data.size(); 
        ++count;
    } 

    assert(count == tree.size); // check that we visited the number of nodes we thought the tree had

    ssize_t n = writev(STDOUT_FILENO, iov_arr, count);
    //free(iov_arr); // disable, iov_arr is static

    if(n < 0)
        throw std::runtime_error(std::string("IO Error writing serialization of tree: ") + strerror(errno));
    if((size_t)n < total_nbytes)
        printf("Warning: data truncated (we calculated %lu bytes, writev returned %ld bytes)\n", total_nbytes, n);

    puts("...done.");
}



/* 
   This is like the  first range version but divides up the stream into chunks and copies each chunk to output (either iostream or a temporary buffer for fputs):
*/

/* 
    In these versions, if a tree node contains a long string we divide it up for multiple writes, otherwise we write the string.
    Our example data has small strings so we use a very small chunk size of 5 characters.
    First, by iterating over all the tree node strings:
*/


/*  Second, with a custom range adapter that returns chunks, either whole strings or sub-strings if long.

*/

void test3(const TreeNode& tree)
{
}



TreeNode tree("first ");
TreeNode n2("second ", &tree);
TreeNode n3("third ", &tree);
TreeNode n4("fourth ", &n3);
TreeNode n5("fifth ", &n3);
TreeNode n6("sixth ", &n3);
TreeNode n7("seventh ", &n6);
TreeNode n8("eighth ", &n6);
TreeNode n9("ninth ", &n6);
TreeNode n10("tenth ", &n3);
TreeNode n11("eleventh ", &n10);
TreeNode n12("twelfth ", &n11);
TreeNode n13("thirteenth ", &n12);




#ifdef ENABLE_BENCHMARK

#include "benchmark/benchmark.h"

static void bench_iterate_fputs(benchmark::State& state) {
  for (auto _ : state) {
    test_iterate_fputs(tree);
  }
}
BENCHMARK(bench_iterate_fputs);

static void bench_iterate_writev(benchmark::State& state) {
  for (auto _ : state) {
    test_iterate_gather_then_writev(tree);
  }
}
BENCHMARK(bench_iterate_writev);

static void bench_join_ranges_putchar(benchmark::State& state) {
  for (auto _ : state) {
    test_join_ranges_putchar(tree);
  }
}
BENCHMARK(bench_join_ranges_putchar);

static void bench_join_ranges_iostream(benchmark::State& state) {
  for (auto _ : state) {
    test_join_ranges_iostream(tree);
  }
}
BENCHMARK(bench_join_ranges_iostream);

static void bench_join_ranges_libfmt(benchmark::State& state) {
  for (auto _ : state) {
    test_join_ranges_libfmt(tree);
  }
}
BENCHMARK(bench_join_ranges_libfmt);



BENCHMARK_MAIN();

#else

int main()
{
  test_iterate_fputs(tree);
  test_iterate_gather_then_writev(tree);
  test_join_ranges_putchar(tree);
  test_join_ranges_iostream(tree);
  test_join_ranges_libfmt(tree);
  return 0;
}

#endif
