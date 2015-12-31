#ifndef jb_tde_generic_reduce_hpp
#define jb_tde_generic_reduce_hpp

#include <jb/tde/generic_reduce_program.hpp>
#include <jb/log.hpp>
#include <jb/p2ceil.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/memory/local_buffer.hpp>

namespace jb {
namespace tde {

template<typename reducer, typename input_type_t, typename output_type_t>
class generic_reduce {
 public:
  typedef input_type_t input_type;
  typedef output_type_t output_type;

  generic_reduce(std::size_t size, boost::compute::command_queue const& queue)
      : size_(size)
      , queue_(queue)
      , program_(reducer::create_program(queue))
      , initial_(program_, "generic_transform_reduce_initial")
      , intermediate_(program_, "generic_transform_reduce_intermediate")
  {
    // ... size the first pass of the reduction.  We need to balance two
    // constraints:
    //   (a) We cannot use more memory than whatever the device
    //   supports, since the algorithm uses one entry in scratch per
    //   local thread that could limit the number of local threads.
    //   This is extremely unlikely, but who wants crashes?
    //   (b) The maximum size of a workgroup limits by how much we can
    //   reduce in a single pass, so smaller workgroups require more
    //   passes and more intermediate memory.
    // first we query the device ...
    boost::compute::device device = queue_.get_device();
    std::size_t local_mem_size = device.local_memory_size();
    max_workgroup_size_ = device.max_work_group_size();

    // ... the we convert the bytes into array sizes ...
    boost::compute::kernel sizer(program_, "scratch_element_size");
    boost::compute::vector<boost::compute::ulong_> dev(1, queue_.get_context());
    sizer.set_arg(0, dev);
    queue_.enqueue_1d_range_kernel(sizer, 0, 1, 1).wait();
    std::vector<boost::compute::ulong_> host(1);
    boost::compute::copy(dev.begin(), dev.end(), host.begin(), queue_);
    sizeof_output_type_ = host[0];
    scratch_size_ = local_mem_size / sizeof_output_type_;
    // ... this is the largest amount that we might need from local
    // scratch ...
    scratch_size_ = std::min(scratch_size_, max_workgroup_size_);

    // ... now on to compute the reduction factor, first how many
    // threads can we effectively use, unless local scratch is tiny,
    // almost all of the time this would be max_workgroup_size ...
    effective_workgroup_size_ = std::min(scratch_size_, max_workgroup_size_);

    // ... so in a single pass we will need this many workgroups to
    // handle all the data ...
    auto workgroups = std::max(
        std::size_t(1), size_ / effective_workgroup_size_);

    JB_LOG(trace)
        << "Reducer plan settings:"
        << "\n  size_ = " << size_
        << "\n  sizeof(output)=" << sizeof_output_type_
        << "\n  scratch_size=" << scratch_size_
        << "\n  local_mem_size=" << local_mem_size
        << "\n  max_workgroup_size=" << max_workgroup_size_
        << "\n  effective_workgroup_size=" << effective_workgroup_size_
        << "\n  workgroups=" << workgroups
        << "\n  workgroups * effective_workgroup_size="
        << workgroups * effective_workgroup_size_;

    // ... a lot of that silly math was to size the output buffer ...
    ping_ = boost::compute::vector<output_type>(
        workgroups, queue_.get_context());
    pong_ = boost::compute::vector<output_type>(
        workgroups, queue_.get_context());
  }

  typedef typename boost::compute::vector<input_type>::iterator vector_iterator;
  boost::compute::future<vector_iterator> execute(
      std::vector<input_type> const& orig,
      boost::compute::vector<input_type> const& src,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    if (src.size() != size_) {
      throw std::invalid_argument("mismatched size");
    }
    
    auto workgroup_size = effective_workgroup_size_;
    auto workgroups = size_ / workgroup_size;
    if (workgroups == 0) {
      workgroups = 1;
    }
    auto div = std::div(
        static_cast<long long>(size_),
        static_cast<long long>(workgroups * workgroup_size));
    auto VPT = div.quot + (div.rem != 0);

    JB_LOG(trace)
        << "Executing (initial) reducer plan for :"
        << "\n    size_ = " << size_
        << "\n    sizeof(output)=" << sizeof_output_type_
        << "\n    scratch_size=" << scratch_size_
        << "\n    max_workgroup_size=" << max_workgroup_size_
        << "\n    effective_workgroup_size=" << effective_workgroup_size_
        << "\n    workgroups=" << workgroups
        << "\n    workgroup_size=" << workgroup_size
        << "\n    worgroups*workgroup_size="
        << workgroups * workgroup_size
        << "\n    arg.VPT=" << VPT
        << "\n    arg.TPB=" << workgroup_size
        << "\n    arg.N=" << size_
        ;

    int arg = 0;
    initial_.set_arg(arg++, ping_);
    initial_.set_arg(arg++, boost::compute::ulong_(VPT));
    initial_.set_arg(arg++, boost::compute::ulong_(workgroup_size));
    initial_.set_arg(arg++, boost::compute::ulong_(size_));
    initial_.set_arg(arg++, src);
    initial_.set_arg(
        arg++, boost::compute::local_buffer<output_type>(workgroup_size));
    auto event = queue_.enqueue_1d_range_kernel(
        initial_, 0, workgroups * workgroup_size, workgroup_size, wait);

    for (auto pass_output_size = workgroups;
         pass_output_size > 1;
         pass_output_size = workgroups) {
      // ... it is possible (specially towards the end) that we do not
      // have enough work to fill a "workgroup_size" number of
      // local work items, in that case, just limit the local size even
      // further ...
      if (pass_output_size < workgroup_size) {
        workgroup_size = jb::p2ceil(pass_output_size) / 2;
      }
      workgroups = pass_output_size / workgroup_size;
      if (workgroups == 0) {
        workgroups = 1;
      }
      auto div = std::div(
          static_cast<long long>(pass_output_size),
          static_cast<long long>(workgroups * workgroup_size));
      auto VPT = div.quot + (div.rem != 0);

      JB_LOG(trace)
          << "  executing (intermediate) reducer plan with :"
          << "\n    size_ = " << size_
          << "\n    sizeof(output)=" << sizeof_output_type_
          << "\n    scratch_size=" << scratch_size_
          << "\n    max_workgroup_size=" << max_workgroup_size_
          << "\n    effective_workgroup_size=" << effective_workgroup_size_
          << "\n    workgroups=" << workgroups
          << "\n    workgroup_size=" << workgroup_size
          << "\n    worgroups*workgroup_size=" << workgroups * workgroup_size
          << "\n    pass_output_size=" << pass_output_size
          << "\n    arg.VPT=" << VPT
          << "\n    arg.TPB=" << workgroup_size
          << "\n    arg.N=" << pass_output_size
          ;

      // ... prepare the kernel ...
      int arg = 0;
      intermediate_.set_arg(arg++, pong_);
      intermediate_.set_arg(arg++, boost::compute::ulong_(VPT));
      intermediate_.set_arg(arg++, boost::compute::ulong_(workgroup_size) /* TPB */);
      intermediate_.set_arg(arg++, boost::compute::ulong_(pass_output_size));
      intermediate_.set_arg(arg++, ping_);
      intermediate_.set_arg(
          arg++, boost::compute::local_buffer<output_type>(workgroup_size));
      event = queue_.enqueue_1d_range_kernel(
          intermediate_, 0, workgroups * workgroup_size, workgroup_size,
          boost::compute::wait_list(event));

      std::swap(ping_, pong_);
    }
    return boost::compute::make_future(ping_.begin(), event);
  }

 private:
  std::size_t size_;
  boost::compute::command_queue queue_;
  boost::compute::program program_;
  boost::compute::kernel initial_;
  boost::compute::kernel intermediate_;
  std::size_t max_workgroup_size_;
  std::size_t sizeof_output_type_;
  std::size_t scratch_size_;
  std::size_t effective_workgroup_size_;
  boost::compute::vector<output_type> ping_;
  boost::compute::vector<output_type> pong_;
};

} // namespace tde
} // namespace jb

#endif // jb_tde_generic_reduce_hpp
