#include "wrapper.h"

#pragma weak zend_empty_string
#pragma weak zend_one_char_string

zend_string *ext_php_rs_zend_string_init(const char *str, size_t len, bool persistent) {
  if (!persistent && len <= 1
      && zend_empty_string != NULL
      && zend_one_char_string != NULL && zend_one_char_string[0] != NULL) {
    return len == 0 ? zend_empty_string : ZSTR_CHAR((zend_uchar) *str);
  }
  return zend_string_init(str, len, persistent);
}

void ext_php_rs_zend_string_release(zend_string *zs) {
  zend_string_release(zs);
}

bool ext_php_rs_is_known_valid_utf8(const zend_string *zs) {
  return GC_FLAGS(zs) & IS_STR_VALID_UTF8;
}

void ext_php_rs_set_known_valid_utf8(zend_string *zs) {
  if (!ZSTR_IS_INTERNED(zs)) {
    GC_ADD_FLAGS(zs, IS_STR_VALID_UTF8);
  }
}

const char *ext_php_rs_php_build_id() { return ZEND_MODULE_BUILD_ID; }

void *ext_php_rs_zend_object_alloc(size_t obj_size, zend_class_entry *ce) {
  return zend_object_alloc(obj_size, ce);
}

void ext_php_rs_zend_object_release(zend_object *obj) {
  zend_object_release(obj);
}

zend_executor_globals *ext_php_rs_executor_globals() {
#ifdef ZTS
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_FAST_BULK_STATIC(executor_globals_offset, zend_executor_globals *);
#else
  return TSRMG_FAST_BULK(executor_globals_offset, zend_executor_globals *);
#endif
#else
  return &executor_globals;
#endif
}

zend_compiler_globals *ext_php_rs_compiler_globals() {
#ifdef ZTS
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_FAST_BULK_STATIC(compiler_globals_offset, zend_compiler_globals *);
#else
  return TSRMG_FAST_BULK(compiler_globals_offset, zend_compiler_globals *);
#endif
#else
  return &compiler_globals;
#endif
}

php_core_globals *ext_php_rs_process_globals() {
#ifdef ZTS
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_FAST_BULK_STATIC(core_globals_offset, php_core_globals *);
#else
  return TSRMG_FAST_BULK(core_globals_offset, php_core_globals *);
#endif
#else
  return &core_globals;
#endif
}

sapi_globals_struct *ext_php_rs_sapi_globals() {
#ifdef ZTS
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_FAST_BULK_STATIC(sapi_globals_offset, sapi_globals_struct *);
#else
  return TSRMG_FAST_BULK(sapi_globals_offset, sapi_globals_struct *);
#endif
#else
  return &sapi_globals;
#endif
}

php_file_globals *ext_php_rs_file_globals() {
#ifdef ZTS
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_BULK_STATIC(file_globals_id, php_file_globals *);
#else
  return TSRMG_BULK(file_globals_id, php_file_globals *);
#endif
#else
  return &file_globals;
#endif
}

#ifdef ZTS
void *ext_php_rs_tsrmg_bulk(int id) {
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
  return TSRMG_BULK_STATIC(id, void *);
#else
  return TSRMG_BULK(id, void *);
#endif
}
#endif

sapi_module_struct *ext_php_rs_sapi_module() {
  return &sapi_module;
}

bool ext_php_rs_output_activated() {
  return OG(flags) & PHP_OUTPUT_ACTIVATED;
}

bool ext_php_rs_zend_try_catch(void* (*callback)(void *), void *ctx, void **result) {
  zend_try {
    *result = callback(ctx);
  } zend_catch {
    return true;
  } zend_end_try();

  return false;
}

bool ext_php_rs_zend_first_try_catch(void* (*callback)(void *), void *ctx, void **result) {
  zend_first_try {
    *result = callback(ctx);
  } zend_catch {
    return true;
  } zend_end_try();

  return false;
}

void ext_php_rs_zend_bailout() {
  zend_bailout();
}

zend_op_array *ext_php_rs_zend_compile_string(zend_string *source, const char *filename) {
#if PHP_VERSION_ID >= 80200
  return zend_compile_string(source, filename, ZEND_COMPILE_POSITION_AFTER_OPEN_TAG);
#else
  return zend_compile_string(source, filename);
#endif
}

void ext_php_rs_zend_execute(zend_op_array *op_array) {
  zval local_retval;
  ZVAL_UNDEF(&local_retval);

  op_array->scope = zend_get_executed_scope();

  zend_try {
    zend_execute(op_array, &local_retval);
  } zend_catch {
    destroy_op_array(op_array);
    efree(op_array);
    zend_bailout();
  } zend_end_try();

  zval_ptr_dtor(&local_retval);
  zend_destroy_static_vars(op_array);
  destroy_op_array(op_array);
  efree(op_array);
}


static void copy_constant_array(zval *dst, const zend_array *src) /* {{{ */
{
  zend_string *key;
  zend_ulong idx;
  zval *new_val, *val;

  zend_array *ht = pemalloc(sizeof(zend_array), true);
  zend_hash_init(ht, zend_hash_num_elements(src), NULL, ZVAL_PTR_DTOR, true);

  ZEND_HASH_FOREACH_KEY_VAL(src, idx, key, val) {
    /* constant arrays can't contain references */
    ZVAL_DEREF(val);
    if (key) {
      zend_string *interned_key = zend_string_init_interned(ZSTR_VAL(key), ZSTR_LEN(key), true);
      new_val = zend_hash_add_new(ht, interned_key, val);
      zend_string_release(interned_key);
    } else {
      new_val = zend_hash_index_add_new(ht, idx, val);
    }
    if (Z_TYPE_P(val) == IS_ARRAY) {
      copy_constant_array(new_val, Z_ARRVAL_P(val));
    } else if (Z_TYPE_P(val) == IS_STRING) {
      zend_string *interned_str = zend_string_init_interned(Z_STRVAL_P(val), Z_STRLEN_P(val), true);
      ZVAL_STR(new_val, interned_str);
      Z_TYPE_FLAGS_P(new_val) = 0;
    } else {
      ZVAL_COPY_VALUE(new_val, val);
    }
  } ZEND_HASH_FOREACH_END();

  GC_SET_REFCOUNT(ht, 1);
  GC_ADD_FLAGS(ht, IS_ARRAY_IMMUTABLE | GC_PERSISTENT | GC_NOT_COLLECTABLE);
  Z_ARR_P(dst) = ht;
  Z_TYPE_INFO_P(dst) = IS_ARRAY;
}

zend_constant *ext_php_rs_zend_register_array_constant(const char *name, size_t name_len, const zend_array *arrval, int flags, int module_number) {

  zend_constant c;
  copy_constant_array(&c.value, arrval);
  ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
  c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);

  return zend_register_constant(&c);
}

void ext_php_rs_make_array_immutable(zval *zv) {
  if (Z_TYPE_P(zv) == IS_ARRAY) {
    zend_array *old = Z_ARRVAL_P(zv);
    if (!(GC_FLAGS(old) & IS_ARRAY_IMMUTABLE)) {
      copy_constant_array(zv, old);
      zend_array_destroy(old);
    }
  }
}
