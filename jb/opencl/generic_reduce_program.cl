/* -*- c -*- */

// typedef ... reduce_input_t;
// typedef ... reduce_output_t;

// void reduce_initialize(reduce_output_t* dst);
// void reduce_transform(
//    reduce_output_t* dst, reduce_input_t const* src, unsigned long offset);
// void reduce_combine(
//    reduce_output_t* accumulated, reduce_output_t const* value);

inline void reduce_combine_local2local(
    __local reduce_output_t* accumulated, __local reduce_output_t* value) {
  reduce_output_t tmp = *value;
  reduce_output_t acc = *accumulated;
  reduce_combine(&acc, &tmp);
  *accumulated = acc;
}

inline void reduce_transform_global(
    reduce_output_t* lhs,
    __global reduce_input_t const* src, unsigned long offset) {
  reduce_input_t val = src[offset];
  reduce_transform(lhs, &val, offset);
}

void reduce_scratch(__local reduce_output_t* scratch, uint size);

// First phase of a generic transform-reduce.  Need to transform
__kernel void generic_transform_reduce_initial(
    __global reduce_output_t* dst,
    unsigned long VPT, unsigned long TPB,
    unsigned long const N , __global reduce_input_t const* src,
    __local reduce_output_t* scratch) {
  uint const offset = get_group_id(0) * TPB;
  uint const lid = get_local_id(0);

  reduce_output_t p;
  reduce_initialize(&p);
  for (uint i = 0; i < VPT; ++i) {
    ulong loc = offset + lid + i * get_global_size(0);
    if (loc < N) {
      reduce_output_t tmp;
      reduce_transform_global(&tmp, src, loc);
      reduce_combine(&p, &tmp);
    }
  }
  scratch[lid] = p;
  reduce_scratch(scratch, TPB);
  if (lid == 0) {
    dst[get_group_id(0)] = scratch[0];
  }
}

__kernel void generic_transform_reduce_intermediate(
    __global reduce_output_t* dst,
    unsigned long VPT, unsigned long TPB,
    unsigned long const N, __global reduce_output_t const *src,
    __local reduce_output_t* scratch) {
  uint const offset = get_group_id(0) * TPB;
  uint const lid = get_local_id(0);
  
  reduce_output_t p;
  reduce_initialize(&p);
  for (uint i = 0; i < VPT; ++i) {
    uint loc = offset + lid + i * get_global_size(0);
    if (loc < N) {
      reduce_output_t tmp = src[loc];
      reduce_combine(&p, &tmp);
    }
  }
  scratch[lid] = p;
  reduce_scratch(scratch, TPB);
  if (lid == 0) {
    dst[get_group_id(0)] = scratch[0];
  }
}

void reduce_scratch(__local reduce_output_t* scratch, uint size) {
  uint const lid = get_local_id(0);
  for (int i = size / 2; i >= 1; i >>= 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
    // keep the processing threads consolidated ...
    if (lid < i) {
      reduce_combine_local2local(scratch + lid, scratch + lid + i);
    }
  }
  barrier(CLK_LOCAL_MEM_FENCE);
}

__kernel void scratch_element_size(__global ulong* result) {
  uint const lid = get_local_id(0);
  uint const gid = get_local_id(0);
  if (gid == 0 && lid == 0) {
    *result = sizeof(reduce_output_t);
  }
}
