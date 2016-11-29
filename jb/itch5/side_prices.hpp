#ifndef jb_itch5_side_prices_hpp
#define jb_itch5_side_prices_hpp

#include <map>
#include <jb/itch5/price_field.hpp>

namespace jb {
  namespace itch5 {
    /** @todo MAX_HEAD_SIZE will be configure :: config()
     */
    #define MAX_HEAD_SIZE 10000  // to test

    typedef std::array<int, MAX_HEAD_SIZE> head_t;
    typedef head_t::iterator head_iterator_t;
    typedef std::map<price4_t, int> tail_t;
    typedef tail_t::iterator tail_iterator_t;
    
    template <typename side>
    class base_cache_prices {
      auto find(price4_t px) {
	/** 
         * @todo do the common stuff 
	 * some algorith like: copy from or to the tail,
         * redifining limits,
         * are common (maybe...)
         */

	/* ..... */

	/** 
         * @todo call object for specifics                      
	 * this is a simplification to show the interface
         */
	return static_cast<side*>(this)->do_find_tail();  
      }
      
      /**
       * @todo same for the rest of the interface                 
       * review the whole madness with Carlos before typing in 
       */
      
      auto end() { return static_cast<side*>(this)->do_end(); }
      auto begin() { return static_cast<side*>(this)->do_begin(); }
      /**
       * @todo some members will be overloaded to support both data
       * structures... (maybe..)
       */
      void erase(head_iterator_t& px_it) {
	static_cast<side*>(this)->do_erase_head(px_it); }
      void erase(tail_iterator_t& px_it) {
	static_cast<side*>(this)->do_erase_tail(px_it); }
      /* etc... */
      auto emplace(price4_t const& px, int qty) {
	return static_cast<side*>(this)->do_emplace(px, qty);
      }
      auto size() const { return static_cast<side*>(this)->do_size(); }
      auto rbegin() { return static_cast<side*>(this)->do_rbegin(); }
      auto empty() const { return static_cast<side*>(this)->do_empty(); }
    };

    /** 
     *@todo need to check if greater or less provides enough of a type 
     * to handle the differences between both sizes, if this is the case
     * it could be an extre template typename....
     */
    class cache_buy_prices : public base_cache_prices<cache_buy_prices> {
    public:
      using buy_tail_t = std::map<price4_t, int, std::greater<price4_t>>;
      using buy_tail_iterator_t =
	std::map<price4_t, int, std::greater<price4_t>>::iterator;
     
      auto do_find_tail(price4_t px) {return tail_.find(px); }
      /**
       * @todo check limits, addapt return to make return types consistent
       * etc..
       */
      auto do_find_head(price4_t px) {return head_[px.as_integer()]; }

      auto do_end() { return tail_.end(); }
      auto do_begin() { return tail_.begin(); }
      void do_erase_head(head_iterator_t& px_it) {
	*px_it = 0; }
      void do_erase_tail(buy_tail_iterator_t& px_it) {
	tail_.erase(px_it); }
      auto do_emplace(price4_t const& px, int qty) {
	return tail_.emplace(px, qty);
      }
      auto do_size() const { return tail_.size(); }
      auto do_rbegin() { return tail_.rbegin(); }
      auto do_empty() const { return tail_.empty(); }
      
    private:
      head_t head_;
      buy_tail_t tail_;
    };
    
    class cache_sell_prices : public base_cache_prices<cache_sell_prices> {
    public:
      using sell_tail_t = std::map<price4_t, int, std::less<price4_t>>;
      using sell_tail_iterator_t =
           std::map<price4_t, int, std::less<price4_t>>::iterator;
    
      auto do_find(price4_t px) {return tail_.find(px); }
      auto do_end() { return tail_.end(); }
      auto do_begin() { return tail_.begin(); }
      void do_erase_head(head_iterator_t& px_it) {
	*px_it = 0; }
      void do_erase_tail(sell_tail_iterator_t& px_it) {
	tail_.erase(px_it); }
      auto do_emplace(price4_t const& px, int qty) {
	return tail_.emplace(px, qty);
      }
      auto do_size() const { return tail_.size(); }
      auto do_rbegin() { return tail_.rbegin(); }
      auto do_empty() const { return tail_.empty(); }
            
    private:
      head_t head_;
      tail_t tail_;
    };

    // order_book type = jb::itch5::cache_price
    struct cache_price {
      typedef cache_buy_prices buys_t;
      typedef cache_sell_prices sells_t;
    };

    // order_book type = jb::itch5::map_price
    struct map_price {
      typedef std::map<price4_t, int, std::greater<price4_t>> buys_t;
      typedef std::map<price4_t, int, std::less<price4_t>> sells_t;
    };
    
  } // namespace itch
} // namespace jb

#endif // jb_itch5_side_prices_hpp
