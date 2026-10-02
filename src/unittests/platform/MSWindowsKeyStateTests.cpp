/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsKeyStateTests.h"
#include "base/EventQueue.h"
#include "platform/MSWindowsKeyState.h"
#include <QTest>

namespace {
class RecordingKeyState : public MSWindowsKeyState
{
public:
  RecordingKeyState(IEventQueue *events, deskflow::KeyMap &map)
      : MSWindowsKeyState(nullptr, nullptr, events, map, {"zh"}, true)
  {
    setMacCapsLockSync(true);
  }
  using MSWindowsKeyState::synchronizeInputMethod;
  HWND foreground = reinterpret_cast<HWND>(1);
  HWND focus = reinterpret_cast<HWND>(2);
  HKL layout = reinterpret_cast<HKL>(3);
  bool available = true;
  bool fails = false;
  mutable int calls = 0;
  mutable bool open = false;
  mutable DWORD_PTR conversion = 0x8;
  int32_t pollActiveGroup() const override
  {
    return 0;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }

protected:
  bool queryInputMethodTarget(HWND &outForeground, HWND &outFocus, HKL &outLayout) const override
  {
    outForeground = foreground;
    outFocus = focus;
    outLayout = layout;
    return available;
  }
  HWND getInputMethodWindow(HWND target) const override
  {
    return target;
  }
  bool controlInputMethod(HWND, WPARAM command, LPARAM value, DWORD_PTR &result) const override
  {
    ++calls;
    if (fails) {
      return false;
    }
    switch (command) {
    case 1:
      result = conversion;
      break;
    case 2:
      conversion = value;
      break;
    case 5:
      result = open;
      break;
    case 6:
      open = value != 0;
      break;
    default:
      return false;
    }
    return true;
  }
};
} // namespace

void MSWindowsKeyStateTests::initTestCase()
{
  m_arch.init();
}

void MSWindowsKeyStateTests::repeatedKeys_skipImeMessages()
{
  EventQueue events;
  deskflow::KeyMap map;
  RecordingKeyState state(&events, map);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 4);
  QVERIFY(state.open);
  QCOMPARE(state.conversion, DWORD_PTR(0x9));
  for (int i = 0; i < 100; ++i) {
    state.synchronizeInputMethod("zh");
  }
  QCOMPARE(state.calls, 4);
  state.synchronizeInputMethod("en");
  QCOMPARE(state.calls, 8);
  QVERIFY(!state.open);
  QCOMPARE(state.conversion, DWORD_PTR(0x8));
}

void MSWindowsKeyStateTests::changedContext_resynchronizes()
{
  EventQueue events;
  deskflow::KeyMap map;
  RecordingKeyState state(&events, map);
  state.synchronizeInputMethod("zh");
  state.focus = reinterpret_cast<HWND>(4);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 6);
  state.foreground = reinterpret_cast<HWND>(5);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 8);
  state.layout = reinterpret_cast<HKL>(6);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 10);
  state.available = false;
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 10);
  state.available = true;
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 12);
}

void MSWindowsKeyStateTests::failedAttempt_isNotRetriedForEveryKey()
{
  EventQueue events;
  deskflow::KeyMap map;
  RecordingKeyState state(&events, map);
  state.fails = true;
  for (int i = 0; i < 100; ++i) {
    state.synchronizeInputMethod("zh");
  }
  QCOMPARE(state.calls, 1);
  state.fails = false;
  state.synchronizeInputMethod("en");
  QCOMPARE(state.calls, 3);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 7);
}

void MSWindowsKeyStateTests::inputStateAndOptionChanges_invalidateCache()
{
  EventQueue events;
  deskflow::KeyMap map;
  RecordingKeyState state(&events, map);
  state.synchronizeInputMethod("zh");
  state.open = false;
  state.conversion = 0x8;
  state.synchronizeInputState(0, "zh");
  QCOMPARE(state.calls, 8);
  QVERIFY(state.open);
  state.setMacCapsLockSync(false);
  state.synchronizeInputMethod("en");
  QCOMPARE(state.calls, 8);
  state.setMacCapsLockSync(true);
  state.synchronizeInputMethod("zh");
  QCOMPARE(state.calls, 10);
}

QTEST_MAIN(MSWindowsKeyStateTests)
