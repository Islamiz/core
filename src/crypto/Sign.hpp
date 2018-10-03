#ifndef NEURO_SRC_CRYPTO_SIGN_HPP
#define NEURO_SRC_CRYPTO_SIGN_HPP

#include "messages.pb.h"
#include "messages/Message.hpp"
#include "crypto/EccPriv.hpp"

namespace neuro {
namespace crypto {

bool sign (const crypto::EccPriv &key_priv,
           const int input_index,
           messages::Transaction *transaction);

}  // crypto
}  // neuro

#endif /* NEURO_SRC_CRYPTO_SIGN_HPP */