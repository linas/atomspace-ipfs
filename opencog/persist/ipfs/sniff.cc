/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <sstream>

#include <ipfs/client.h>

std::string addAtom(ipfs::Client& client, const std::string& scm_text)
{
	ipfs::Json add_result;
	client.FilesAdd({
		{"Atom.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			scm_text,
		},},
		&add_result);
	std::cout << "addAtom result:" << std::endl
			  << add_result.dump(2) << std::endl;
	return add_result[0]["hash"];
}

int main (int, const char **)
{
	std::stringstream contents;
	ipfs::Client client("localhost", 5001);

	std::string cona = addAtom(client, "(Concept \"abcd\")\n");
	std::string conp = addAtom(client, "(Concept \"pqrs\")\n");
	std::string pred = addAtom(client, "(Predicate \"p\")\n");

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
