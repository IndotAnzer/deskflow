/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "server/ClientProxy1_8.h"

class ClientProxy1_9 : public ClientProxy1_8
{
public:
  using ClientProxy1_8::ClientProxy1_8;
  void synchronizeInputState(KeyModifierMask mask, const std::string &lang) override;
};
