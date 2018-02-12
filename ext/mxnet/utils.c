#include "mxnet_internal.h"

struct enumerator_head {
  VALUE obj;
  ID    meth;
  VALUE args;
};

static int
obj_is_step_range(VALUE obj)
{
  struct enumerator_head *eh;

  if (!RB_TYPE_P(obj, T_DATA)) {
    return 0;
  }

  if (!rb_obj_is_kind_of(obj, rb_cEnumerator)) {
    return 0;
  }

  eh = (struct enumerator_head *)DATA_PTR(obj);

  if (!rb_obj_is_kind_of(eh->obj, rb_cRange)) {
    return 0;
  }
  if (eh->meth == rb_intern("step")) {
    if (!RB_TYPE_P(eh->args, T_ARRAY)) {
      return 0;
    }
    return (RARRAY_LEN(eh->args) == 1);
  }

  return 0;
}

static int
extract_range(VALUE obj, VALUE *pbegin, VALUE *pend, int *pexclude_end, VALUE *pstep)
{
  ID id_begin, id_end, id_exclude_end;
  VALUE begin, end, exclude_end, step = Qnil;

  CONST_ID(id_begin,       "begin");
  CONST_ID(id_end,         "end");
  CONST_ID(id_exclude_end, "exclude_end?");

  if (rb_obj_is_kind_of(obj, rb_cRange)) {
    begin = rb_funcallv(obj, id_begin, 0, 0);
    end   = rb_funcallv(obj, id_end,   0, 0);
    exclude_end = rb_funcallv(obj, id_exclude_end, 0, 0);
  }
  else if (obj_is_step_range(obj)) {
    struct enumerator_head *eh = (struct enumerator_head *)DATA_PTR(obj);
    begin = rb_funcallv(eh->obj, id_begin, 0, 0);
    end   = rb_funcallv(eh->obj, id_end,   0, 0);
    exclude_end = rb_funcallv(eh->obj, id_exclude_end, 0, 0);
    step  = RARRAY_AREF(eh->args, 0);
  }
  else {
    return 0;
  }

  if (pbegin)       *pbegin = begin;
  if (pend)         *pend = end;
  if (pexclude_end) *pexclude_end = RTEST(exclude_end);
  if (pstep)        *pstep = step;

  return 1;
}

/* Decompose start, stop, and step components from a slice-like object.
 * These components are interpreted as the components of Python's slice.
 */
static VALUE
utils_m_decompose_slice(VALUE mod, VALUE slice_like)
{
  VALUE begin, end, step = Qnil, components;
  int exclude_end;

  if (rb_obj_is_kind_of(slice_like, rb_cRange)) {
    extract_range(slice_like, &begin, &end, &exclude_end, NULL);
  }
  else if (obj_is_step_range(slice_like)) {
    extract_range(slice_like, &begin, &end, &exclude_end, &step);
  }
  else {
    rb_raise(rb_eTypeError, "unexpected argument type %s (expected Range or Enumerator generated by Range#step)",
             rb_class2name(CLASS_OF(slice_like)));
  }

  if (!NIL_P(step) && NUM2SSIZET(step) < 0) {
    if (!NIL_P(end)) {
      if (!exclude_end) {
        ssize_t end_i = NUM2SSIZET(end);
        if (end_i == 0) {
          end = Qnil;
        }
        else {
          end = SSIZET2NUM(end_i - 1); /* TODO: limit check */
        }
      }
    }
  }
  else if (!NIL_P(end)) {
    if (!exclude_end) {
      ssize_t end_i = NUM2SSIZET(end);
      if (end_i == -1) {
        end = Qnil;
      }
      else {
        end = SSIZET2NUM(end_i + 1); /* TODO: limit check */
      }
    }
  }

  components = rb_ary_new_capa(3);
  rb_ary_push(components, begin);
  rb_ary_push(components, end);
  rb_ary_push(components, step);

  return components;
}

void
mxnet_init_utils(void)
{
  VALUE mUtils;

  mUtils = rb_define_module_under(mxnet_mMXNet, "Utils");
  rb_define_module_function(mUtils, "decompose_slice", utils_m_decompose_slice, 1);
}
