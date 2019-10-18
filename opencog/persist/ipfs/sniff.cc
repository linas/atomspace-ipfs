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
	std::cout << "addAtom: " << scm_text
			  << "Result: " << add_result[0] << "\n" << std::endl;
	return add_result[0]["hash"];
}

int main (int, const char **)
{
	std::stringstream contents;
	ipfs::Client client("localhost", 5001);

	std::string cona = addAtom(client, "(Concept \"abcd\")\n");
	std::string conp = addAtom(client, "(Concept \"pqrs\")\n");
	std::string list = addAtom(client, "(List {" + cona + ", " + conp + "})\n");
	std::string pred = addAtom(client, "(Predicate \"p\")\n");

	std::string data;
	client.ObjectData(cona, &data);
	std::cout << "Concept A data: " << data << std::endl;

	client.ObjectData(conp, &data);
	std::cout << "Concept P data: " << data << std::endl;

	client.ObjectData(list, &data);
	std::cout << "List data: " << data << std::endl;

	std::string new_id;
	client.ObjectPatchAddLink(cona, "incoming", list, &new_id);
	std::cout << "new concept-A: " << new_id << std::endl;

	ipfs::Json object;
	client.ObjectGet(cona, &object);
	std::cout << "Concept-A Object: " << std::endl << object.dump(2) << std::endl;

	client.ObjectGet(new_id, &object);
	std::cout << "New Concept-A Object: " << std::endl << object.dump(2) << std::endl;

	std::string clone_id;
	client.ObjectPatchSetData(cona,
		{"foobar", ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"abcd\" (stv 0.5 0.5))\n"},
		&clone_id);

	client.ObjectGet(cona, &object);
	std::cout << "Concept-A Object: " << std::endl << object.dump(2) << std::endl;

	std::cout << "concept-A-STV: " << clone_id << std::endl;
	client.ObjectGet(clone_id, &object);
	std::cout << "Concept-A-STV Object: " << std::endl << object.dump(2) << std::endl;

	return 0;
}
