#include "vm.hpp"
#include "objectmemory.hpp"
#include "objects.hpp"
#include "event.hpp"
#include "global_cache.hpp"

#include <iostream>

namespace rubinius {
  VM::VM(size_t bytes) : probe(NULL), wait_events(false) {
    om = new ObjectMemory(bytes);
    bootstrap_ontology();

    events = new event::Loop(EVFLAG_FORKCHECK);
    global_cache = new GlobalCache;

    boot_threads();
  }

  VM::~VM() {
    delete om;
    delete events;
    delete global_cache;
  }

  void VM::boot_threads() {
    Thread* thr = Thread::create(this);
    thr->boot_task(this);

    activate_thread(thr);
  }

  OBJECT VM::new_object(Class *cls) {
    return om->new_object(cls, cls->instance_fields->n2i());
  }

  Symbol* VM::symbol(const char* str, size_t size) {
    return (Symbol*)globals.symbols->lookup(this, str, size);
  }

  OBJECT VM::new_struct(Class* cls, size_t bytes) {
    return om->new_object_bytes(cls, bytes);
  }

  void type_assert(OBJECT obj, object_type type, char* reason) {
    if(obj->reference_p() && obj->obj_type != type) {
      throw new TypeError(type, obj, reason);
    } else if(type == FixnumType && !obj->fixnum_p()) {
      throw new TypeError(type, obj, reason);
    }
  }

  void VM::add_type_info(TypeInfo* ti) {
    om->add_type_info(ti);
    ti->state = this;
  }

  OBJECT VM::current_thread() {
    return Qnil;
  }

  void VM::collect() {
    om->collect_young(globals.roots);
    om->collect_mature(globals.roots);
  }

  void VM::run_best_thread() {
    Thread* next = NULL;

    events->poll();

    for(size_t i = 0; i < globals.scheduled_threads->field_count; i++) {
      List* lst = as<List>(globals.scheduled_threads->at(i));
      if(lst->empty_p()) continue;
      next = as<Thread>(lst->shift(this));
    }

    if(!next) {
      if(events->num_of_events() == 0) {
        throw new DeadLock("no runnable threads, present or future.");
      }

      wait_events = true;
      return;
    }

    activate_thread(next);
  }

  void VM::return_value(OBJECT val) {
    globals.current_task->push(val);
  }

  void VM::queue_thread(Thread* thread) {
    List* lst = as<List>(globals.scheduled_threads->at(thread->priority->n2i()));
    lst->append(this, thread);
  }

  void VM::activate_thread(Thread* thread) {
    globals.current_thread.set(thread);
    globals.current_task.set(thread->task);
  }

  OBJECT VM::current_block() {
    return globals.current_task->active->block;
  }

  void VM::raise_from_errno(char* msg) {

  }

  void VM::inspect(OBJECT obj) {
    if(obj->symbol_p()) {
      String* str = as<Symbol>(obj)->to_str(this);
      std::cout << "<Symbol :" << (char*)*str << ">" << std::endl;
    } else if(obj->fixnum_p()) {
      std::cout << "<Fixnum " << as<Fixnum>(obj)->to_nint() << ">" << std::endl;
    } else {
      std::cout << "<Object: " << (void*)obj << ">" << std::endl;
    }
  }

  void VM::set_const(const char* name, OBJECT val) {
    globals.object->set_const(this, (char*)name, val);
  }

  void VM::set_const(Module* mod, const char* name, OBJECT val) {
    mod->set_const(this, (char*)name, val);
  }
};
