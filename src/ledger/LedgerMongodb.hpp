#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include "ledger/Filter.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

class LedgerMongodb {
 private:
  mutable mongocxx::instance _instance{};
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;
  mutable mongocxx::collection _transactions;
  
  bool get_block_header(const bsoncxx::document::view &block,
                        messages::BlockHeader *header) {

    messages::from_bson(block, header);
    return true;
  }

  // bool
  // get_block(const bsoncxx::document::view &bson_block,
  //           messages::Block *block);
  ~LedgerMongodb() {}

  bool get_transactions_from_block(const bsoncxx::document::view &id,
                                   messages::Block *block) {
    auto query = bss::document{} << "header.id" << id << bss::finalize;
    auto cursor = _transactions.find(std::move(query));

    for(auto &bson_transaction: cursor) {
      auto transaction = block->add_transactions();
      from_bson(bson_transaction, transaction);
    }

    return true;
  }

  bool get_transactions_from_block(const messages::BlockID &id,
                                   messages::Block *block) {
    const auto bson_id = to_bson(id);
    return get_transactions_from_block(bson_id.view(), block);
  }

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);

  messages::BlockHeight height() const {
    auto query = bss::document{} << bss::finalize;
    auto options = mongocxx::options::find{};
    options.sort(bss::document{} << "header.height" << -1 << bss::finalize);
    const auto res = _blocks.find_one(std::move(query), options);
    if(!res) {
      return 0;
    }
    
    return res->view()["header.height"].get_int32().value;
  }

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header) {
    
    const auto bson_id = to_bson(id);

    auto query = bss::document{} << "id" << bson_id << bss::finalize;
    auto res = _blocks.find_one(std::move(query));
    if (!res) {
      return false;
    }
    return get_block_header(res->view(), header);
  }

  bool get_last_block_header(messages::BlockHeader *block_header) {
    auto query = bss::document{} << bss::finalize;
    auto options = mongocxx::options::find{};
    options.sort(bss::document{} << "header.height" << -1 << bss::finalize);
    const auto res = _blocks.find_one(std::move(query), options);
    if(!res) {
      return false;
    }
    
    messages::from_bson(*res, block_header);
    return true;
  }

  bool get_block(const messages::BlockID &id,
                 messages::Block *block) {

    auto header = block->mutable_header();
    auto res_state = get_block_header(id, header);

    res_state &= get_transactions_from_block(id, block);

    return res_state;
  }
  
  
  bool get_block(const messages::BlockHeight height,
                 messages::Block *block) {

    auto query = bss::document{} << "header.height" <<  height << bss::finalize;
    const auto res = _blocks.find_one(std::move(query));
    if(!res) {
      return false;
    }

    get_transactions_from_block(res->view()["header.id"].get_document(), block);
    
    return true;
  }

  bool push_block(const messages::Block &block) {
    const auto header = block.header();
    auto bson_header = messages::to_bson(header);

    _blocks.insert_one(std::move(bson_header));

    for (const auto &transaction: block.transactions()) { 
      auto bson_transaction = messages::to_bson(transaction);
      _transactions.insert_one(std::move(bson_transaction));
    }
    return true;
  }
  

  
  
  // messages::Transaction::State get_block_header(const messages::BlockID &id,
  //                                 messages::BlockHeader *header) {

  //   bsoncxx::document bson_id;
  //   to_bson(id, &bson_id);
  
  //   auto query = bss::document{} << "header.id" << bson_id << bss::finalize;
  //   auto res = _blocks.find_one(std::move(query));
  //   if (!res) {
  //     return messages::Transaction::UNKNOWN;
  //   }

  //   return get_block_header(res->view(), header);
  // }


  //   messages::BlockHeader::State get_block(const messages::BlockID &id,
  // 					 messages::Block *block) {

  //   auto header = block->mutable_header();
  //   auto res_state = get_block_header(id, header);

  //   bsoncxx::document::value bson_id;
  //   to_bson(id, &bson_id);
  
  //   auto query = bss::document{} << "block_id" << bson_id << bss::finalize;
  //   auto cursor = _transactions.find(query);

  //   for (const auto transaction : cursor) {
    
  //   }

  
  // messages::BlockHeader::State get_block(const messages::BlockID &id,
  // 					 messages::Block *block);
  // messages::BlockHeader::State get_block(const messages::BlockHeight height,
  // 					 messages::Block *block);
  // messages::BlockHeader::State get_last_block(messages::Block *block);

  // bool push_block(const messages::Block &block);
  // bool delete_block(const messages::BlockID &id);
  // bool for_each(Filter filter, Functor functor);
  // messages::Transaction::State get_transaction(
  //     const messages::TransactionID &id, messages::Transaction *transaction);
  // bool has_transaction(const messages::TransactionID &transaction_id) const;
  // bool has_transaction(const messages::BlockID &block_id,
  //                      const messages::TransactionID &transaction_id) const;
  // messages::Transaction::State transaction(
  //     const messages::BlockID &block_id, const messages::TransactionID &id,
  //     messages::Transaction *transaction) const;

  // ~LedgerMongodb();
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
