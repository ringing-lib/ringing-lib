// Random useful things

#ifndef STUFF_H
#define STUFF_H

// A buffer which allocates and frees itself
class buffer {
private:
  char *p;
  int s;

public:
  buffer(int siz) : s(siz) { p = new char[s]; }
  ~buffer() { delete[] p; }
  
  int size() { return s; }
  operator char*() { return p; }
  operator const char *() const { return p; }
  char& operator[](int i) { return p[i]; }
  const char& operator[](int i) const { return p[i]; }
};

#endif  


