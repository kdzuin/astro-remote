// Native unit tests for SelectableList navigation logic.
// draw() is exercised too (against the no-op display stub) to guard the code
// path, but assertions target selection/skip behaviour only.

#include <unity.h>

#include "Arduino.h"
#include "M5Unified.h"

// Globals the fakes require.
uint32_t g_fakeMillis = 0;
SerialStub Serial;
M5Stub M5;

#include "components/selectable_list.h"

enum class Item { A, B, C, D, None };

void setUp() {}
void tearDown() {}

// selectNext wraps and lands on real items. Title adds a leading separator, so
// a fresh list with 2 items has layout: [sep, A, B].
void test_next_skips_separators() {
    SelectableList<Item> list("Menu");
    list.clear();
    list.addItem(Item::A, "A");
    list.addItem(Item::B, "B");

    // Index starts at 0 (the separator). First next -> A.
    TEST_ASSERT_TRUE(list.selectNext());
    TEST_ASSERT_EQUAL(static_cast<int>(Item::A), static_cast<int>(list.getSelectedId()));
    list.selectNext();
    TEST_ASSERT_EQUAL(static_cast<int>(Item::B), static_cast<int>(list.getSelectedId()));
    // Wrap past end skips the separator back to A.
    list.selectNext();
    TEST_ASSERT_EQUAL(static_cast<int>(Item::A), static_cast<int>(list.getSelectedId()));
}

// A separator between groups must be skipped by both directions.
void test_mid_separator_skipped() {
    SelectableList<Item> list("Menu");
    list.clear();
    list.addItem(Item::A, "A");
    list.addSeparator();
    list.addItem(Item::B, "B");

    list.selectNext();  // -> A
    TEST_ASSERT_EQUAL(static_cast<int>(Item::A), static_cast<int>(list.getSelectedId()));
    list.selectNext();  // skip mid separator -> B
    TEST_ASSERT_EQUAL(static_cast<int>(Item::B), static_cast<int>(list.getSelectedId()));
    list.selectPrev();  // back over separator -> A
    TEST_ASSERT_EQUAL(static_cast<int>(Item::A), static_cast<int>(list.getSelectedId()));
}

// selectPrev wraps backward across the leading separator.
void test_prev_wraps() {
    SelectableList<Item> list("Menu");
    list.clear();
    list.addItem(Item::A, "A");
    list.addItem(Item::B, "B");

    list.selectNext();  // A
    list.selectPrev();  // wrap back -> B (skipping leading separator)
    TEST_ASSERT_EQUAL(static_cast<int>(Item::B), static_cast<int>(list.getSelectedId()));
}

// Empty list navigation returns false and does not crash.
void test_empty_list() {
    SelectableList<Item> list("Menu");
    list.clear();  // only the title separator present
    // No real items: selectNext should fail to find a non-separator.
    TEST_ASSERT_FALSE(list.selectNext());
}

// draw() against the stub display must not crash with mixed content.
void test_draw_smoke() {
    SelectableList<Item> list("Menu");
    list.clear();
    list.addItem(Item::A, "A");
    list.addItem(Item::B, "B", "info", true);
    list.addSeparator();
    list.addItem(Item::C, "C", false);
    list.draw();
    TEST_PASS();
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_next_skips_separators);
    RUN_TEST(test_mid_separator_skipped);
    RUN_TEST(test_prev_wraps);
    RUN_TEST(test_empty_list);
    RUN_TEST(test_draw_smoke);
    return UNITY_END();
}
