
#ifdef TEST_CLI
#include "cli.hxx"
#endif

#ifdef TEST_LOG
#include "log.hxx"
int test_log(int argc, const char *argv[]);
#endif

int main(int argc, const char *argv[])
{
#ifdef TEST_CLI
	return	TEST_CLI(argc, argv);
#elif defined TEST_LOG
	return TEST_LOG(argc,argv);
#else
	return 0;
#endif
}
