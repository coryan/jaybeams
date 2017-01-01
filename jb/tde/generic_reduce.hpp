#ifndef jb_tde_generic_reduce_hpp
#define jb_tde_generic_reduce_hpp

#include <jb/tde/generic_reduce_program.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>
#include <jb/p2ceil.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/memory/local_buffer.hpp>

namespace jb {
namespace tde {

/**
 * Implement a generic reducer for OpenCL.
 *
 * Aggregating all the values in a vector to a single value, also
 * known as reductions, is a common building block in parallel
 * algorithms.  All the reductions follow a common form, this template
 * class implements a generic reduction given the aggregation function
 * and its input / output types.
 *
 * This implementation uses a parallel reduction, for a general
 * motivation and description please see:
 *   http://developer.amd.com/resources/articles-whitepapers/opencl-optimization-case-study-simple-reductions/
 *
 * TODO(coryan) this class is work in progress, it is not fully implemented
 *
 * @tparam reducer a class derived from generic_reduce<reducer,...>.
 * Please see jb::tde::reducer_concept for details.
 * @tparam input_type_t the host type that represents the input
 * @tparam output_type_t the host type that represents the output
 */
template <typename reducer, typename input_type_t, typename output_type_t>
class generic_reduce {
public:
  //@{
  /**
   * @name Type traits
   */
  /// The host type used to represent the input into the reduction
  using input_type = input_type_t;

  /// The host type representing the output of the reduction
  using output_type = output_type_t;

  /**
   * The type of the vector used to store the results.
   *
   * The final output is a single element, but OpenCL makes it easier
   * to treat that as a result of a vector with a single element.
   */
  using vector_iterator = typename boost::compute::vector<input_type>::iterator;
  //@}

  /**
   * Constructor.  Initialize a generic reduce for a given size and
   * device queue.
   *
   * @param size the size of the input array
   * @param queue a command queue to communicate with a single OpenCL device
   */
  generic_reduce(std::size_t size, boost::compute::command_queue const& queue)
      : size_(size)
      , queue_(queue)
      , program_(create_program(queue))
      , initial_(program_, "generic_transform_reduce_initial")
      , intermediate_(program_, "generic_transform_reduce_intermediate") {
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
    // threads can we effectively use, nless local scratch is tiny,
    // almost all of the time this would be max_workgroup_size ...
    effective_workgroup_size_ = std::min(scratch_size_, max_workgroup_size_);

    // ... so in a single pass we will need this many workgroups to
    // handle all the data ...
    auto workgroups =
        std::max(std::size_t(1), size_ / effective_workgroup_size_);

    // ... a lot of that silly math was to size the output buffer ...
    ping_ =
        boost::compute::vector<output_type>(workgroups, queue_.get_context());
    pong_ =
        boost::compute::vector<output_type>(workgroups, queue_.get_context());
  }

  /**
   * Schedule the execution of a reduction.
   *
   * The algorithm works in phases, each phase runs in the OpenCL
   * device, reducing the input to a (typically much smaller) vector,
   * which is stored in either the ping_ or pong_ variable.
   *
   * If necessary the algorithm schedules multiple repeated phases,
   * asynchronously (but waiting for each other), until the output has
   * been reduced to a vector with a single element.
   *
   * @param src the device vector that will be reduced, the code
   * assumes that the vector is initialized by the time all the events
   * in @a wait have completed.
   * @param wait a list of events to wait for before any work starts
   * on the device.
   *
   * @returns a boost::compute::future<>, when ready, the future
   * contains an iterator to the beginning of a vector with a single
   * element.
   */
  boost::compute::future<vector_iterator> execute(
      boost::compute::vector<input_type> const& src,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    if (src.size() != size_) {
      throw std::invalid_argument("mismatched size");
    }

    // First initialize how much work we can do in each workgroup ...
    auto workgroup_size = effective_workgroup_size_;
    // ... that determines how manny workgroups we can work on ...
    auto workgroups = size_ / workgroup_size;
    if (workgroups == 0) {
      workgroups = 1;
    }
    // ... with those values we can compute how many values will each
    // thread need to aggregate ...
    auto div = std::div(
        static_cast<long long>(size_),
        static_cast<long long>(workgroups * workgroup_size));
    // ... that is "values-per-thread" ...
    auto VPT = div.quot + (div.rem != 0);

    // ... fill up the arguments to call the "initial_" program which
    // we prepared in the constructor ..
    int arg = 0;
    initial_.set_arg(arg++, ping_);
    initial_.set_arg(arg++, boost::compute::ulong_(VPT));
    initial_.set_arg(arg++, boost::compute::ulong_(workgroup_size));
    initial_.set_arg(arg++, boost::compute::ulong_(size_));
    initial_.set_arg(arg++, src);
    initial_.set_arg(
        arg++, boost::compute::local_buffer<output_type>(workgroup_size));
    // ... schedule that program to start ...
    auto event = queue_.enqueue_1d_range_kernel(
        initial_, 0, workgroups * workgroup_size, workgroup_size, wait);

    // ... now, if there output from the initial (or any future) phase
    // contains more than one element we need to schedule another
    // phase ...
    for (auto pass_output_size = workgroups; pass_output_size > 1;
         pass_output_size = workgroups) {
      // ... it is possible (specially towards the end) that we do not
      // have enough work to fill a "workgroup_size" number of
      // local work items, in that case, just limit the local size even
      // further ...
      if (pass_output_size < workgroup_size) {
        // ... the workgroup size should always be a power of two,
        // notice that this number will always be smaller than
        // pass_output_size because p2ceil() returns the *smallest*
        // power of two greather or equal than pass_output_size.
        // Therefore p2ceil()/2 must be strictly smaller.  Also notice
        // that p2ceil() must be at least 2 because pass_output_size
        // is greater than 1 ...
        workgroup_size = jb::p2ceil(pass_output_size) / 2;
      }
      workgroups = pass_output_size / workgroup_size;
      // ... so we have set workgroup_size to be smaller than
      // pass_output_size, therefore this is true:
      JB_ASSERT_THROW(workgroups > 0);

      // ... once more, compute the "values per thread" argument ...
      auto div = std::div(
          static_cast<long long>(pass_output_size),
          static_cast<long long>(workgroups * workgroup_size));
      auto VPT = div.quot + (div.rem != 0);

      // ... prepare the program to run ...
      int arg = 0;
      intermediate_.set_arg(arg++, pong_);
      intermediate_.set_arg(arg++, boost::compute::ulong_(VPT));
      intermediate_.set_arg(
          arg++, boost::compute::ulong_(workgroup_size) /* TPB */);
      intermediate_.set_arg(arg++, boost::compute::ulong_(pass_output_size));
      intermediate_.set_arg(arg++, ping_);
      intermediate_.set_arg(
          arg++, boost::compute::local_buffer<output_type>(workgroup_size));
      // ... schedule the "intermediate_" program to reduce the data
      // from ping_ into pong_ ...
      event = queue_.enqueue_1d_range_kernel(
          intermediate_, 0, workgroups * workgroup_size, workgroup_size,
          boost::compute::wait_list(event));

      // ... this does not swap anything in device memory, we just
      // swap our pointers in the host, so the next iteration of the
      // loop (or if we exit here) has the output in ping_ ...
      std::swap(ping_, pong_);
    }
    // ... return the results ...
    return boost::compute::make_future(ping_.begin(), event);
  }

  static boost::compute::program
  create_program(boost::compute::command_queue const& queue) {
    std::ostringstream os;
    os << "typedef " << boost::compute::type_name<input_type_t>()
       << " reduce_input_t;\n";
    os << "typedef " << boost::compute::type_name<output_type_t>()
       << " reduce_output_t;\n";
    os << "inline void reduce_initialize(reduce_output_t* lhs) {\n";
    os << "  " << reducer::initialize_body("lhs") << "\n";
    os << "}\n";
    os << "inline void reduce_transform(\n";
    os << "    reduce_output_t* lhs, reduce_input_t const* value,\n";
    os << "    unsigned long offset) {\n";
    os << "  " << reducer::transform_body("lhs", "value", "offset") << "\n";
    os << "}\n";
    os << "inline void reduce_combine(\n";
    os << "    reduce_output_t* accumulated, reduce_output_t* value) {\n";
    os << "  " << reducer::combine_body("accumulated", "value") << "\n";
    os << "}\n";
    os << "\n";
    os << generic_reduce_program_source;
    auto program = boost::compute::program::create_with_source(
        os.str(), queue.get_context());
    try {
      program.build();
    } catch (boost::compute::opencl_error const& ex) {
      JB_LOG(error) << "errors building program: " << ex.what() << "\n"
                    << program.build_log() << "\n";
      JB_LOG(error) << "Program body\n================\n"
                    << os.str() << "\n================\n";
      throw;
    }
    return program;
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
