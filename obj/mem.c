#include "../include/rsp_core.h"

// memory management

/* memory management and bounds checking */
size_t calc_mem_size(size_t nbytes) {
  size_t basesz = max(nbytes, 16u);
  while (basesz % 16)
    basesz++;

  return basesz;
}

bool gc_check(size_t b, bool aligned)
{
  if (!aligned)
    b = calc_mem_size(b);

  return (FREE + b) >= (RAM + (HEAPSIZE * 8));
}

bool v_in_heap(val_t v, uchr_t* base, uint64_t heapsz)
{
  uptr_t a = addr(v), b = (uptr_t)base, c = (uptr_t)(base + heapsz * 8);
  return a >= b && a <= c;
}

bool p_in_heap(void* p, uchr_t* base, uint64_t heapsz)
{
  uptr_t a = (uptr_t)p, b = (uptr_t)base, c = (uptr_t)(base + heapsz * 8);
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

uchr_t* vm_crealloc(void** m, uint64_t size, bool scrub)
{
  void* new = realloc(*m,size);
  if (!new)
    {
      fprintf(stdout,"[%s:%d:%s]: Exiting due to machine allocation failure.\n", __FILE__, __LINE__, __func__ ); 
      exit(EXIT_FAILURE);
    }

  if (scrub) memset(new,0,size);
  *m = new;
  return new;
}

bool vm_alloc(size_t bs, size_t elct, size_t elsz, void* np, size_t n, ...)
{
  size_t allc_sz = calc_mem_size(bs + elct * elsz);
  bool gc_ran = false;

  if (gc_check(allc_sz,true))
    {
      if (n)
	{
	  val_t bs = pushn(n);
	  va_list va_args;
	  val_t* locals[n];
	  va_start(va_args,n);

	  for (val_t i = 0; i < n; i++)
	    {
	      locals[i] = va_arg(va_args,val_t*);
	      STACK[bs+i] = *(locals[i]);
	    }
      
	  va_end(va_args);
	  gc_run();

	  for (val_t i = 0; i < n; i++)
	    *(locals[i]) = STACK[bs+i];

	  popn(n);
      }
    else
      gc_run();

    gc_ran = true;
    }

  if (np) // null pointer runs the GC pre-emptively but doesn't allocate
    {
      memcpy(np,FREE,8);
      FREE += allc_sz;
    }

  return gc_ran;
}


bool vm_realloc(size_t bs, size_t elct, size_t elsz, val_t* v, size_t n, ...)
{
  size_t allc_sz = calc_mem_size(bs + elct * elsz);
  bool gc_ran = false;

  if (gc_check(allc_sz,true))
    {
      // save the value that requested the collection
	  val_t bs = pushn(n+1);
	  STACK[bs] = *v;
	  bs++;
	  va_list va_args;
	  val_t* locals[n];
	  va_start(va_args,n);

	  for (val_t i = 0; i < n; i++)
	    {
	      locals[i] = va_arg(va_args,val_t*);
	      STACK[bs+i] = *(locals[i]);
	    }

	  va_end(va_args);
	  gc_run();

	  for (val_t i = 0; i < n; i++)
	    *(locals[i]) = STACK[bs+i];

	  bs--;
	  *v = STACK[bs];
	  popn(n+1);

    gc_ran = true;
    }

  size_t vsz = val_size(*v);
  memcpy(FREE,ptr(uchr_t*,*v),vsz);
  FREE += allc_sz;

  return gc_ran;
}



void gc_resize(void)
{
  if (GROWHEAP)
    {
       HEAPSIZE *= 2;
       vm_crealloc((void**)&EXTRA,HEAPSIZE,true);
       GREWHEAP = true;
    }

  else if (GREWHEAP)
    {
       vm_crealloc((void**)&EXTRA,HEAPSIZE,true);
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
  
  if (tp->tp_copy)
      new = tp->tp_copy(tp,v,&FREE);

  else
    {
      size_t asz = rsp_asize(v);
      size_t osz = rsp_size(v);

      memcpy(FREE,frm,osz);
      new = vm_tag(tp,(val_t)FREE);

      FREE += asz;
    }

  car_(frm) = R_FPTR;
  cdr_(frm) = new;
  
  return new;
}

val_t gc_trace(val_t v)
{
  if (!isallocated(v) || in_heap(v,EXTRA,HEAPSIZE))
    return v;

  else if (isfptr(car_(v)))
      return trace_fp(v);

  else
    {
      type_t* to = val_type(v);
      return gc_copy(to,v);
    }
}

val_t vm_relocate(val_t v)
{
  if (!isallocated(v))
    return v;

  else if (isfptr(car_(v)))
    return vm_relocate(trace_fp(v));

  else
    {
      type_t* to = val_type(v);
      return to->tp_relocate(to,v);
    }
}
