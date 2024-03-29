#ifndef SIO_TOKENIZER_H
#define SIO_TOKENIZER_H

#include <fstream>

#include "sio/base.h"

namespace sio {

using TokenId = i32;
constexpr TokenId kNoTokenId = -1;

class Tokenizer {
    Map<TokenId, Str> index_to_token_; // TODO: consider vector implementation
    Map<Str, TokenId> token_to_index_;

public:
    TokenId blk = kNoTokenId;
    TokenId unk = kNoTokenId;
    TokenId bos = kNoTokenId;
    TokenId eos = kNoTokenId;

    Error Load(const Str& tokenizer_vocab) {
        std::ifstream is(tokenizer_vocab);
        SIO_CHECK(is.good());
        Str line;
        for (TokenId index = 0; std::getline(is, line); index++) {
            Vec<Str> cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 2); // token prob
            Str token = cols[0];
            index_to_token_[index] = token;
            token_to_index_[token] = index;

            if (token == "<blk>" || token == "<blank>" || token == "<pad>") {
                blk = index;
            } else if (token == "<unk>" || token == "<UNK>") {
                unk = index;
            } else if (token == "<s>" || token == "<bos>" || token == "<sos>") {
                bos = index;
            } else if (token == "</s>" || token == "<eos>") {
                eos = index;
            }
        }

        // Use blk & unk interchangably if only one of them is undefined
        if (blk == kNoTokenId && unk != kNoTokenId) { blk = unk; }
        if (unk == kNoTokenId && blk != kNoTokenId) { unk = blk; }

        //dbg(index_to_token_);
        //dbg(token_to_index_);

        // Post-condition checks
        SIO_CHECK(blk != kNoTokenId);
        SIO_CHECK(unk != kNoTokenId);
        SIO_CHECK(bos != kNoTokenId);
        SIO_CHECK(eos != kNoTokenId);

        return Error::OK;
    }


    size_t Size() const {
        return index_to_token_.size();
    }


    const Str& Token(TokenId t) const {
        return index_to_token_.at(t);
    }


    TokenId Index(const Str& token) const {
        return token_to_index_.at(token);
    }

}; // class Tokenizer
}  // namespace sio
#endif
