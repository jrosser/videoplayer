
#ifndef __DF_BOOST_PROG_OPTS_HPP
#define __DF_BOOST_PROG_OPTS_HPP

#define BOOST_AUTO_LINK_NOMANGLE
#include <boost/program_options.hpp>

namespace fixes {
namespace program_options {
	namespace pox = boost::program_options;
	using namespace pox;
	/*
	 * handle specifying the same argument multiple times
	 */
	template<class T, class charT = char>
	class typed_value : public pox::typed_value<T, charT>
	{
	public:
		typed_value(T* store_to)
		: pox::typed_value<T, charT>(store_to)
		{}

		void xparse(boost::any& value_store,
		            const std::vector< std::basic_string<charT> >& new_tokens)
			const {
			/* Handle multiple overriding values by creating a temp
			 * value store and copying the value out at the end */
			boost::any value_store_tmp;
			pox::typed_value<T,charT>::xparse(value_store_tmp, new_tokens);
			value_store = value_store_tmp;
		}
	};

	template<class T>
	typed_value<T>*
	value(T* v)
	{
		typed_value<T>* r = new typed_value<T>(v);
		return r;
	}

	template<class T>
	typed_value<T>*
	value()
	{
		// Explicit qualification is vc6 workaround.
		return value<T>(0);
	}

	typed_value<bool>* bool_switch(bool* v, bool default_v=true);
	typed_value<bool>* bool_switch(bool default_v=true);

	/*
	 *
	 */
	class untyped_value : public pox::untyped_value
	{
	public:
		untyped_value(bool zero_tokens = false)
		: pox::untyped_value(zero_tokens)
		{}

		void xparse(boost::any& value_store,
		            const std::vector< std::string >& new_tokens)
			const {
			/* Handle multiple overriding values by creating a temp
			 * value store and copying the value out at the end */
			/* to bypass the valuestore test */
			boost::any value_store_tmp;
			pox::untyped_value::xparse(value_store_tmp, new_tokens);
			value_store = value_store_tmp;
		}
	};

	/*
	 * handle easy options builder
	 */
	class BOOST_PROGRAM_OPTIONS_DECL
	options_description_easy_init : public pox::options_description_easy_init {
	public:
		options_description_easy_init(options_description* owner)
			: pox::options_description_easy_init(owner) {}

		options_description_easy_init&
		operator()(const char* name, const char* description)
		{
			pox::options_description_easy_init::operator()(name, new untyped_value(true), description);
			return *this;
		}

		options_description_easy_init&
		operator()(const char* name, const value_semantic* s)
		{
			pox::options_description_easy_init::operator()(name, s);
			return *this;
		}

		options_description_easy_init&
		operator()(const char* name, const value_semantic* s, const char* description)
		{
			pox::options_description_easy_init::operator()(name, s, description);
			return *this;
		}

	};

	/*
	 * options_description
	 */
	class BOOST_PROGRAM_OPTIONS_DECL
	options_description : public pox::options_description {
	public:
		options_description() : pox::options_description() {}
		options_description(unsigned line_length) : pox::options_description(line_length) {}
		options_description(const std::string& caption) : pox::options_description(caption) {}
		options_description(const std::string& caption, unsigned line_length)
			: pox::options_description(caption, line_length) {}

		options_description_easy_init add_options() {
			return options_description_easy_init(this);
		};
	};

}; /* po */
}; /* fixes */

#endif
