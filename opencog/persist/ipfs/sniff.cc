/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <sstream>

#include <ipfs/client.h>

int main (int, const char **)
{
	std::stringstream contents;
	ipfs::Client client("localhost", 5001);
	client.FilesGet("/ipfs/QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG/readme", &contents);
	std::cout << contents.str() << std::endl;
	return 0;
}
