/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <sstream>

#include <ipfs/client.h>

#define DEBUG 1

std::string addNode(ipfs::Client& client,
                    const std::string& type,
                    const std::string& node_text)
{
	std::string name = type + " \"" + node_text + "\"";
	std::string text = "(" + name + ")\n";
	ipfs::Json add_result;
	client.FilesAdd({
		{name,
			ipfs::http::FileUpload::Type::kFileContents,
			text,
		},},
		&add_result);
	std::string id = add_result[0]["hash"];

#ifdef DEBUG
	std::cout << "addNode: " << id << std::endl
			  << "Result: " << add_result.dump(2) << "\n" << std::endl;
#endif
	return id;
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

	std::string clone_id;
	client.ObjectPatchSetData(cona,
		{"foobar", ipfs::http::FileUpload::Type::kFileContents,
			"(Concept \"abcd\" (stv 0.5 0.5))\n"},
		&clone_id);

	ipfs::Json object;
	client.ObjectGet(cona, &object);
	std::cout << "Concept-A Object: " << std::endl << object.dump(2) << std::endl;

	std::cout << "concept-A-STV: " << clone_id << std::endl;
	client.ObjectGet(clone_id, &object);
	std::cout << "Concept-A-STV Object: " << std::endl << object.dump(2) << std::endl;

	std::string stuff;
#if 0
	// The "dropped text.txt" file
	// Data looks like the file contents, but there's no filename...
	stuff = "QmTRQkD8RKpFzywY5phzPhtWsM7HPsir4uSNGwiWmgwzaU";
	client.ObjectGet(stuff, &object);
	std::cout << "file-thing: " << std::endl << object.dump(2) << std::endl;
#endif

	// My ipfs resolved peerid (seems to be empty)
	stuff = "QmUNLLsPACCz1vLxQVkXqqLX5R1X345qqfHbsf67hvA3Nn";
	stuff = "QmNzxAxnJYor2G4kXsf4LF9sqsQmMAZkKXAitVqdC1Mp81";
	client.ObjectGet(stuff, &object);
	std::cout << "resolved peerid 2file: " << std::endl << object.dump(2) << std::endl;

	stuff = "QmQhqkiysCeD698SVGHvVX2scNSMZnQLrEh3T4txvMqYDU";
	client.ObjectGet(stuff, &object);
	std::cout << "resolved peerid 3file: " << std::endl << object.dump(2) << std::endl;

#if 0
	// My ipns peerid ... this hangs.
	stuff = "QmbJ5UzreC86qtHrWC2SwWKLsTiLqTuG4cqHHJVdYPK6s9";
	client.ObjectGet(stuff, &object);
	std::cout << "ipns peer: " << std::endl << object.dump(2) << std::endl;
#endif

	stuff = "QmPXahdEAs7nGhTAPmct458LEWtEsWRGaX1xXBURdJu1ir";
	client.ObjectGet(stuff, &object);
	std::cout << "ipns keeyed peer: " << std::endl << object.dump(2) << std::endl;

#if 0
	// given some object id Qmobject...
	//    `ipfs name publish Qmobject...`
	// should return the peer ID, that is, /ipfs/Qmpeerid...
	// Try it.
	//    `ipfs name publish QmRrVRGx5xAXX52BYuScmJk1KWPny86BtexP8YNJ8jz76U`
	//    Error: cannot manually publish while IPNS is mounted
	//    `fusermount -u /ipns`
	//    Published to Qm..s9: /ipfs/Qm..6U
	// well, but we already knew that, we were already published.
	//    `ipfs name resolve Qmpeerid...`
	//    `ipfs name resolve QmbJ5UzreC86qtHrWC2SwWKLsTiLqTuG4cqHHJVdYPK6s9`
	// Now this gives Qm..6U as the resolution!
	// The default peerid:
	//    `ipfs name resolve`

	// For me: `ipfs name resolve` gives
	//    /ipfs/QmUNLLsPACCz1vLxQVkXqqLX5R1X345qqfHbsf67hvA3Nn
	// and ls -la of above works.
	// But after the `ipfs name publish Qm..6U` the resolve is now Qm..6U
	// ... and ipfs mount crashes:
	//     Error: root can't be a file (unixfs type: File)
	//
	// My peerid is QmbJ5UzreC86qtHrWC2SwWKLsTiLqTuG4cqHHJVdYPK6s9
	// and `cd /ipns/QmbJ5UzreC86qtHrWC2SwWKLsTiLqTuG4cqHHJVdYPK6s9`
	// works and I can create files there.
	//
	// Ahh, OK. So afer every file edit on /ipns/Qm..s9 I then have to
	// `ipfs name resolve` to get the newest actual CID and then the
	// `ls -la /ipfs/CID` works and shows the latest.
	//
	// `ipfs key gen --type=rsa --size=2048 mykey`
	// `ipfs key list`
	// ipfs name publish --key=mykey QmRrVRGx5xAXX52BYuScmJk1KWPny86BtexP8YNJ8jz76U
	// Published to QmPXahdEAs7nGhTAPmct458LEWtEsWRGaX1xXBURdJu1ir
	// ipfs name resolve QmPXahdEAs7nGhTAPmct458LEWtEsWRGaX1xXBURdJu1ir
	// returns the resolved name Qm..6U
	//
	// ipfs.name.publish(addr, ...)
#endif

#if 0
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

#endif
	return 0;
}
