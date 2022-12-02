

#include "assert.h"

int main()
{
  
  rhm_assert(true);

  //int i = 0;
  //rhm_assert_precond(i > 0);

  try {
    char *s = nullptr;
    rhm_assert(s);
  } catch (const rhm::failed_assertion& e)  {
    fmt::print("Caught exception: {}\n", e.what());
    throw e;
  }

  return 0;
}

