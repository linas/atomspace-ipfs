/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <sstream>

#include <ipfs/client.h>

std::string addNode(ipfs::Client& client,
                    const std::string& type,
                    const std::string& node_text)
{
	std::string text = "(" + type + " \"" + node_text + "\")\n";
	ipfs::Json add_result;
	client.FilesAdd({
		{"Node.scm",
			ipfs::http::FileUpload::Type::kFileContents,
			text,
		},},
		&add_result);
	std::cout << "addNode: " << type << " " << node_text
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

	std::string id = add_result[0]["hash"];

	// Not terribly efficient, but what else?
	int i=0;
	for (const std::string& out: outgoing)
	{
		// Strange -- using a slash in the label causes a fault.
		std::string label = "out-" + std::to_string(i);
		std::string nid;
		client.ObjectPatchAddLink(id, label, out, &nid);
		id = nid;
		i++;
	}

#define DEBUG 1
#ifdef DEBUG
	ipfs::Json object;
	client.ObjectGet(id, &object);
	std::cout << "addLink: " << id << std::endl
	          << object.dump(2) << "\n" << std::endl;
#endif
	return id;
}

int main (int, const char **)
{
	std::stringstream contents;
	ipfs::Client client("localhost", 5001);

	std::string cona = addNode(client, "Concept", "abcd");
	std::string conp = addNode(client, "Concept", "pqrs");
	std::string pred = addNode(client, "Predicate", "p");

	std::vector<std::string> oset;
	oset.push_back(cona);
	oset.push_back(conp);
	std::string list = addLink(client, "List", oset);

	std::vector<std::string> eset;
	eset.push_back(pred);
	eset.push_back(list);
	std::string eval = addLink(client, "Evaluation", eset);

	ipfs::Json object;
	client.ObjectGet(eval, &object);
	std::cout << "Eval Object: " << std::endl << object.dump(2) << std::endl;

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
