#include "objects.hpp"
#include "vm.hpp"
#include "objectmemory.hpp"
#include "gc_debug.hpp"

#include <cxxtest/TestSuite.h>

#include <map>
#include <vector>

using namespace rubinius;

class TestVM : public CxxTest::TestSuite {
  public:

#undef state
  VM *state;

  void setUp() {
    state = new VM();
  }

  void tearDown() {
    delete state;
  }

  void test_symbol() {
    Symbol *sym1 = state->symbol("blah");
    Symbol *sym2 = state->symbol("blah");

    TS_ASSERT_EQUALS(sym1, sym2);
  }

  void test_new_object_uses_field_count_from_class() {
    Class* cls = state->new_class("Blah", G(object), 3);

    OBJECT blah = state->new_object(cls);

    TS_ASSERT_EQUALS(blah->field_count, 3);
  }

  void test_globals() {
    TS_ASSERT_EQUALS(state->globals.roots.size(), 121);
  }

  void test_collection() {
    std::map<int, OBJECT> objs;

    int index = 0;
    Roots::iterator i;
    for(i = state->globals.roots.begin(); i != state->globals.roots.end(); i++) {
      OBJECT tmp = (*i)->get();
      if(tmp->reference_p() && tmp->zone == YoungObjectZone) {
        objs[index] = tmp;
      }
      index++;
    }

    //std::cout << "young: " << index << " (" <<
    //  state->om->young.total_objects << ")" << std::endl;

    state->om->collect_young(state->globals.roots);

    index = 0;
    for(i = state->globals.roots.begin(); i != state->globals.roots.end(); i++) {
      if(OBJECT tmp = objs[index]) {
        TS_ASSERT((*i)->get() != tmp);
      }
      index++;
    }

    HeapDebug hd(state->om);
    hd.walk(state->globals.roots);

    //std::cout << "total: " << hd.seen_objects << " (" <<
    //  state->om->young.total_objects << ")" << std::endl;
  }

};
