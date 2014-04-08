#include<cassert>
#include<stdexcept>
#include<map>
#include<list>
#include<string>
#include<initializer_list>

#define assert_msg(x)  x

namespace ljr {

enum yes_no_maybe{
	yes,
	no,
	maybe 
};

namespace cli {

/// Encapsulates the info needed to parse options on the command line for a particular command
/// Like an entry in a database table of all possible options of all possible commands
/// Even if the 'id' is a char between 'a' and 'Z' , that character will NOT be used
/// in the parsing algorithm.  So if you ID'd your option as 'a', still add "-a" (or even just "a") 
/// to the list of aliases!  Not all IDs will be ascii codes and having it be
/// used sometimes, but not other times  could lead to confusion.
/// if options are expected to have hyphens, then they should put the hyphen
/// into the alias list e.g.  { "-l", "--long"}
struct option {
	int                     id;             ///< Unique, may be an ASCII char code 
	std::list<std::string>  aliases;        ///< All the strings that denote this option
	std::string             description;    ///< Possibly be used in --help  
public:
	option(
		int                                     _id,
		std::initializer_list<const char*>&&    _aliases,
		std::string                             _description = ""
	) 
	: id(_id)
	, aliases()
	, description(std::move(_description)) 
	{
		for(auto s : _aliases) aliases.push_back(std::move(s));
	}
};

/** 
Describes whatever could be entered as a command and it's options that would result in calling this program 
Some programs behave differently depeding on how they are called,  so more than one primary command is possible. 

Usage: \code
command_option_descriptor ls_description = { 
		{ 'r'   , {"-r", "--recursive"} , "Recursively list directory contents" },
		{ 's'   , {"-s", "--size" }     , "Sort entries by size"}
};
\endcode
*/

enum {
	arg_is_command         = 1 << 0,
	arg_is_option          = 1 << 1,
	arg_is_option_value    = 1 << 2,
	arg_is_generic         = 1 << 3,

	opt_is_flag_style      = 1 << 8,
	opt_is_command_style   = 1 << 9,
	opt_is_long_style      = 1 << 10
};

class command_parser{
public:
	command_parser(
		std::initializer_list<option>&& cmmd_options   //< To identify options on command line
	)
	: priv_option_list()
	, priv_command_map()
	, priv_flag_map()
	, priv_longopt_map()
	, priv_arg_list()
	, priv_arg_iter()
	, priv_arg_hyphcount(0)
	, priv_tok_begin()
	, priv_tok_end()
	, priv_tok_type(arg_is_generic)
	, priv_in_opt_group(false)
	{
		for(auto o : cmmd_options) priv_option_list.push_back(std::move(o));

	 	for(auto opt = priv_option_list.begin() ; opt != priv_option_list.end(); ++opt)
		{
			for(auto alias: opt->aliases)
	      		{
	       			auto char_iter = alias.begin();
          
				int nhyphs = 0;
				while((char_iter != alias.end()) and (*char_iter == '-'))
				{
					++nhyphs;
					++char_iter; // eat the hyphs
				}
				std::string raw_alias = std::string(char_iter, alias.end());
				if((nhyphs ==0) and (raw_alias.length() > 1)) // command-style option
				{
					priv_command_map[raw_alias] = opt;
					continue;
				}
				if((nhyphs<2) and (raw_alias.length() == 1)) // flag-style option
				{
					priv_flag_map[raw_alias.front()] = opt;
					continue;
				}
				if(raw_alias.length() == 0) //< only hyphens are always checked, so ignore
				{
					continue;
				}
				priv_longopt_map[raw_alias] = opt; //< long-style option
			}
		}
	}	
	
	auto parse(int argc, const char* argv[]) -> bool
	{
		priv_arg_list.clear();
	
		auto first_arg = std::string(argv[0]);
		auto currchar = first_arg.rbegin();
		auto eoc = first_arg.rend();
		while( (currchar != eoc) and (*currchar != '/'))
		{
			++currchar;
		}	
		priv_arg_list.push_back(std::string(currchar.base(), first_arg.end()));

		for(int i =1 ; i < argc; ++i)
		{
			priv_arg_list.push_back(argv[i]);
		}

		priv_arg_iter = priv_arg_list.begin();
		// now make the current token the command name
		priv_tok_begin = priv_arg_iter->begin();
		priv_tok_end = priv_arg_iter->end();
		priv_tok_type = arg_is_command;
		priv_in_opt_group = false;
		return true;
	}

	
	/// this should throw an exception if not at an option
	

	auto option_id()const-> int { return priv_current_opt->id; } ///< undefined if 'is_option()' is false
	auto arg_type()const -> int { return priv_tok_type; }
	auto str()const -> std::string { return std::string(priv_tok_begin, priv_tok_end); }

	
private:
	auto done()const ->bool
	{
		return priv_arg_iter == priv_arg_list.end() ;
	}

	auto try_command_option() -> bool
	{
		for(auto x = priv_command_map.begin(); x != priv_command_map.end() ; ++x)
		{
			if(x->first == *priv_arg_iter) 
			{
				priv_tok_end = priv_arg_iter->end();
				priv_current_opt = x->second;
				priv_tok_type = arg_is_option | opt_is_command_style;
				priv_in_opt_group =false;
				return true;
			}
		}
		return false;
	}
	auto is_delim(const char x)->bool
	{
		switch(x){
		case '=':
		case ',':
			return true;
		default:
			return false;
		};
		// never get here, make compiler happy
		return false;
	}

	auto try_long_option() -> bool
	{
		std::string raw_arg = std::string(priv_tok_begin, priv_arg_iter->end());
		// see if this is a long opt
		for(auto x = priv_longopt_map.begin(); x != priv_longopt_map.end(); ++x)
		{
			const std::string &longopt = x->first;
			if(longopt.length() > raw_arg.length())
			{
				continue;
			}
			if(raw_arg.compare(0,longopt.length(),longopt) ==0)
			{
				if( is_delim(raw_arg[longopt.length()])
				or (raw_arg.begin()+longopt.length() == raw_arg.end())) 
				{
					priv_tok_end = priv_tok_begin+longopt.length();
					priv_tok_type = arg_is_option | opt_is_long_style;
					priv_in_opt_group = false;
					priv_current_opt = x->second;
					return true;
				}
			}

		}
		return false;
	}
public:
	auto parse() -> bool
	{
		if(done())return false;
		if(priv_tok_end == priv_arg_iter->end())
		{
			priv_in_opt_group =false;
			++priv_arg_iter;
			if(done())return false;
			priv_tok_begin = priv_arg_iter->begin();
			// we are at the beginning of a new argument
			// it could be 
			// 1) a single hyphed option
			// 2) an option group
			// 3) a command option
			// 4) a hyphed option with value
			// 5) not an option at all

			priv_arg_hyphcount = 0;
			while(*priv_tok_begin == '-')
			{
				++priv_arg_hyphcount;
				++priv_tok_begin;
			}
			priv_tok_end = priv_tok_begin;
			switch(priv_arg_hyphcount){
			case 0:
				if(! try_command_option())
				{
					priv_tok_begin = priv_arg_iter->begin();
					priv_tok_end = priv_arg_iter->end();
					priv_tok_type = arg_is_generic;
				}
				return true;
			case 1:
				if(try_long_option()) return true;

				// see if this is a group of options
				priv_in_opt_group = true;
				for(auto c = priv_tok_begin ; c != priv_arg_iter->end(); ++c)
				{
					if(priv_flag_map.find(*c) == priv_flag_map.end()) 
					{
						priv_in_opt_group = false;
						break;
					}
				}
				if(priv_in_opt_group)
				{
					priv_tok_end = priv_tok_begin + 1;
					priv_tok_type = arg_is_option | opt_is_flag_style;
					priv_current_opt = priv_flag_map[*priv_tok_begin];
					return true;
				}

				// check to see if the first char is a flag
				if(priv_flag_map.find(*priv_tok_begin) == priv_flag_map.end())
				{
					// it's not so give the user the full arg and label it unknown
					priv_tok_begin = priv_arg_iter->begin();
					priv_tok_end = priv_arg_iter->end();
					priv_tok_type = arg_is_generic;
					return true;
				}
				// first char is a flag,  but the rest is a value
				priv_tok_end = priv_tok_begin + 1;
				priv_tok_type = arg_is_option | opt_is_flag_style;
				priv_current_opt = priv_flag_map[*priv_tok_begin];
				return true;
			case 2:
				if(try_long_option()) return true;
			default:
				// give the user the full arg and label it unknown, because we don't know
				priv_tok_begin = priv_arg_iter->begin();
				priv_tok_end = priv_arg_iter->end();
				priv_tok_type = arg_is_generic;
				return true;
			};
		}
		
		// we are in middle of arg
		// so either 
		// 1) in an arg group
		// 2) in a flag assign
		// 3) in a long assign

		// check opt group
		if(priv_in_opt_group)
		{
			priv_tok_begin = priv_tok_end;
			++priv_tok_end;
			priv_current_opt = priv_flag_map[*priv_tok_begin];
			return true;
		}

		
		if(priv_tok_type & opt_is_flag_style)
		{
			if(is_delim(*priv_tok_end))
			{
				++priv_tok_end;
			}
			priv_tok_begin = priv_tok_end;
			priv_tok_end = priv_arg_iter->end();
			priv_tok_type = arg_is_option_value;
			priv_in_opt_group = false;
			return true;
		}

		if(priv_tok_type & opt_is_long_style)
		{
			assert(is_delim(*priv_tok_end));
			priv_tok_begin = priv_tok_end +1;
			priv_tok_end = priv_arg_iter->end();
			priv_tok_type = arg_is_option_value;
			priv_in_opt_group =false;
			return true;
		}
		
		throw std::invalid_argument(*priv_arg_iter);
		
		// never get here
		return true;
	}


public:
	using char_opt_map_type   = std::map<char,typename std::list<option>::iterator>; // to easily map flags to options
	using string_opt_map_type = std::map<std::string,typename std::list<option>::iterator>; // to easily map flags to options
	// option info
	std::list<option>               priv_option_list;
	string_opt_map_type             priv_command_map;
	char_opt_map_type               priv_flag_map;
	string_opt_map_type             priv_longopt_map;

	// current arg set info
	std::list<std::string>                  priv_arg_list;
	typename std::list<std::string>::iterator    priv_arg_iter;
	int                                     priv_arg_hyphcount;

	// current token info
	std::string::iterator           priv_tok_begin;
	std::string::iterator           priv_tok_end;
	int                             priv_tok_type;
	std::list<option>::iterator     priv_current_opt;
	bool                            priv_in_opt_group;
};

} //cli

} // ljr

#ifdef TEST_CLI 
#include<iostream>

void process_token(ljr::cli::command_parser& test_cli_parser)
{
		std::cout << test_cli_parser.str() << " is ";
		switch(test_cli_parser.arg_type() & 0x0f) {
		case ljr::cli::arg_is_generic:
			std::cout << "a generic argument\n";
			break;
		case ljr::cli::arg_is_option:
			std::cout << "option #" <<  test_cli_parser.option_id();

			if(   test_cli_parser.parse() 
			and ((test_cli_parser.arg_type() & 0x0f) == ljr::cli::arg_is_option_value))
			{
				std::cout << " with value '" << test_cli_parser.str() << "'\n";
			}
			else {
				std::cout << "\n";
				process_token(test_cli_parser);
			}
			break;
		default:
			throw std::logic_error("Undefined argument type case");
		};
}

int test_cli(int argc, const char *argv[])
{
	enum test_options {
		help_option_id = 'h' ,
		version_option_id = 'v',
		output_option_id = 'o',
		update_command = 1 <<8 + 1
	};

	
	ljr::cli::command_parser test_cli_parser{
		{ help_option_id,       { "-h",    "--help" },  "This option right now" },
		{ version_option_id,    { "-v", "--version" },  "Current version of program" },
		{ update_command,       { "update" },           "Update the database"},       
		{ 'D',                  { "-D", "--define" },   "--define=someval Defines a macro" },
	};

	test_cli_parser.parse(argc, argv);
	std::cout << "The primary command was " << test_cli_parser.str() << "\n";
	while(test_cli_parser.parse() )
	{
		process_token(test_cli_parser);
	}

	return 0;
}

#endif
