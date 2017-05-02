#ifndef __MINIMIZER_H_INCLUDED__   // if minimizer.h hasn't been included yet...
#define __MINIMIZER_H_INCLUDED__

#include <string>
#include <functional>
#include <ostream>
#include <stdint.h>
#include "interval.h"

struct Minimizer
{
    uint64_t kmer;
    Interval pos;
    bool strand;
    bool operator < ( const Minimizer& y) const;
    bool operator == (const Minimizer& y) const;
    Minimizer(uint64_t, uint32_t, uint32_t, bool);
    ~Minimizer();
    friend std::ostream& operator<< (std::ostream& out, const Minimizer& m); 
};

struct MiniPos
{
  bool operator()(Minimizer* lhs, Minimizer* rhs);
};

struct pMiniComp
{
  bool operator()(Minimizer* lhs, Minimizer* rhs);
};


#endif
