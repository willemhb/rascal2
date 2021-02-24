#include "../include/mem.h"

// stack manipulation
inline void grow_stack()
{
  STACK = vm_crealloc(STACK,STACKSIZE*2,false);
  STACKSIZE *= 2;
  return;
}

inline val_t pop()
{
  assert(SP, BOUNDS_ERR);
  return STACK[SP--];
}

inline val_t push(val_t v)
{
  if (SP >= STACKSIZE)
    grow_stack();

  STACK[++SP] = v;
  return SP-1;
}


val_t pushn(size_t n)
{
  val_t out = SP;
  if (SP + n >= STACKSIZE)
    grow_stack();

  SP += n;
  return out;
}


void popn(size_t n)
{
  assert(SP - n > 0, BOUNDS_ERR);
  SP -= n;
  return;
}


/* memory management and bounds checking */
size_t calc_mem_size(size_t nbytes) {
  size_t basesz = max(nbytes, 16u);
  while (basesz % 16)
    basesz++;

  return basesz;
}

inline size_t val_asizeof(val_t v, type_t* to)
{
  return calc_mem_size(val_sizeof(v,to));
}

inline bool gc_check(void)
{

  return FREE > (RAM + HEAPCRITICAL);
}

bool v_in_heap(val_t v, void* base, uint64_t heapsz)
{
  uptr_t a = addr(v), b = (uptr_t)base, c = (uptr_t)(b + heapsz * 8);
  return a >= b && a <= c;
}

bool p_in_heap(void* p, void* base, uint64_t heapsz)
{
  uptr_t a = (uptr_t)p, b = (uptr_t)base, c = (uptr_t)(b + heapsz * 8);
  return a >= b && a <= c;
}

void* vm_cmalloc(uint64_t nbytes)
{
  void* out = malloc(nbytes);
  if (!out)
    {
      fprintf(stdout,"[%s:%d:%s] Exiting due to machine allocation failure.\n",__FILE__,__LINE__,__func__);
      exit(EXIT_FAILURE);
    }

  memset(out,0,nbytes);
  return out;
}

int32_t vm_cfree(void* m)
{
  free(m);
  return 1;
}

void* vm_crealloc(void* m, uint64_t size, bool scrub)
{
  void* new = realloc(m,size);
  if (!new)
    {
      fprintf(stdout,"[%s:%d:%s]: Exiting due to machine allocation failure.\n", __FILE__, __LINE__, __func__ ); 
      exit(EXIT_FAILURE);
    }

  if (scrub) memset(new,0,size);
  return new;
}

void* vm_alloc(size_t bs, size_t elct, size_t elsz)
{
  size_t allc_sz = calc_mem_size(bs + elct * elsz);
  void* out = FREE;
  FREE += allc_sz;
  return out;
}


void* vm_realloc(val_t v, size_t elct, size_t elsz)
{
  type_t* to = val_type(v);
  size_t curr_sz = val_asizeof(v,to);
  size_t allc_sz = calc_mem_size(to->tp_base_sz + (elct * elsz));

  if (curr_sz >= allc_sz)
    return ptr(void*,v);

  ltag_t  lt = v & LTAG_MASK;
  uchr_t* new = FREE;
  uchr_t* old = ptr(uchr_t*,v);
  FREE += allc_sz;

  memcpy(new,old,curr_sz);
  val_t new_v = (val_t)new | lt;

  car_(old) = R_FPTR;
  cdr_(old) = new_v;

  return new;
}



void gc_resize(void)
{
  if (GROWHEAP)
    {
       HEAPSIZE *= 2;
       EXTRA = vm_crealloc(EXTRA,HEAPSIZE*8,true);
       GREWHEAP = true;
       HEAPCRITICAL = (HEAPSIZE * 3) / 4;
    }

  else if (GREWHEAP)
    {
       EXTRA = vm_crealloc(EXTRA,HEAPSIZE*8,true);
       GREWHEAP = false;
    }
  
  return;
}

void gc_run(void) {
  gc_resize();
  FREE = EXTRA;

  // trace the top level environment
  R_SYMTAB = gc_trace(R_SYMTAB);
  R_MAIN = gc_trace(R_MAIN);
  for (size_t i = 0; i < 4; i++)
    REGISTERS[i] = gc_trace(REGISTERS[i]);

  for (size_t i = SP; i > 0; i--)
    STACK[i] = gc_trace(STACK[i]);

  // swap the fromspace & the tospace
  uchr_t* TMPHEAP = RAM;
  RAM = EXTRA;
  EXTRA = TMPHEAP;

  GROWHEAP = ((val_t*)FREE - (val_t*)RAM) > (HEAPSIZE * RAM_LOAD_FACTOR);

  return;
}

val_t gc_copy(type_t* tp, val_t v)
{
  val_t new; uchr_t* frm = ptr(uchr_t*,v);
  
  if (tp->tp_relocate)
      new = tp->tp_relocate(tp,v,&FREE);

  else
    {
      size_t asz = val_asizeof(v, tp);
      size_t osz = val_sizeof(v, tp);

      memcpy(FREE,frm,osz);
      new = tag(FREE,tp);

      FREE += asz;
    }

  car_(frm) = R_FPTR;
  cdr_(frm) = new;
  
  return new;
}

val_t gc_trace(val_t v)
{
  if (!isallocated(v, NULL) || in_heap(v,EXTRA,HEAPSIZE))
    return v;

  else if (isfptr(car_(v)))
      return trace_fptr(v);

  else
    {
      type_t* to = val_type(v);
      return gc_copy(to,v);
    }
}
