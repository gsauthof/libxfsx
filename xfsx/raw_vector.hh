#ifndef XFSX_RAW_VECTOR_HH
#define XFSX_RAW_VECTOR_HH

#include <vector>
#include <memory>

// The point of this raw vector is that integral types like char,
// unsigned char, int ... aren't value initialized when creating
// a vector of a certain size - or when resizing an existing one.
// This is advantageous e.g. when directly after a resize all the
// bytes are rewritten, anyways - because then a superfluous
// memset() call is eliminated.
//
// Think: malloc() vs. calloc() or realloc() vs.
// realloc()+memset() or new unsigned char[123] vs new unsigned
// char[123]() ...
namespace xfsx {

    // Source: https://stackoverflow.com/a/21028912/427158
    // 'Allocator adaptor that interposes construct() calls to
    // convert value initialization into default initialization.'
    template <typename T, typename A=std::allocator<T>>
	class default_init_allocator : public A {
	    typedef std::allocator_traits<A> a_t;
	    public:

            // allocator users like std::vector use this even if U == T to
            // get the right allocator
	    template <typename U> struct rebind {
		using other =
		    default_init_allocator<
		    U, typename a_t::template rebind_alloc<U>
		    >;
	    };

	    using A::A;

            // construct and default initialize
	    template <typename U>
		void construct(U* ptr)
		noexcept(std::is_nothrow_default_constructible<U>::value) {
		    ::new(static_cast<void*>(ptr)) U;
		}

            // construct and initialize when necessary
	    template <typename U, typename...Args>
		void construct(U* ptr, Args&&... args) {
		    a_t::construct(static_cast<A&>(*this),
			    ptr, std::forward<Args>(args)...);
		}
	};

    template<typename T, typename A = std::allocator<T>>
    using Raw_Vector = std::vector<T, default_init_allocator<A>>;

} // xfsx

#endif // XFSX_RAW_VECTOR_HH
