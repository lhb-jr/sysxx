#include <syslog.h>
#include <memory>
#include <sstream>
#include <fstream>
#include <system_error>
#include <iostream>
#include <cctype>
#include <cstring>
#include <mutex>
#include "log.hxx"

namespace {
std::recursive_mutex local_gw_mutex;
std::weak_ptr<sys::syslog_gateway> local_gateway;
}


namespace sys {

struct syslog_gateway {
	log::facility    facility;
	log::option_mask options;
	char*             logname;
public:
	syslog_gateway(
		log::option_mask _options,
		log::facility    _facility
	)
	: options(_options)
	, facility(_facility)
	{
		std::ifstream comm("/proc/self/comm");
		std::string rawprocess_name;
		std::getline(comm,rawprocess_name);
		// cleansing the string prob isn't necessary but
		// why not, since it's only done once.
		std::stringbuf  buf("");
		for(auto x: rawprocess_name)
		{
			if(std::isprint(x))
			{
				buf.sputc(x);
			}
		}
		logname = new char[buf.str().size() +1];
		logname = std::strcpy(logname, buf.str().c_str());
		openlog(logname, options ,facility);
	}
	~syslog_gateway()
	{ 
		closelog();
		delete logname;
	}
};

}
	
sys::logstream::logstream(
	sys::log::priority              p,
	std::shared_ptr<syslog_gateway> gw
)
: priv_priority(p)
, priv_gateway(gw)
, priv_strbuf(new std::stringstream(""))
{ }

auto sys::logstream::options()const -> sys::log::option_mask      
{ 
	return priv_gateway->options; 
}

auto sys::logstream::facility()const -> sys::log::facility            
{
	return priv_gateway->facility;
}

auto sys::logstream::priority()const -> sys::log::priority
{
	return priv_priority;
}

auto sys::logstream::priority(sys::log::priority p) -> sys::logstream&
{
	priv_priority = p;
	return *this;
}

auto sys::connect(
	sys::log::priority      priority,
	sys::log::option_mask   options,
	sys::log::facility      facility
) -> sys::logstream
{
	std::lock_guard<std::recursive_mutex> gwguard(local_gw_mutex);
	if(local_gateway.expired())
	{
		auto gwptr = std::make_shared<sys::syslog_gateway>(options, facility);
		local_gateway = gwptr;
		return sys::logstream(priority, gwptr);
	}
	throw std::system_error((int)std::errc::already_connected, std::system_category()); 
}

auto sys::connect(
	sys::log::priority priority
) -> sys::logstream
{
	std::lock_guard<std::recursive_mutex> gwguard(local_gw_mutex);
	if(local_gateway.expired())
	{
		return sys::connect(priority, sys::log::pid, sys::log::user);
	}
	return sys::logstream(priority, std::shared_ptr<syslog_gateway>(local_gateway));
}


template<class Stringable>
auto operator<<(sys::logstream& log, Stringable&& s) -> sys::logstream&
{
	*(log.priv_strbuf) << s;
	
	return log;
}
auto operator<<(sys::logstream& log, std::ostream& (*endl)(std::ostream&)) -> sys::logstream&
{
	std::stringstream output;
	auto tmpstr = log.priv_strbuf->str();
	log.priv_strbuf->str("");
	for(auto c : tmpstr)
	{
		if(c == '%') // don't want any formatting going on
		{
			output.put('_');
		}
		else
		{
			output.put(c);
		}
	}
	char buf[sys::log::max_entry_length +1];
	output.getline(buf, sys::log::max_entry_length);
	syslog(log.priority(), buf);
	return log;
}

#ifdef TEST_LOG

int test_log(int argc, const char *argv[])
{
	sys::log::priority mypri = sys::log::warn;
	auto warnlog = sys::connect(sys::log::warn, sys::log::perror, sys::log::user);
	warnlog << "this is only at test" << std::endl;

	
	auto errlog = sys::connect(sys::log::error);
	errlog << "Thi iiiiiiiii iiiiiiiiii iiiiiiiiiiiiiii iiiiiiiiiiiiiiiiiii iiiiiiiiiiii %d %g %x vvvvvvv   ffffffff  s is a big time error or something!" << std::endl;
	errlog << "Thi iiiiiiiii iiiiiiiiii iiiiiiiiiiiiiii iiiiiiiiiiiiiiiiiii iiiiiiiiiiii %d %g %x vvvvvvv   ffffffff  s is a big time error or something!" << std::endl;
	return 0;
}

#endif
