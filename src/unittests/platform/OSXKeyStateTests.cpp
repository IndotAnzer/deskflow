/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"

namespace {

class RecordingOSXKeyState : public OSXKeyState
{
public:
  using OSXKeyState::OSXKeyState;
  std::vector<KeyButton> presses;

protected:
  void fakeKey(const Keystroke &key) override
  {
    if (key.m_type == Keystroke::KeyType::Button && key.m_data.m_button.m_press) {
      presses.push_back(key.m_data.m_button.m_button);
    }
  }
};

struct RestoreInputSource
{
  TISInputSourceRef original;
  ~RestoreInputSource()
  {
    TISSelectInputSource(original);
  }
};

} // namespace

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

void OSXKeyStateTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void OSXKeyStateTests::mapModifiersFromOSX_OSXMask()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  KeyModifierMask outMask = 0;

  uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
  outMask = keyState.mapModifiersFromOSX(shiftMask);
  QCOMPARE(outMask, KeyModifierShift);

  uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
  outMask = keyState.mapModifiersFromOSX(ctrlMask);
  QCOMPARE(outMask, KeyModifierControl);

  uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
  outMask = keyState.mapModifiersFromOSX(altMask);
  QCOMPARE(outMask, KeyModifierAlt);

  uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
  outMask = keyState.mapModifiersFromOSX(cmdMask);
  QCOMPARE(outMask, KeyModifierSuper);

  uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
  outMask = keyState.mapModifiersFromOSX(capsMask);
  QCOMPARE(outMask, KeyModifierCapsLock);

  uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
  outMask = keyState.mapModifiersFromOSX(numMask);
  QCOMPARE(outMask, KeyModifierNumLock);
}

void OSXKeyStateTests::fakePollShift()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(SHIFT_ID_L, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyDown(SHIFT_ID_R, 0, 2, "en");
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(2);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));
}

void OSXKeyStateTests::fakePollChar()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

void OSXKeyStateTests::fakePollCharWithModifier()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, KeyModifierShift, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

bool OSXKeyStateTests::isKeyPressed(const OSXKeyState &keyState, KeyButton button)
{
  // HACK: allow os to realize key state changes.
  Arch::sleep(.2);

  IKeyState::KeyButtonSet pressed;
  keyState.pollPressedKeys(pressed);

  IKeyState::KeyButtonSet::const_iterator it;
  for (it = pressed.begin(); it != pressed.end(); ++it) {
    LOG_DEBUG("checking key %d", *it);
    if (*it == button) {
      return true;
    }
  }
  return false;
}

void OSXKeyStateTests::imeClient_usesCharacterLayout_data()
{
  QTest::addColumn<QString>("language");
  QTest::newRow("Chinese IME") << QString("zh");
  QTest::newRow("Japanese IME") << QString("ja");
}

void OSXKeyStateTests::imeClient_usesCharacterLayout()
{
  QFETCH(QString, language);
  AutoTISInputSourceRef original(TISCopyCurrentKeyboardInputSource(), CFRelease);
  QVERIFY(original);
  RestoreInputSource restore{original.get()};
  AutoTISInputSourceRef ascii(TISCopyCurrentASCIICapableKeyboardLayoutInputSource(), CFRelease);
  QVERIFY(ascii);
  QCOMPARE(TISSelectInputSource(ascii.get()), noErr);
  EventQueue events;
  deskflow::KeyMap map;
  RecordingOSXKeyState state(&events, map, {"en"}, false);
  state.updateKeyMap();
  const auto expectedGroup = state.pollActiveGroup();
  state.fakeKeyDown(A_CHAR_ID, 0, 1, {});
  const auto expectedPresses = state.presses;
  QVERIFY(!expectedPresses.empty());
  state.fakeKeyUp(1);
  state.presses.clear();

  AutoCFArray sources(TISCreateInputSourceList(nullptr, false), CFRelease);
  QVERIFY(sources);
  bool selected = false;
  for (CFIndex i = 0; i < CFArrayGetCount(sources.get()) && !selected; ++i) {
    auto source = static_cast<TISInputSourceRef>(const_cast<void *>(CFArrayGetValueAtIndex(sources.get(), i)));
    if (TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData)) {
      continue;
    }
    auto languages = static_cast<CFArrayRef>(TISGetInputSourceProperty(source, kTISPropertyInputSourceLanguages));
    if (!languages) {
      continue;
    }
    for (CFIndex j = 0; j < CFArrayGetCount(languages); ++j) {
      auto code = static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, j));
      if (CFStringHasPrefix(code, language == "zh" ? CFSTR("zh") : CFSTR("ja")) &&
          TISSelectInputSource(source) == noErr) {
        selected = true;
        break;
      }
    }
  }
  if (!selected) {
    QSKIP("No enabled IME for this language on the test machine");
  }
  QCOMPARE(state.pollActiveGroup(), expectedGroup);
  state.fakeKeyDown(A_CHAR_ID, 0, 1, {});
  QCOMPARE(state.presses, expectedPresses);
  state.fakeKeyUp(1);
}

QTEST_MAIN(OSXKeyStateTests)
