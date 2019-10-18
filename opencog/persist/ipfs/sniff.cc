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
#if 0
	client.FilesGet("/ipfs/QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG/readme", &contents);
	std::cout << contents.str() << std::endl;
#endif

	ipfs::Json version;
	client.Version(&version);
	std::cout << "Peer's version: " << version << std::endl;

	ipfs::Json add_result;
	client.FilesAdd(
		{{"foo.txt", ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"abcd\")"},},
		&add_result);
	std::cout << "FilesAdd() result:" << std::endl
			  << add_result.dump(2) << std::endl;

	return 0;
}
