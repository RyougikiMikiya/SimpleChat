#include <iterator>

#include "simpleMessage.h"



simpleMessage::simpleMessage(MESSAGEID ID) : m_ID(ID)
{

}


simpleMessage::~simpleMessage()
{
}

void simpleMessage::AddPayload(std::stringstream &payload)
{
    m_Payload.insert(m_Payload.end(), std::istreambuf_iterator<char>(payload),
                                      std::istreambuf_iterator<char>());
}

int simpleMessage::FrameHead = 0x4321;
