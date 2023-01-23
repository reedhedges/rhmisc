

#include <cstdlib>
#include <cstdio>


// leak
char *leak()
{
  char *mem = (char*)malloc(10);
  return mem;
}

// read from bad (uninitialized) pointer. may result in error about alignment
void read_unalloc1()
{
  int *p1;
  int x = *p1;
}

// read from null pointer
void null_ptr()
{
  int *p = nullptr;
  int x = *p;
}

// increment pointer beyond our stack, read from pointer to unallocated memory (but a reasonable pointer?)
void read_unalloc2()
{
  int x = 0;
  int *p = &x;
  p += 4;
  int y = *p;
}

// increment pointer beyond allocated memory, read
void read_unalloc3()
{
  int *p = new int[2];
  ++p; // p[1]
  ++p; // p[2], one off
  int y = *p;
  delete[] p;
}
  


// write to unallocated memory (UB will probably manifest here, crash or otherwise.)
void write_unalloc()
{
  int *p;
  *p = 23;
}

// write to unallocated memory
void write_unalloc2()
{
  int *p = new int[2];
  p[5] = 23;
  delete[] p;
}


// read uninitialized memory
void read_uninit()
{
  char *buf = (char*)malloc(10);
  char x = buf[2];
  //printf("%c\n", x);
  free(buf);
}
  
char *violate_nonnull_return() __attribute__((returns_nonnull));

char *violate_nonnull_return()
{
  return NULL;
}

void nonnull_arg(int *ptr __attribute((nonnull)) )
{
}

void overflow()
{
  int n = 0xffffffff;
  ++n;
}

void stack_array_bounds()
{
  int pre[4];
  int a[5];
  int post[4];
  a[5] = 23; // one past
  //a[-1] = 42; // one before?
}

void heap_array_bounds()
{
  int *a = new int[5];
  a[5] == 23; // one past
  delete[] a;
}

void div_by_zero()
{
  float d = 0.0;
  float x = 1.0f/d;

  int n = 0;
  int y = 1/n;
}

void f(int x)
{
  printf("%d\n", x);
}

typedef void(*floatfunc)(float);

void bad_func_ptr_call()
{
  floatfunc fptr = (floatfunc)&f;
  fptr(3.14);
}

int no_return_val()
{
}

void wrong_delete1()
{
  int *a = new int[5];
  delete a;
}

void wrong_delete2()
{
  int *a = new int;
  delete[] a;
}

int main()
{
  puts("--------\ntestSanitizers begin...");
  //no_return_val();
  puts("...integer value overflow...");
  overflow();
  puts("...wrong delete 1...");
  wrong_delete1();
  puts("...wrong delete 2...");
  wrong_delete2();
  puts("...array out of bounds...");
  stack_array_bounds();
  puts("...divisions by zero...");
  div_by_zero();
  //puts("...bad cast and call of function pointer...");
  //bad_func_ptr_call();
  puts("...read uninitialized memory...");
  read_uninit();
  //int x; nonnull_arg(&x);
  puts("...pass argument with nonnull attribute...");
  nonnull_arg(nullptr);
  puts("...return null from nonnull function...");
  violate_nonnull_return();
  puts("...leak (will see sanitizer error after program exits)...");
  char *l = leak();
  puts("...read from unallocated memory 1 (uninitialized pointer)...");
  read_unalloc1();
  puts("...read from unallocated memory 2 (update pointer beyond valid stack memory, read)...");
  read_unalloc2();
  puts("...read from unallocated memory 3 (update pointer beyond valid heap memory, read)...");
  read_unalloc3();
  puts("...write to unallocated memory (things will probbaly go bad here...");
  write_unalloc2();
  puts("...write to unallocated memory (things will probbaly go bad here...");
  write_unalloc();
  puts("...use null pointer...");
  null_ptr();
  puts("--------\ntestSanitizers done.");
  return 0;
}

