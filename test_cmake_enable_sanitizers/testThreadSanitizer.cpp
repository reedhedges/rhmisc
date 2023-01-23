

/* Race condition (Second thread writes an entry into a std::map while main thread tries to read from it using operator[])
   Then deadlock.
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <map>

typedef std::map<std::string, std::string> map_t;
int global1;
long global2;

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

void *threadfunc(void *p) {
  map_t& m = *(map_t*)p;
  //pthread_mutex_lock(&mutex2);
  for(int i = 0; i < 5; ++i)
  {
    usleep(100);
    m["foo"] = std::string("bar") + std::to_string(i);
    global1 = i;
    global2 = (long)i;
  }
  //puts("writer thread locking mutex for deadlock...");
  //pthread_mutex_lock(&mutex1);
  //sleep(3);
  //pthread_mutex_unlock(&mutex2);
  return 0;
}

int main() {
  map_t m;
  //pthread_mutex_init(&mutex1, NULL);
  pthread_t t;
  pthread_create(&t, 0, threadfunc, &m);
  //pthread_mutex_lock(&mutex1);
  for(int i = 0; i < 5; ++i)
  {
    usleep(100);
    global1 = i;
    global2 = (long)i;
    printf("main thread: foo=%s g1=%d g2=%ld\n", m["foo"].c_str(), global1, global2);
  }
  //sleep(3);
  //pthread_mutex_lock(&mutex2);
  pthread_join(t, 0);
  //pthread_mutex_destroy(&mutex1);
  //pthread_mutex_destroy(&mutex2);
  return 0;
}
