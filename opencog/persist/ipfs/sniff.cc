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

	ipfs::Json add_result;
	client.FilesAdd({
		{"/opencog/atomspace-bogus/Concept-abcd.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"abcd\")\n"},
		{"/opencog/atomspace-bogus/Concept-pqrs.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"pqrs\")\n"},
		{"/opencog/atomspace-bogus/Predicate-p.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Predicate \"p\")\n"},
		},
		&add_result);
	std::cout << "FilesAdd() result:" << std::endl
			  << add_result.dump(2) << std::endl;

	std::cout << "size=" << add_result.size() << std::endl;
	std::cout << "first=" << add_result[0] << std::endl;
	std::cout << "first path=" << add_result[0]["path"] << std::endl;
	std::cout << "first hash=" << add_result[0]["hash"] << std::endl;

	std::string block = add_result[0]["hash"];

	ipfs::Json stat_result;
	client.BlockStat(block, &stat_result);
	std::cout << "Block stats: "
		<< stat_result << std::endl;

	std::stringstream block_contents;
	client.BlockGet(block, &block_contents);
	std::cout << "Block: "
		<< block_contents.str() << std::endl;

	return 0;
}
