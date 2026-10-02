/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManagerTests.h"

#include "deskflow/KeyboardLayoutManager.h"

void KeyboardLayoutManagerTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void KeyboardLayoutManagerTests::remoteLayouts()
{
  std::string remoteLayouts = "ruenuk";
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));

  manager.setRemoteLayouts(std::string());
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::remoteLayouts_tooShort_returnsEmpty()
{
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts("a");
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::remoteLayouts_oddLength_returnsEmpty()
{
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts("rue");
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::localLayout()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);
  QCOMPARE(manager.getLocalLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));
}

void KeyboardLayoutManagerTests::missedLayout()
{
  std::string remoteLayouts = "ruenuk";
  std::vector<std::string> localLayouts = {"en"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getMissedLayouts(), "ru, uk");
}

void KeyboardLayoutManagerTests::layoutInstall()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QVERIFY(!manager.isLayoutInstalled("us"));
  QVERIFY(manager.isLayoutInstalled("en"));
}

void KeyboardLayoutManagerTests::serializeLocalLayouts()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QCOMPARE(manager.getSerializedLocalLayouts(), "ruenuk");
}

void KeyboardLayoutManagerTests::normalizeLanguageCode_data()
{
  QTest::addColumn<QString>("language");
  QTest::addColumn<QString>("expected");
  QTest::newRow("simplified Chinese") << "zh-Hans" << "zh";
  QTest::newRow("traditional Chinese") << "zh-Hant" << "zh";
  QTest::newRow("English region") << "en-US" << "en";
  QTest::newRow("underscore region") << "zh_CN" << "zh";
  QTest::newRow("uppercase") << "EN" << "en";
  QTest::newRow("plain language") << "ja" << "ja";
  QTest::newRow("empty") << "" << "";
  QTest::newRow("unsupported language length") << "eng" << "";
  QTest::newRow("invalid") << "z1-Hans" << "";
}

void KeyboardLayoutManagerTests::normalizeLanguageCode()
{
  QFETCH(QString, language);
  QFETCH(QString, expected);
  QCOMPARE(deskflow::KeyboardLayoutManager::normalizeLanguageCode(language.toStdString()), expected.toStdString());
}

QTEST_MAIN(KeyboardLayoutManagerTests)
