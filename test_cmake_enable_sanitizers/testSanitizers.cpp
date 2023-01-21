

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
  int x[4];
  int *p = x;
  p += 4;
  int y = *p;
}


// write to unallocated memory (UB will probably manifest here, crash or otherwise.)
void write_unalloc()
{
  int pre[4];
  int *p;
  int post[4];
  *p = 23;
}

// read uninitialized memory
void read_uninit()
{
  char *buf = (char*)malloc(10);
  char x = buf[2];
  //printf("%c\n", x);
}
  
char *violate_nonnull_return() __attribute__((returns_nonnull))
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

void array_bounds()
{
  int pre[4];
  int a[5];
  int post[4];
  a[5] = 23; // one past
  //a[-1] = 42; // one before?
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

int main()
{
  puts("--------\ntestSanitizers begin...");
  //no_return_val();
  puts("...overflow...");
  overflow();
  puts("...array out of bounds...");
  array_bounds();
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
  puts("...read from unallocated memory 2 (update pointer beyond valid memory)...");
  read_unalloc2();
  //puts("...write to unallocated memory (things will probbaly go bad here...");
  //write_unalloc();
  puts("...use null pointer...");
  null_ptr();
  puts("--------\ntestSanitizers done.");
  return 0;
}

