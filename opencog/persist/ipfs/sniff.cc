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

std::string cona =
({
	ipfs::Json add_result;
	client.FilesAdd({
		{"Concept.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"abcd\")\n"},
		},
		&add_result);
	std::cout << "FilesAdd(abcd) result:" << std::endl
			  << add_result.dump(2) << std::endl;
	add_result[0]["hash"];
});
	std::string conp =
({
	ipfs::Json add_result;
	client.FilesAdd({
		{"Concept.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"pqrs\")\n"},
		},
		&add_result);
	std::cout << "FilesAdd(pqrs) result:" << std::endl
			  << add_result.dump(2) << std::endl;
	add_result[0]["hash"];
});
{
	ipfs::Json add_result;
	client.FilesAdd({
		{"Predicate.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			"(Predicate \"p\")\n"},
		},
		&add_result);
	std::cout << "FilesAdd(pred) result:" << std::endl
			  << add_result.dump(2) << std::endl;
	std::string pred = add_result[0]["hash"];
}

	std::string data;
	client.ObjectData(cona, &data);
	std::cout << "Concept A data: " << data << std::endl;

	client.ObjectData(conp, &data);
	std::cout << "Concept P data: " << data << std::endl;

	ipfs::Json object;
	client.ObjectGet(cona, &object);
	std::cout << "Concep-A Object: " << std::endl << object.dump(2) << std::endl;

	return 0;
}
