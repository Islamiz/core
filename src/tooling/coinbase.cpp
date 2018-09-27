#include "messages.pb.h"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "common/types.hpp"
#include "messages/Message.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace neuro {

int main(int argc, char *argv[]) {

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Produce help message.")
    ("filepath,f", po::value<std::string>()->default_value("key.pub"), "File path for keys (appending .pub or .priv)")
    ("ncc,n", po::value<uint64_t>()->default_value(1000), "How many ncc you want")
    ("fees,e", po::value<uint64_t>()->default_value(1), "How many ncc you want")
    ("type,t", po::value<std::string>()->default_value("json"), "enum [json, bson, protobuf]")
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  const auto filepath = vm["filepath"].as<std::string>();
  const auto type = vm["type"].as<std::string>();
  const auto ncc = vm["ncc"].as<uint64_t>();
  const auto fees = vm["fees"].as<uint64_t>();
  
  crypto::EccPub ecc_pub(filepath);
  Buffer key_pub;
  ecc_pub.save(&key_pub);
  Buffer address = crypto::Hash(key_pub);
  
  messages::Transaction transaction;
  auto input = transaction.add_inputs();

  auto input_id = input->mutable_id();
  input_id->set_type(messages::Hash::SHA256);
  input_id->set_data("");
  input->set_output_id(0);

  auto output = transaction.add_outputs();
  output->mutable_value()->set_value(ncc);
  auto addr = output->mutable_address();
  addr->set_data(address.data(), address.size());
  addr->set_type(messages::Hash::SHA256);

  transaction.set_fees(fees);
  
  if(type == "json") {
    std::string t;
    messages::to_json(transaction, &t);
    std::cout << t << std::endl;
  } else if (type == "bson") {
    std::cout << "bson output not implemented" << std::endl;
  } else if (type == "protobuf") {
    Buffer t;
    messages::to_buffer(transaction, &t);
    std::cout << t << std::endl;
  } else {
    std::cout << "Wrong output type" << std::endl;
    std::cout << desc << "\n";
    return 1;
  }
  
  return 0;
}

}  // neuro


int main(int argc, char *argv[]) {
  return neuro::main(argc, argv);
}