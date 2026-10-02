/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_9.h"
#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

void ClientProxy1_9::synchronizeInputState(KeyModifierMask mask, const std::string &lang)
{
  LOG_VERBOSE("send input state to %s, mask=0x%04x, lang=%s", getName().c_str(), mask, lang.c_str());
  ProtocolUtil::writef(getStream(), kMsgDInputState, mask, &lang);
}
