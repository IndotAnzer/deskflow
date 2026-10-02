/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyStateTests.h"
#include "base/EventQueue.h"
#include "deskflow/KeyMap.h"

#include "MockEventQueue.h"
#include "MockKeyMap.h"
#include "MockKeyState.h"

#include <algorithm>

namespace {

//! KeyState that records the keystrokes it is asked to synthesize.
class RecordingKeyState : public KeyState
{
public:
  using KeyState::synchronizeCapsLock;
  RecordingKeyState(IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool langSync)
      : KeyState(events, keyMap, std::move(layouts), langSync)
  {
  }

  int32_t pollActiveGroup() const override
  {
    return m_activeGroup;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  void getKeyMap(deskflow::KeyMap &) override
  {
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  void pollPressedKeys(KeyButtonSet &) const override
  {
  }
  void fakeKey(const Keystroke &keystroke) override
  {
    m_faked.push_back(keystroke);
  }
  void synchronizeInputMethod(const std::string &lang) override
  {
    m_syncedLanguages.push_back(lang);
    m_strokesBeforeSync.push_back(m_faked.size());
  }

  int countStrokes(Keystroke::KeyType type) const
  {
    return static_cast<int>(std::count_if(m_faked.begin(), m_faked.end(), [type](const Keystroke &k) {
      return k.m_type == type;
    }));
  }

  int32_t m_activeGroup = 0;
  std::vector<Keystroke> m_faked;
  std::vector<std::string> m_syncedLanguages;
  std::vector<size_t> m_strokesBeforeSync;
};

//! Two groups on one button: a latin key in group 0 ("en"), a thai key in group 1 ("th").
void buildTwoGroupKeyMap(deskflow::KeyMap &keyMap, KeyID enKey, KeyID thKey, KeyButton button)
{
  deskflow::KeyMap::KeyItem item;
  item.m_button = button;

  item.m_id = enKey;
  item.m_group = 0;
  keyMap.addKeyEntry(item);

  item.m_id = thKey;
  item.m_group = 1;
  keyMap.addKeyEntry(item);

  keyMap.finish();
}

constexpr KeyID kLatinA = 'a';
constexpr KeyID kThaiFoFan = 0x0e1f; // ฟ, the same physical key as 'a' on a thai layout
constexpr KeyButton kSharedButton = 1;
constexpr KeyButton kCapsButton = 3;

void buildCapsKeyMap(deskflow::KeyMap &keyMap)
{
  deskflow::KeyMap::KeyItem caps;
  caps.m_id = kKeyCapsLock;
  caps.m_button = kCapsButton;
  deskflow::KeyMap::initModifierKey(caps);
  keyMap.addKeyEntry(caps);
  keyMap.finish();
}

// Supply a Windows-style repeat sequence so the common synthesis loop is tested
// on Linux too, where KeyMap leaves repeats to the platform implementation.
class RepeatKeyMap : public deskflow::KeyMap
{
public:
  const KeyItem *mapKey(
      Keystrokes &keys, KeyID, int32_t, ModifierToKeys &, KeyModifierMask &, KeyModifierMask, bool repeat,
      const std::string &
  ) const override
  {
    if (repeat) {
      keys.emplace_back(kSharedButton, false, true, 0);
    }
    keys.emplace_back(kSharedButton, true, repeat, 0);
    return &m_item;
  }

private:
  KeyItem m_item{.m_id = kLatinA, .m_button = kSharedButton};
};

} // namespace

void KeyStateTests::initTestCase()
{
  m_arch.init();
}

void KeyStateTests::keyDown()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::keyUp()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);
  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::invalidKey()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::onKey_aKeyDown_keyStateOne()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, true, KeyModifierAlt);

  QVERIFY(keyState.getKeyState(1));
}

void KeyStateTests::onKey_aKeyUp_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(1, false, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(1));
}

void KeyStateTests::onKey_invalidKey_keyStateZero()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.onKey(0, true, KeyModifierAlt);

  QVERIFY(!keyState.getKeyState(0));
}

void KeyStateTests::updateKeyState_pollDoesNothing_keyNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_activeModifiers_maskNotSet()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  keyState.updateKeyState();

  QCOMPARE(0, keyState.getActiveModifiers());
}

void KeyStateTests::fakeKeyRepeat_invalidKey_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyRepeat(0, 0, 0, 0, "en"));
}

void KeyStateTests::fakeKeyUp_buttonNotDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.fakeKeyUp(0));
}

void KeyStateTests::isKeyDown_noKeysDown_returnsFalse()
{
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, m_keymap);

  QVERIFY(!keyState.isKeyDown(1));
}

void KeyStateTests::isKeyDown_keyDown_retrunsTrue()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::updateKeyState_pollInsertsSingleKey_keyIsDown()
{
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  MockKeyState keyState(eventQueue, keyMap);

  deskflow::KeyMap::KeyItem key;
  key.m_button = 1;
  keyState.fakeKeyDown(1, 0, 1, "en");

  keyState.updateKeyState();
  QVERIFY(keyState.isKeyDown(1));
}

void KeyStateTests::fakeKeyDown_langSyncEnabled_switchesToServerGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 1);
  const auto group = std::find_if(keyState.m_faked.begin(), keyState.m_faked.end(), [](const auto &k) {
    return k.m_type == deskflow::KeyMap::Keystroke::KeyType::Group;
  });
  QCOMPARE(group->m_data.m_group.m_group, 1);
}

void KeyStateTests::fakeKeyDown_langSyncDisabled_keepsLocalGroup()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);

  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, false);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  // the local layout must be left alone, but the key itself is still pressed by position
  QCOMPARE(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 0);
  QVERIFY(keyState.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button) > 0);
  QCOMPARE(keyState.m_faked.front().m_data.m_button.m_button, kSharedButton);
}

void KeyStateTests::fakeKeyDown_syncsInputMethodAfterGroupBeforeButton()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kThaiFoFan, 0, kSharedButton, "th");

  QCOMPARE(keyState.m_syncedLanguages, (std::vector<std::string>{"th"}));
  QCOMPARE(keyState.m_strokesBeforeSync, (std::vector<size_t>{1}));
  QCOMPARE(keyState.m_faked[0].m_type, deskflow::KeyMap::Keystroke::KeyType::Group);
  QCOMPARE(keyState.m_faked[1].m_type, deskflow::KeyMap::Keystroke::KeyType::Button);

  keyState.fakeKeyUp(kSharedButton);
  QCOMPARE(keyState.m_syncedLanguages.size(), size_t(1));
}

void KeyStateTests::fakeKeyRepeat_syncsInputMethod()
{
  MockEventQueue eventQueue;
  RepeatKeyMap keyMap;
  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kLatinA, 0, kSharedButton, "en");
  QVERIFY(keyState.fakeKeyRepeat(kLatinA, 0, 3, kSharedButton, "en"));
  QCOMPARE(keyState.m_syncedLanguages, (std::vector<std::string>{"en", "en"}));
}

void KeyStateTests::fakeKeyDown_inputMethodSyncDisabled()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, false);
  keyState.fakeKeyDown(kLatinA, 0, kSharedButton, "en");
  QVERIFY(keyState.m_syncedLanguages.empty());
}

void KeyStateTests::fakeKeyDown_emptyLanguageDoesNotSyncInputMethod()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildTwoGroupKeyMap(keyMap, kLatinA, kThaiFoFan, kSharedButton);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en", "th"}, true);
  keyState.fakeKeyDown(kLatinA, 0, kSharedButton, {});
  QVERIFY(keyState.m_syncedLanguages.empty());
}

void KeyStateTests::synchronizeCapsLock_setsAndClearsState()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildCapsKeyMap(keyMap);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.setMacCapsLockSync(true);

  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  QCOMPARE(keyState.getActiveModifiers(), KeyModifierCapsLock);
  QCOMPARE(keyState.m_faked.size(), size_t(2));
  QCOMPARE(keyState.m_faked[0].m_data.m_button.m_button, kCapsButton);
  QVERIFY(keyState.m_faked[0].m_data.m_button.m_press);
  QVERIFY(!keyState.m_faked[1].m_data.m_button.m_press);

  keyState.synchronizeCapsLock(0);
  QCOMPARE(keyState.getActiveModifiers(), KeyModifierMask(0));
  QCOMPARE(keyState.m_faked.size(), size_t(4));
  QVERIFY(keyState.m_syncedLanguages.empty());
}

void KeyStateTests::synchronizeCapsLock_matchingStateDoesNotToggle()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildCapsKeyMap(keyMap);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.setMacCapsLockSync(true);
  keyState.synchronizeCapsLock(0);
  QVERIFY(keyState.m_faked.empty());

  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  keyState.m_faked.clear();
  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  QVERIFY(keyState.m_faked.empty());
}

void KeyStateTests::synchronizeCapsLock_preservesOtherModifiers()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildCapsKeyMap(keyMap);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, false);
  keyState.setMacCapsLockSync(true);
  keyState.onKey(0, true, KeyModifierControl | KeyModifierShift | KeyModifierNumLock);

  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  QCOMPARE(
      keyState.getActiveModifiers(), KeyModifierControl | KeyModifierShift | KeyModifierNumLock | KeyModifierCapsLock
  );
  keyState.synchronizeCapsLock(0);
  QCOMPARE(keyState.getActiveModifiers(), KeyModifierControl | KeyModifierShift | KeyModifierNumLock);
  for (const auto &stroke : keyState.m_faked) {
    QCOMPARE(stroke.m_type, deskflow::KeyMap::Keystroke::KeyType::Button);
    QCOMPARE(stroke.m_data.m_button.m_button, kCapsButton);
  }
}

void KeyStateTests::synchronizeCapsLock_disabledLeavesStateAlone()
{
  MockEventQueue eventQueue;
  deskflow::KeyMap keyMap;
  buildCapsKeyMap(keyMap);
  RecordingKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  QVERIFY(!keyState.isMacCapsLockSyncEnabled());
  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  QCOMPARE(keyState.getActiveModifiers(), KeyModifierMask(0));
  QVERIFY(keyState.m_faked.empty());

  keyState.setMacCapsLockSync(true);
  keyState.synchronizeCapsLock(KeyModifierCapsLock);
  keyState.m_faked.clear();
  keyState.setMacCapsLockSync(false);
  keyState.synchronizeCapsLock(0);
  QCOMPARE(keyState.getActiveModifiers(), KeyModifierCapsLock);
  QVERIFY(keyState.m_faked.empty());
}

QTEST_MAIN(KeyStateTests)

void KeyStateTests::synchronizeInputState_withoutTyping()
{
  MockEventQueue events;
  deskflow::KeyMap map;
  buildTwoGroupKeyMap(map, kLatinA, kThaiFoFan, kSharedButton);
  RecordingKeyState state(&events, map, {"en", "th"}, true);
  state.setMacCapsLockSync(true);
  state.synchronizeInputState(0, "th");
  state.synchronizeInputState(0, "en");
  state.synchronizeInputState(0, "th");
  QCOMPARE(state.m_syncedLanguages, (std::vector<std::string>{"th", "en", "th"}));
  QCOMPARE(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Button), 0);
  QCOMPARE(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 3);
  QCOMPARE(state.m_faked[0].m_data.m_group.m_group, 1);
  QCOMPARE(state.m_faked[1].m_data.m_group.m_group, 0);
  QCOMPARE(state.m_faked[2].m_data.m_group.m_group, 1);
  QCOMPARE(state.m_strokesBeforeSync, (std::vector<size_t>{1, 2, 3}));
  QVERIFY(!state.fakeKeyUp(kSharedButton));
}

void KeyStateTests::synchronizeInputState_disabledDoesNothing()
{
  MockEventQueue events;
  deskflow::KeyMap map;
  buildCapsKeyMap(map);
  RecordingKeyState state(&events, map, {"en"}, true);
  state.synchronizeInputState(KeyModifierCapsLock, "en");
  QVERIFY(state.m_faked.empty());
  QVERIFY(state.m_syncedLanguages.empty());
  QCOMPARE(state.getActiveModifiers(), KeyModifierMask(0));
}

void KeyStateTests::synchronizeInputState_languageSyncDisabledOnlySyncsCaps()
{
  MockEventQueue events;
  deskflow::KeyMap map;
  buildCapsKeyMap(map);
  RecordingKeyState state(&events, map, {"en"}, false);
  state.setMacCapsLockSync(true);
  state.synchronizeInputState(KeyModifierCapsLock, "zh");
  QCOMPARE(state.getActiveModifiers(), KeyModifierCapsLock);
  QCOMPARE(state.countStrokes(deskflow::KeyMap::Keystroke::KeyType::Group), 0);
  QVERIFY(state.m_syncedLanguages.empty());
  state.synchronizeInputState(0, "en");
  QCOMPARE(state.getActiveModifiers(), KeyModifierMask(0));
}
