
#define TEST_CLI test_cli

#include "cli.hxx"

int main(int argc, const char *argv[])
{
#ifdef TEST_CLI
	return	TEST_CLI(argc, argv);
#else
	return 0;
#endif
}
