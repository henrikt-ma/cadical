#ifndef _clause_hpp_INCLUDED
#define _clause_hpp_INCLUDED

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

typedef int *                                  literal_iterator;
typedef const int *                      const_literal_iterator;

/*------------------------------------------------------------------------*/

// The 'Clause' data structure is very important. There are usually many
// clauses and accessing them is a hot-spot.  Thus we use common
// optimizations to reduce their memory foot print and improve cache usage.
// Even though this induces some complexity in understanding the actual
// implementation, arguably not the usage of this data-structure, we
// consider two optimizations as essential.
//
//   (1) The most important optimization is to 'embed' the actual literals
//   in the clause.  This requires a variadic size structure and thus
//   strictly is not 'C' conform, but supported by all compilers we used.
//   The alternative is to store the actual literals somewhere else, which
//   not only needs more memory but more importantly also requires another
//   memory access and thus is very costly.
//
//   (2) Clauses with size at least 'opts.posize' have a '_pos' field, which
//   contains  the position of the last exchanged watch in a long clause.
//   This field is only present if 'extended' is true.  Saving in '_pos'
//   starts making sense for clauses of size 4 and we would typically have
//   'opts.posize = 4'.
//
// If you want to add few additional boolean flags, add them after the
// sequence of already existing ones and adjust 'LD_MAX_GLUE' accordingly,
// i.e., adding one more flag reduces the latter by one.  In general these
// optimizations are local to 'clause.[hc]pp' and otherwise can be ignored
// except that you should for instance never access '_pos' of a clauses
// which is not extended.  This can be checked with for instance 'valgrind'
// but is also guarded by making the actual '_pos' field private and
// checking this contract in the 'pos ()' access functions.

// The glucose level (LBD or short 'glue') is a heuristic value of the
// usefulness of a learned clause, where smaller glue is consider more
// useful.  During learning the 'glue' is determined as the number of
// decisions needed to learn this clause, or equivalently the number of
// decisions necessary to make it propagating (together with other clauses
// in the formula).  See the IJCAI'09 paper by Audemard & Simon for more
// details.  We switched back and forth between keeping the glue stored in
// a clause and using it only initially to determine whether it is kept,
// that is survives clause reduction.  The latter strategy is not bad but
// also does not allow to use glue values elsewhere.  The glues are not big
// anyhow and thus we keep it in the remaining 'LD_MAX_GLUE' bits in the
// clause which make up a 32-bit word after accounting for boolean flags.
// Thus 'LD_MAX_GLUE' should be '32 - #flags'.  The maximum glue value saved
// is then put into this bit-field, and since we want to keep it signed
// (in C bit-fields need explicit 'signed' or 'unsigned' attributes in C),
// we waste another bit.  Currently with 13 boolean flags we have 19 bits
// for the glue field, and thus have 'MAX_GLUE = 2^18 - 1'.

#define LD_MAX_GLUE 19
#define MAX_GLUE    ((1<<(LD_MAX_GLUE-1))-1)

class Clause {

public:

  int _pos;         // position of last watch replacement

  // Keep start of clause and 'copy' field (and thus 'literals[0]' at 64-bit
  // aligned offsets, no matter whether we have a '_pos' field or not.
  // Otherwise a binary clause does not have 16 bytes.  Keeping clauses at
  // 64-bit aligned addresses gives around 5% speed improvement.
  //
  int alignment;    // unused four bytes alignment

  bool extended:1;  // has this '_pos' field (and 'alignment')

  bool redundant:1; // aka 'learned' so not 'irredundant' (original)
  bool stable:1;    // keep this clause during stabilization (if redundant)
  bool keep:1;      // always keep this clause (if redundant)

  bool garbage:1;   // can be garbage collected unless it is a 'reason'
  bool reason:1;    // reason / antecedent clause can not be collected
  bool moved:1;     // moved during garbage collector ('copy' valid)

  bool hbr:1;       // redundant hyper binary resolved clause (size == 2)
  bool used:1;      // 'hbr' resolved during conflict analysis

  bool vivify:1;    // clause scheduled to be vivified
  bool vivified:1;  // redundant clause already tried to vivify
  bool ignore:1;    // ignore during (vivify) propagation

  bool transred:1;  // already checked for transitive reduction

  signed int glue : LD_MAX_GLUE;

  int size;         // actual size of 'literals' (at least 2)

  union {

    int literals[2];    // of variadic 'size' (not just 2) in general

    Clause * copy;      // only valid if 'moved', then that's where to

    // The 'copy' field is only used for 'moved' clauses in 'copy_clause'
    // in the moving garbage collector 'copy_non_garbage_clauses'.
    // Otherwise 'literals' is valid.
  };

        int & pos ()       { assert (extended); return _pos; }
  const int & pos () const { assert (extended); return _pos; }

  literal_iterator       begin ()       { return literals; }
  literal_iterator         end ()       { return literals + size; }

  const_literal_iterator begin () const { return literals; }
  const_literal_iterator   end () const { return literals + size; }

  // Actual start of allocated memory, bytes allocated and offset are only
  // used for memory (de)allocation in 'delete_clause' and in the moving
  // garbage collector 'copy_non_garbage_clauses' and 'copy_clause'.
  //
  char * start () const;        // actual start of allocated memory
  size_t bytes () const;        // actual number of bytes allocated
  size_t offset () const;       // offset of valid bytes (start - this)

  // Check whether this clause is ready to be collected and deleted.  The
  // 'reason' flag is only there to protect reason clauses in 'reduce',
  // which does not backtrack to the root level.  If garbage collection is
  // triggered from a preprocessor, which backtracks to the root level, then
  // 'reason' is false for sure. We want to use the same garbage collection
  // code though for both situations and thus hide here this variance.
  //
  bool collect () const { return !reason && garbage; }
};

struct smaller_size {
  bool operator () (const Clause * a, const Clause * b) {
    return a->size < b->size;
  }
};

/*------------------------------------------------------------------------*/

inline size_t Clause::offset () const {
  size_t res = 0;
  if (!extended) res += sizeof _pos + sizeof alignment;
  assert (aligned (res, 8));
  return res;
}

inline size_t Clause::bytes () const {
  size_t res = sizeof (Clause);
  res += (size - 2) * sizeof (int);
  res -= offset ();
  res = align (res, 8);
  return res;
}

inline char * Clause::start () const {
  return offset () + (char*) this;
}

/*------------------------------------------------------------------------*/

// Place literals over the same variable close to each other.  This allows
// eager removal of identical literals and detection of tautological clauses
// and also is easier to read for debugging.

struct lit_less_than {
  bool operator () (int a, int b) const {
    int s = abs (a), t = abs (b);
    return s < t || (s == t && a < b);
  }
};

};

#endif
