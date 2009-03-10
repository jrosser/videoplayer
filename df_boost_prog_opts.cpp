
#define BOOST_AUTO_LINK_NOMANGLE
#include <boost/program_options.hpp>
#include "df_boost_prog_opts.hpp"

namespace fixes {
namespace program_options {

typed_value<bool>*
bool_switch(bool* v, bool default_v)
{
	typed_value<bool>* r = new typed_value<bool>(v);
#if BOOST_VERSION >= 103500
	r->implicit_value(default_v);
#endif
	r->default_value(!default_v);
	r->zero_tokens();
	return r;
}

typed_value<bool>*
bool_switch(bool default_v)
{
	return bool_switch(0, default_v);
}

}; // program_options
}; // fixes
