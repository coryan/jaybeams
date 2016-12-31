/// Compile with -DTYPENAME_MACRO=float2 or -DTYPENAME_MACRO=double2
typedef TYPENAME_MACRO T;

/// Complex number conjugate
inline T cconj(T a) {
  return (T)(a.x, -a.y);
}

/// Complex number multiplication
inline T cmul(T a, T b) {
  return (T)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

/**
 * Interpret @a a, @a b, and @a dst as vectors of @a N complex numbers
 * and compute:
 *
 * dst(i) = conj(a(i)) * b(i)
 *
 * This is used when computing a cross correlation via FFTs, as this
 * expression is the frequency space version of cross-correlation.
 */
__kernel void conjugate_and_multiply(
    __global T* dst, __global T const* a, __global T const* b,
    unsigned int const N) {
  // Get our global thread ID
  int col = get_global_id(0);

  if (col < N) {
    dst[col] = cmul(cconj(a[col]), b[col]);
  }
}
