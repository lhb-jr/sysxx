#include <syslog.h>
#include <memory>
#include <sstream>


namespace sys {
class logstream;
class syslog_gateway; // fwd impl

}

template<class String>
auto operator<<(sys::logstream& log, String&& s) -> sys::logstream&;
auto operator<<(sys::logstream& log, std::ostream& (*)(std::ostream&)) -> sys::logstream&;


namespace sys {

namespace log {

constexpr int max_entry_length = 80;

using option_mask = int;
enum { 
// If on, openlog sets up the connection so that any syslog on this connection
// writes its message to the calling process' Standard Error stream in addition
// to submitting it to Syslog. If off, syslog does not write the message to
// Standard Error.
	perror  = LOG_PERROR,

// If on, openlog sets up the connection so that a syslog on this connection
// that fails to submit a message to Syslog writes the message instead to system
// console. If off, syslog does not write to the system console (but of course
// Syslog may write messages it receives to the console).
	console = LOG_CONS,

// When on, openlog sets up the connection so that a syslog on this connection
// inserts the calling process' Process ID (PID) into the message. When off, 
// openlog does not insert the PID.
	pid     = LOG_PID,

// When on, openlog opens and connects the /dev/log socket. When off, a future
// syslog call must open and connect the socket.
	ndelay  = LOG_NDELAY
};

enum facility {
	user     = LOG_USER,
	mail     = LOG_MAIL,
	daemon   = LOG_DAEMON,
	auth     = LOG_AUTH,
	syslog   = LOG_SYSLOG,
	lpr      = LOG_LPR,
	news     = LOG_NEWS,
	uucp     = LOG_UUCP,
	cron     = LOG_CRON,
	authpriv = LOG_AUTHPRIV,
	ftp      = LOG_FTP,
	local    = LOG_LOCAL0,
	local0   = LOG_LOCAL0,
	local1   = LOG_LOCAL1,
	local2   = LOG_LOCAL2,
	local3   = LOG_LOCAL3,
	local4   = LOG_LOCAL4,
	local5   = LOG_LOCAL5,
	local6   = LOG_LOCAL6,
	local7   = LOG_LOCAL7
};

enum priority {
	error  = LOG_ERR,
	warn   = LOG_WARNING,
	notify = LOG_NOTICE,
	debug  = LOG_DEBUG
};

} // log::


class logstream {
public:
	auto options()const             -> log::option_mask;
	auto facility()const            -> log::facility;
	auto priority()const            -> log::priority;
	auto priority(log::priority p)  -> logstream&;
public:
	logstream() = delete;
	
	logstream(logstream &&other)
	: priv_priority(other.priv_priority)
	, priv_gateway(std::move(other.priv_gateway))
	, priv_strbuf(std::move(other.priv_strbuf)) {}
private:
	friend auto connect(log::priority, log::option_mask, log::facility) -> logstream;
	friend auto connect(log::priority) -> logstream;

	template<class String> 
	friend auto ::operator<<(logstream&, String&&)->logstream&;
	friend auto ::operator<<(logstream& log, std::ostream& (*)(std::ostream&)) -> logstream&;
private:
	logstream(log::priority p, std::shared_ptr<syslog_gateway> gw);
private:
	log::priority                      priv_priority;
	std::shared_ptr<syslog_gateway>    priv_gateway;
	std::unique_ptr<std::stringstream> priv_strbuf;
};

auto connect(
	log::priority,
	log::option_mask,
	log::facility
) -> logstream;


} // sys::
