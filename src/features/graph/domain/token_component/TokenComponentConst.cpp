#include "graph/domain/token_component/TokenComponentConst.h"

std::shared_ptr<TokenComponent> TokenComponentConst::copy() const {
  return std::make_shared<TokenComponentConst>(*this);
}
