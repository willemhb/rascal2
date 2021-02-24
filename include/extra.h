
// strings, bstrings and core atoms api (string.c)
str_t*    mk_str(chr_t*);
bstr_t*   mk_bstr(size_t,uchr_t*);
atom_t*   new_atom(chr_t*,size_t,uint16_t,hash_t);
val_t     copy_str(type_t*,val_t,uchr_t**);
val_t     copy_bstr(type_t*,val_t,uchr_t**);
size_t    str_size(obj_t*);
size_t    bstr_size(obj_t*);
int32_t   ord_str(val_t,val_t);
int32_t   ord_bstr(val_t,val_t);
hash_t    hash_str(val_t);
hash_t    hash_bstr(val_t);
hash_t    hash_atom(val_t);
void      prn_str(val_t,iostrm_t*);
void      prn_bstr(val_t,iostrm_t*);
void      prn_atom(val_t,iostrm_t*);
chr_t*    strval(val_t);

// tables, sets, and dicts (table.c)
table_t*   mk_table(size_t,tpkey_t);
dict_t*    mk_dict(size_t);
set_t*     mk_set(size_t);
val_t      copy_table(type_t*,val_t);
val_t*     tb_lookup(table_t*,val_t);
val_t*     tb_addkey(table_t*,val_t);
val_t*     tb_setkey(table_t*,val_t,val_t);
val_t*     tb_rmvkey(table_t*,val_t);
void       prn_dict(val_t,iostrm_t*);
void       prn_set(val_t,iostrm_t*);
atom_t*    mk_atom(chr_t*,uint16_t);
atom_t*    intern_string(chr_t*,hash_t,uint16_t);

// callable types
builtin_t* mk_builtin(void*,size_t,bool);
closure_t* mk_closure(void*,nmspc_t*,envt_t*,size_t,uint16_t);
method_t*  mk_method(code_t*,nmspc_t*,envt_t*,module_t*,size_t,uint16_t);
val_t      call_builtin(builtin_t*,size_t,val_t*);
int32_t    check_argco(obj_t*,size_t);

// direct data (direct.c)
val_t     mk_bool(int32_t);
val_t     mk_char(int32_t);
val_t     mk_int(int32_t);
val_t     mk_float(flt32_t);
hash_t    hash_small(val_t);
void      prn_int(val_t,iostrm_t*);
void      prn_float(val_t,iostrm_t*);
void      prn_char(val_t,iostrm_t*);
void      prn_bool(val_t,iostrm_t*);
int32_t   ord_ival(val_t,val_t);
int32_t   ord_fval(val_t,val_t);
val_t     val_itof(val_t);              // floating point conversion 
val_t     val_ftoi(val_t);              // integer conversion

/* 
   vm.c - environment API, the VM, and the compiler 
 */

nmspc_t*  mk_nmspc(size_t,list_t*);
envt_t*   mk_envt(size_t,envt_t*,module_t*);
int32_t   env_locate(envt_t*,atom_t*,size_t*,size_t*);
pair_t*   env_extend(envt_t*,atom_t*);
size_t    env_bind(envt_t*,val_t);
val_t     env_lookup(tuple_t*,size_t,size_t);
val_t     env_assign(tuple_t*,size_t,size_t,val_t);

// the virtual machine and compiler           
opcode_t  vm_fetch_instr();                     
val_t     vm_exec(code_t*,envt_t*);
code_t*   vm_compile(val_t,envt_t*,module_t*); 
val_t     vm_expand(val_t,envt_t*,pair_t*);

// reader/printer support functions
rsp_tok_t vm_get_token(iostrm_t*);
val_t     vm_read_expr(iostrm_t*);
val_t     vm_read_cons(iostrm_t*);                   // read a cons or list
val_t     vm_read_bytes(iostrm_t*);                  // optimized reader for bytestring literals
val_t     vm_read_coll(type_t*,iostrm_t*,rsp_tok_t);
void      vm_print(val_t,iostrm_t*);
val_t     vm_load(iostrm_t*,val_t);


// io utilities
int32_t peekwc(iostrm_t*);
int32_t fskip(iostrm_t*);
int32_t fskipto(iostrm_t*,cint32_t);

/* rspinit.c */
// memory
void init_heap();
void init_types();
void init_registers();

// builtins
void init_symbol_table();
void init_builtin_types();
void init_special_forms();
void init_global_variables();
void init_builtin_functions();

/* toplevel bootstrapping and cleanup functions */
void bootstrap_rascal();
int  teardown_rascal();

/* rspmain.c */
int rsp_repl();
int rsp_main(int32_t,chr_t**);
