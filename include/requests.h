//
// Created by d4wgr on 5/8/2025.
//

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "dictionary.h"
#include "resp.h"

namespace requests {
    void handle(const resp::Message &request, std::ostream &stream, Dictionary &dictionary);
}

#endif //REQUEST_HANDLER_H
