#include "MessageBase.h"

MessageBase::MessageBase()
    : m_id(s_nextId++)
    , m_schedulerId(0)
    , m_isParallel(false)
    , m_isReplayed(false)
    , m_sendAsTask(true)
    , m_keepContent(false)
    , m_isLast(true)
    , m_isLogged(true) {}

MessageBase::~MessageBase() = default;

Id MessageBase::s_nextId = 1;