#include "PiiConsensus.hpp"

namespace neuro {
namespace consensus {

PiiConsensus::PiiConsensus(ledger::LedgerMongodb &ledger, uint32_t block_assembly) : _ledger(ledger),
    _valide_block(true) {
    // cotr
    // load all block from
    _assembly_blocks = block_assembly;
    bool save_valide = _valide_block;
    _valide_block = false;

    int height = _ledger.height();
    for(int i = 0 ; i <= height ; ++i ) {
        messages::Block block;
        _ledger.get_block(i, &block);
        add_block(block);
    }
    _valide_block = save_valide;
}

void PiiConsensus::add_block(const neuro::messages::Block &block) {
    ///!< verif block suppose Correct Calcul of PII
    if (_valide_block && !check_owner(block.header())) {
        throw std::runtime_error("Erreur of Owner");
        return;  // TO DO
    }

    neuro::messages::Block last_block, prev_block;
    ///!< compare heigth with ledger
    _ledger.get_block(_ledger.height(), &last_block);
    ///!< get prev block from this
    _ledger.get_block(block.header().previous_block_hash(), &prev_block);

    if ( _valide_block ) {
        Forkmanager::ForkStatus r = _Forkmanager.fork_status(
                                    block.header(),
                                    prev_block.header(),
                                    last_block.header());
        if ( r == Forkmanager::ForkStatus::Non_Fork ) {
            _ledger.push_block(block);
        } else {
            /// add it to
            _ledger.fork_add_block(block);
            _Forkmanager.fork_results(_ledger);
            throw std::runtime_error({"Fork " + std::to_string(r) } );
        }
    }

    ///!< build PiiTransaction for intermider calcule
    std::vector<PiiTransaction> _piithx;
    std::vector<std::pair<std::string, uint64_t> > _output, _input;

    ///!< From https://developers.google.com/protocol-buffers/docs/cpptutorial
    for (int j = 0; j < block.transactions_size(); j++) {
        const neuro::messages::Transaction &transaction = block.transactions(j);
        _output.clear();
        _input.clear();
        /* output first */
        for (int jo = 0; jo < transaction.outputs_size(); jo++) {
            const neuro::messages::Output &output = transaction.outputs(jo);
            _output.push_back({output.address().SerializeAsString(),
                               output.value().value() //std::atol(output.value().value().c_str())
                              }///!< ProtoBuf to JSON String for uint64_t fix
                             );
        }
        /* inputs */
        double somme_Inputs = 0;

        for (int ji = 0; ji < transaction.inputs_size(); ji++) {
            const neuro::messages::Input &input = transaction.inputs(ji);
            // Get out ref and block header for time
            // std::unique_ptr<neuro::messages::Transaction> thinput =
            // std::make_unique<neuro::messages::Transaction>();
            messages::Transaction thinput;
            // #error  TO DO get transaction
            uint64_t add = 0;
            auto inputid = input.id();
            if ( _ledger.get_transaction(inputid, &thinput)) {
                const neuro::messages::Output &thxouput = thinput.outputs(input.output_id());
                add = thxouput.value().value() ; //std::atol(thxouput.value().value().c_str());
                _input.push_back({thxouput.address().SerializeAsString(),add});
            }
            somme_Inputs += add;
        }

        for (const auto &input : _input)
            for (const auto &output : _output)
                _piithx.push_back(
                    PiiTransaction{input.first, output.first,
                                   (input.second / somme_Inputs) * output.second, 1});
    }

    addBlocks(_piithx);

    _last_heigth_block = block.header().height();
    if (((_last_heigth_block+1) % _assembly_blocks) == 0) {
        std::cout << "I m in new assembly "<< std::endl;
        calcul();
        random_from_hashs();
    }
}

void PiiConsensus::add_blocks(const std::vector<neuro::messages::Block *> &blocks) {
    for (const auto block : blocks)
        add_block(*block);
}

void PiiConsensus::random_from_hashs() {
    // Get all hash from 2000 block
    std::vector<std::string> _hash;
    for (uint32_t i = 0; i < _assembly_blocks; i++) {
        neuro::messages::Block b;
        ///!< assembly_blocks/2 is juste pour test
        _ledger.get_block(_last_heigth_block - i - _assembly_blocks / 2, &b);
        _hash.push_back(b.header().id().data());
    }

    _nonce_assembly = 0;
    for (const std::string &h : _hash)
        _nonce_assembly += std::atoi(h.substr(0, 4).c_str());  ///!< just for fun oh
}

uint32_t PiiConsensus::ramdon_at(int index, uint64_t nonce) const {
    std::srand(nonce);
    int r = std::rand();
    for (int i = 0; i < index; i++)
        r = std::rand();

    return static_cast<uint32_t>(r % _assembly_blocks);
}

std::string PiiConsensus::get_next_owner() const {
    return operator()(
               ramdon_at((_last_heigth_block + 1) % _assembly_blocks, _nonce_assembly));
}

bool PiiConsensus::check_owner(const neuro::messages::BlockHeader &blockheader) const {


    std::string owner_p = operator()(
                              ramdon_at(blockheader.height() % _assembly_blocks
                                        , _nonce_assembly
                                       ));
    Buffer buf(blockheader.author().raw_data());
    messages::Address author_addr(buf);

    /** for debug
    std::string t;
    Buffer buftmp(owner_p);
    messages::Hash owner_addr;
    owner_addr.ParseFromString(owner_p);
    messages::to_json(owner_addr,&t);
    std::cout << "Owner -- " << t << std::endl;
    t = "";
    messages::to_json(author_addr, &t);
    std::cout << "Autho -- " << t << std::endl;
    std::cout << "Height -- " << bh.height() << std::endl;
    */
    return (author_addr.SerializeAsString() == owner_p);
}

}  // namespace consensus
}  // namespace neuro