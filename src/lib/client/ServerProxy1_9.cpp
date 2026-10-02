/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ServerProxy1_9.h"
#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include <cstring>

ServerProxy::ConnectionResult ServerProxy1_9::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDInputState, 4) != 0) {
    return ServerProxy1_8::parseMessage(code);
  }
  uint16_t mask = 0;
  std::string lang;
  ProtocolUtil::readf(getStream(), kMsgDInputState + 4, &mask, &lang);
  LOG_VERBOSE("recv input state mask=0x%04x, lang=%s", mask, lang.c_str());
  setActiveServerLayout(lang);
  synchronizeInputState(mask, lang);
  return ConnectionResult::Okay;
}
