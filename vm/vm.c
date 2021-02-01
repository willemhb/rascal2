#include "rascal.h"
#include "vm.h"

const size_t OPCODE_SIZES[OP_STORE_VAR + 1] =
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 5, 5,
    9, 9,
  };

/*
  virtual machine, evaluator, and function call APIs.
*/


inline uchr8_t*  code_instr(code_t* co)
{
  bstr_t* instr = (bstr_t*)vec_meta(co)[0];
  return bs_bytes(instr);
}
val_t*   code_values(code_t* co)       { return vec_elements(co); }
table_t* envt_names(envt_t* e)         { return (table_t*)vec_meta(e)[0]; }
envt_t*  envt_next(envt_t* e)          { return (envt_t*)vec_meta(e)[1]; }
envt_t*  envt_nmspc(envt_t* e)         { return (envt_t*)vec_meta(e)[2]; }
val_t*   envt_bindings(envt_t* e)      { return vec_elements(e); }
size_t   envt_nbindings(envt_t* e)     { return envt_names(e)->count; }

/* environment API functions */

// find the location of a name in the current environment
// first search local environments, then search namespaces
int32_t env_locate(envt_t* e, sym_t* n, val_t* loc, val_t* value)
{
  envt_t* original = e;
  table_t* nmspc; val_t sn = tagp(n);
  node_t* rslt = NULL; int32_t xo = 0, yo = 0, no = 0;

  if (vec_modenvt(e))
    {
      while (!rslt && e)
	{
	  nmspc = envt_names(e);
	  rslt = table_lookup(nmspc,sn);
	  no++;
	  e = envt_next(e);
	}

      if (rslt)
	{
	  if (loc)
	    *loc = tagp(nmspc);

	  *value = nd_data(rslt);
	  return no;
	}
      else
	{
	  if (loc)
	    *loc = R_NIL;

	  *value = R_UNBOUND;
	  return -1;
	}
    }
  else
    {
      cons_t* xy;
      for (;!rslt && e;xo++,e=envt_next(e))
	{
	  nmspc = envt_names(e);
	  rslt = table_lookup(nmspc,sn);
	}
      if (rslt)
	{
	  yo = nd_order(rslt);
	  *value = envt_bindings(e)[yo];
	  xy = mk_cons(tagi(xo),tagi(yo));
	  if (loc)
	    *loc = tagp(xy);
	  return 0;
	}
      else
	return env_locate(envt_nmspc(original),n,loc,value);
    }
}

node_t* env_extend(envt_t* e, sym_t* n)
{
  table_t* names = envt_names(e);

  if (sm_reserved(n))
    rsp_raise(NAME_ERR);

  val_t sv = tagp(n);
  uint32_t ord = tb_order(names);        // to test if a new binding was added
  node_t* rslt = table_insert(names,sv);

  if (tb_order(names) == ord)
    rsp_raise(NAME_ERR);

  if (!vec_modenvt(e))                   // ensure space is reserved in the local bindings
    {                                    // for the new name
      vec_append(e,tagp(rslt));          // lazy measure to ensure the new value is collected
      
      envt_bindings(e)[vec_curr(e)] = R_UNBOUND;
    }

  return rslt;
}

size_t env_bind(envt_t* e, val_t v)
{
  if (vec_curr(e) == envt_nbindings(e))
    rsp_raise(BOUNDS_ERR);

  vec_append(e,v);
  return vec_curr(e);
}

val_t local_lookup(envt_t* e, size_t xo, size_t yo)
{
  for (;e && xo;xo--,e=envt_next(e))
    continue;

  if (xo)
    rsp_raise(UNBOUND_ERR);

  return envt_bindings(e)[yo];
}

val_t module_lookup(envt_t* e, sym_t* nm)
{
  val_t sv = tagp(nm);
  table_t* names = envt_names(e);
  node_t* rslt = table_lookup(names,sv);

  if (!rslt)
    rsp_raise(UNBOUND_ERR);

  return nd_binding(rslt);
}

val_t local_assign(envt_t* e, val_t b, size_t xo, size_t yo)
{
  for (;e && xo;xo--,e=envt_next(e))
    continue;

  if (xo)
    rsp_raise(UNBOUND_ERR);

  envt_bindings(e)[yo] = b;

  return b;
}


val_t module_assign(envt_t* e, sym_t* nm, val_t b)
{
  val_t sv = tagp(nm);
  table_t* names = envt_names(e);
  node_t* rslt = table_lookup(names,sv);

  if (!rslt)
    rsp_raise(UNBOUND_ERR);

  cons_t* d = ptr(cons_t*,nd_data(rslt));
  cdr_(d) = b;
  return b;
}

// compilation helpers
size_t code_add_value(code_t* co, val_t v)
{
  val_t* vals = code_values(co);
  size_t code_curr = vec_curr(co);
  size_t i = 0;

  for (; i < code_curr; i++)
    if (vals[i] == v)
      break;

  if (i == code_curr)
    vec_append(co,v);

  return i;
}

static inline instr_size(opcode_t o)
{
  return OPCODE_SIZES[o];
}

static flabel_t vm_form_type(val_t exp)
{
  switch (vtag(exp))
    {
    default: return LBL_LITERAL;
    case VTAG_SYM: return LBL_SYMBOL;
    case VTAG_LIST:
      {
	if (!exp) return LBL_LITERAL;
	val_t f = first(exp);

	if (f == F_SETV) return LBL_SETV;
	else if (f == F_DEF) return LBL_DEF;
	else if (f == F_QUOTE) return LBL_QUOTE;
	else if (f == F_IF) return LBL_IF;
	else if (f == F_FUN) return LBL_FUN;
	else if (f == F_MACRO) return LBL_MACRO;
	else if (f == F_DO) return LBL_DO;
	else if (f == F_LET) return LBL_LET;
	else return LBL_FUNAPP;
      }
    }
}

code_t* vm_compile(val_t v, envt_t* e, code_t* co)
{
  if (!co)
    {
      
    }
}

inline void vm_fetch_instr(void)
{
  uchr8_t* instr = code_instr(CODE);
  OP = instr[PC];
  PC++;

  if (OP > OP_BINDV)
    {
      ARG(0) = *((unsigned*)(instr + PC));
      PC += 4;
    }
  if (OP > OP_BINDN)
    {
      ARG(1) = *((unsigned*)(instr + PC));
      PC += 4;
    }

  return;
}

val_t vm_exec(code_t* co, envt_t* env)
{
  static void* exec_labels[] = {
    &&op_halt, &&op_push, &&op_pop, &&op_save, &&op_load_name,
    &&op_store_name, &&op_call, &&op_bindv, &&op_load_gval,
    &&op_load_gstrm, &&op_restore, &&op_jump, &&op_branch,
    &&op_bindn, &&op_load_var, &&op_store_var,};

  unsigned argco, argmin, argmax;
  bool vargs;
  type_t* to;
  table_t* nmspc;
  dvec_t* efrm;

  goto exec_loop;
  
 exec_loop:
  vm_fetch();
  vm_decode();
  goto *exec_labels[OP];

 op_halt:
  return VALUE;

 op_push:
  PUSH(VALUE);
  goto exec_loop;

 op_pop:
  VALUE = POP();
  goto exec_loop;

 op_save:
  CONT = vm_mk_contframe(ARG(1));
  goto exec_loop;

 op_restore:
  

 op_load_name:
  nmspc = isnmspc(ENVT) ? totable_(ENVT) : totable_(efrm_nmspc_(ENVT));
  WRX(0) = nmspc_lookup(nmspc,VALUE);
  VALUE = WRX(0);
  goto exec_loop;

 op_store_name:
  nmspc = isnmspc(ENVT) ? totable_(ENVT) : totable_(efrm_nmspc_(ENVT));
  WRX(0) = POP();
  WRX(1) = nmspc_assign(nmspc,VALUE,WRX(1));
  VALUE = WRX(1);
  goto exec_loop;

 op_call:
  argco = SP - BP;

  if (istype(VALUE)) // replace current contents of VALUE with the type's constructor
    {
      VALUE = totype_(VALUE)->tp_new;
    }

  if (isbuiltin(VALUE))
    {
      ARG(0) = uval_(VALUE);
      goto op_call_bltn;
    }

  else if (isfunction(VALUE))
    {
      FUNC = template_(VALUE);
      argmin = argco_(VALUE);
      argmax = hasvargs(VALUE) ? UINT32_MAX : argmin;

      if (argco < argmin || argco > argmax) rsp_raise(ARITY_ERR);
      
      ENVT = vm_mk_envtframe(fn_envt_(VALUE),fn_closure_(VALUE),EVAL,argco);
      PC = 0;
    }

  else rsp_raise(TYPE_ERR);

  goto exec_loop;

 op_call_bltn:
  WRX(0) = BUILTIN_FUNCTION_VARGS[ARG(0)];
  vargs = WRX(0) > 0;
  argmin = BUILTIN_FUNCTION_ARGCOS[ARG(0)];
  argmax = min(UINT32_MAX, argmin + WRX(0));

  if (argco < argmin || argco > argmax) rsp_raise(ARITY_ERR);
  if (vargs) VALUE = BUILTIN_FUNCTIONS[ARG(0)].ffunc(EVAL);
  else VALUE       = BUILTIN_FUNCTIONS[ARG(0)].vfunc(EVAL,argco);

 op_bindv:
  env_bind(ENVT,VALUE);
  goto exec_loop;

 op_load_gval:
  VALUE = R_GLOBALS[ARG(0)];
  goto exec_loop;

 op_load_gstrm:
  VALUE = R_STREAMS[ARG(0)];
  goto exec_loop;

 op_jump:
  PC += ARG(0);
  goto *exec_labels[OP];

 op_branch:
  if (cbool(VALUE)) goto op_jump;
  else goto exec_loop;

 op_load_var:
  efrm = todvec_(ENVT);
  WRX(0) = efrm_lookup(efrm,ARG(0),ARG(1));
  VALUE = WRX(0);
  goto exec_loop;

 op_store_var:
  efrm = todvec_(ENVT);
  WRX(0) = POP();
  WRX(1) = efrm_assign(efrm,ARG(0),ARG(1),WRX(0));
  VALUE = WRX(1);
  goto exec_loop;
}
