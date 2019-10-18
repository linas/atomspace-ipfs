/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <sstream>

#include <ipfs/client.h>

std::string addNode(ipfs::Client& client, const std::string& scm_text)
{
	ipfs::Json add_result;
	client.FilesAdd({
		{"Node.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			scm_text,
		},},
		&add_result);
	std::cout << "addNode: " << scm_text
			  << "Result: " << add_result[0] << "\n" << std::endl;
	return add_result[0]["hash"];
}

std::string addLink(ipfs::Client& client,
                    const std::string& type,
                    const std::vector<std::string>& outgoing)
{
	ipfs::Json add_result;
	client.FilesAdd({
		{"Link.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			type, // XXX wrong ...
		},},
		&add_result);
	std::cout << "addLink: " << type
			  << "Result: " << add_result[0] << "\n" << std::endl;
	return add_result[0]["hash"];
}

int main (int, const char **)
{
	std::stringstream contents;
	ipfs::Client client("localhost", 5001);

	std::string cona = addNode(client, "(Concept \"abcd\")\n");
	std::string conp = addNode(client, "(Concept \"pqrs\")\n");
	std::string pred = addNode(client, "(Predicate \"p\")\n");

	std::vector<std::string> oset;
	oset.push_back(cona);
	oset.push_back(conp);
	std::string list = addLink(client, "List", oset);

	std::string data;
	client.ObjectData(cona, &data);
	std::cout << "Concept A data: " << data << std::endl;

	client.ObjectData(conp, &data);
	std::cout << "Concept P data: " << data << std::endl;

	client.ObjectData(list, &data);
	std::cout << "List data: " << data << std::endl;

	ipfs::Json object;
	client.ObjectGet(cona, &object);
	std::cout << "Concept-A Object: " << std::endl << object.dump(2) << std::endl;

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

/*
				"Consequence": {
						"ocv-key": "QmWZdJcytbk2B7QdC9aa5M2nbSx6Nke1rTp2c4bFvdrD2S",
						"ocv-Float": [ 1, 2, 3, 4]
					}
*/

	ipfs::Json object_to_store = R"(
		{
			"Data": {
				"Truth": "42"
			},
			"Links": [ {
				"Name": "some link",
				"Hash": "QmRrVRGx5xAXX52BYuScmJk1KWPny86BtexP8YNJ8jz76U",
				"Size": 8
			} ]
		}
	)"_json;

	ipfs::Json object_stored;
	client.ObjectPut(object_to_store, &object_stored);
	std::cout << "Object to store:" << std::endl
	          << object_to_store.dump(2) << std::endl;
	std::cout << "Stored object:" << std::endl
	          << object_stored.dump(2) << std::endl;

	return 0;
}
