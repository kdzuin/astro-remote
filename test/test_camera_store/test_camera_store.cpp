// Native unit tests for CameraStore — the pure list-of-remembered-cameras
// logic (add-if-absent, name handling, active-camera tracking, forget rules).
//
// Strategy: unity-build. CameraStore has no hardware or NVS dependency, so we
// just #include the real .cpp and exercise it directly. No mocks needed.

#include <unity.h>

#include "transport/camera_store.cpp"

// ---- Helpers ----------------------------------------------------------------

static CameraStore makeStore() { return CameraStore(); }

// ---- add: insert-if-absent --------------------------------------------------

void test_add_new_camera_appends() {
    CameraStore s = makeStore();
    TEST_ASSERT_TRUE(s.add("AA:BB", "ILCE-7M4"));
    TEST_ASSERT_EQUAL_UINT(1, s.count());
    TEST_ASSERT_EQUAL_STRING("ILCE-7M4", s.find("AA:BB")->name.c_str());
}

void test_add_existing_address_is_noop_returns_false() {
    CameraStore s = makeStore();
    s.add("AA:BB", "ILCE-7M4");
    TEST_ASSERT_FALSE(s.add("AA:BB", "ILCE-7M4"));  // already present
    TEST_ASSERT_EQUAL_UINT(1, s.count());
}

void test_add_two_distinct_cameras() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.add("CC:DD", "B");
    TEST_ASSERT_EQUAL_UINT(2, s.count());
}

// ---- add: name handling -----------------------------------------------------

void test_add_existing_does_not_clobber_name_with_empty() {
    CameraStore s = makeStore();
    s.add("AA:BB", "ILCE-7M4");
    s.add("AA:BB", "");  // reconnect path carries no name
    TEST_ASSERT_EQUAL_STRING("ILCE-7M4", s.find("AA:BB")->name.c_str());
}

void test_add_fills_previously_empty_name() {
    CameraStore s = makeStore();
    s.add("AA:BB", "");        // saved with no name somehow
    s.add("AA:BB", "ILCE-7M4");  // later a scan supplies the name
    TEST_ASSERT_EQUAL_STRING("ILCE-7M4", s.find("AA:BB")->name.c_str());
}

// ---- capacity ---------------------------------------------------------------

void test_add_rejects_beyond_cap() {
    CameraStore s = makeStore();
    for (size_t i = 0; i < CameraStore::MAX_CAMERAS; i++) {
        TEST_ASSERT_TRUE(s.add(std::to_string(i), "cam"));
    }
    TEST_ASSERT_TRUE(s.isFull());
    TEST_ASSERT_FALSE(s.add("overflow", "cam"));
    TEST_ASSERT_EQUAL_UINT(CameraStore::MAX_CAMERAS, s.count());
}

// ---- find -------------------------------------------------------------------

void test_find_missing_returns_null() {
    CameraStore s = makeStore();
    TEST_ASSERT_NULL(s.find("NOPE"));
}

// ---- active tracking --------------------------------------------------------

void test_no_active_initially() {
    CameraStore s = makeStore();
    TEST_ASSERT_FALSE(s.hasActive());
}

void test_set_active_present_camera() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.setActive("AA:BB");
    TEST_ASSERT_TRUE(s.hasActive());
    TEST_ASSERT_EQUAL_STRING("AA:BB", s.activeAddress().c_str());
}

void test_set_active_unknown_is_ignored() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.setActive("AA:BB");
    s.setActive("ZZ:ZZ");  // not in list
    TEST_ASSERT_EQUAL_STRING("AA:BB", s.activeAddress().c_str());
}

// ---- forget -----------------------------------------------------------------

void test_forget_removes_camera() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.add("CC:DD", "B");
    s.forget("AA:BB");
    TEST_ASSERT_EQUAL_UINT(1, s.count());
    TEST_ASSERT_NULL(s.find("AA:BB"));
}

void test_forget_active_clears_active() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.setActive("AA:BB");
    s.forget("AA:BB");
    TEST_ASSERT_FALSE(s.hasActive());
}

void test_forget_non_active_leaves_active() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.add("CC:DD", "B");
    s.setActive("AA:BB");
    s.forget("CC:DD");
    TEST_ASSERT_TRUE(s.hasActive());
    TEST_ASSERT_EQUAL_STRING("AA:BB", s.activeAddress().c_str());
}

void test_forget_missing_is_noop() {
    CameraStore s = makeStore();
    s.add("AA:BB", "A");
    s.forget("NOPE");
    TEST_ASSERT_EQUAL_UINT(1, s.count());
}

// ---- test runner ------------------------------------------------------------

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_add_new_camera_appends);
    RUN_TEST(test_add_existing_address_is_noop_returns_false);
    RUN_TEST(test_add_two_distinct_cameras);
    RUN_TEST(test_add_existing_does_not_clobber_name_with_empty);
    RUN_TEST(test_add_fills_previously_empty_name);
    RUN_TEST(test_add_rejects_beyond_cap);
    RUN_TEST(test_find_missing_returns_null);
    RUN_TEST(test_no_active_initially);
    RUN_TEST(test_set_active_present_camera);
    RUN_TEST(test_set_active_unknown_is_ignored);
    RUN_TEST(test_forget_removes_camera);
    RUN_TEST(test_forget_active_clears_active);
    RUN_TEST(test_forget_non_active_leaves_active);
    RUN_TEST(test_forget_missing_is_noop);
    return UNITY_END();
}
