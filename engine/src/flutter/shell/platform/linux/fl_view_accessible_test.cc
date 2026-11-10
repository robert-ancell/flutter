// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Included first as it collides with the X11 headers.
#include "flutter/shell/platform/linux/testing/linux_test.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/linux/fl_accessible_node.h"
#include "flutter/shell/platform/linux/fl_atspi_application.h"
#include "flutter/shell/platform/linux/fl_engine_private.h"
#include "flutter/shell/platform/linux/fl_view_accessible.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_standard_message_codec.h"
#include "flutter/shell/platform/linux/testing/atspi_test_bus.h"

namespace {

struct DispatchedAction {
  uint64_t node_id;
  FlutterSemanticsAction action;
  std::vector<uint8_t> data;
};

// A semantics node that can be added to an update.
struct TestNode {
  explicit TestNode(int32_t id, std::vector<int32_t> children = {})
      : children(std::move(children)) {
    flags.struct_size = sizeof(flags);
    node.struct_size = sizeof(node);
    node.id = id;
    node.label = "";
    node.hint = "";
    node.value = "";
    node.tooltip = "";
    node.identifier = "";
    node.increased_value = "";
    node.decreased_value = "";
    node.text_selection_base = -1;
    node.text_selection_extent = -1;
    node.rect = {0, 0, 100, 50};
    node.transform.scaleX = 1;
    node.transform.scaleY = 1;
    node.transform.pers2 = 1;
    node.flags2 = &flags;
  }

  void SetLabel(std::string value) {
    label = std::move(value);
    node.label = label.c_str();
  }

  void SetValue(std::string text) {
    value = std::move(text);
    node.value = value.c_str();
  }

  FlutterSemanticsNode2* Get() {
    node.child_count = children.size();
    node.children_in_traversal_order = children.data();
    node.children_in_hit_test_order = children.data();
    return &node;
  }

  FlutterSemanticsNode2 node = {};
  FlutterSemanticsFlags flags = {};
  std::vector<int32_t> children;
  std::string label;
  std::string value;
};

class FlViewAccessibleTest : public flutter::testing::LinuxTest {
 protected:
  void SetUp() override {
    StartEngine();
    fl_engine_get_embedder_api(engine)->SendSemanticsAction = MOCK_ENGINE_PROC(
        SendSemanticsAction,
        ([actions = &actions](auto engine,
                              const FlutterSendSemanticsActionInfo* info) {
          actions->push_back({info->node_id, info->action,
                              std::vector<uint8_t>(
                                  info->data, info->data + info->data_length)});
          return kSuccess;
        }));

    bus = std::make_unique<flutter::testing::AtspiTestBus>();
    application =
        fl_atspi_application_new_with_connection(bus->app_connection());
    accessible = fl_view_accessible_new(engine, application, 456);
    view = FL_ATSPI_ACCESSIBLE(accessible);
  }

  void TearDown() override {
    g_clear_object(&accessible);
    g_clear_object(&application);
    bus.reset();
    LinuxTest::TearDown();
  }

  void Apply(std::vector<TestNode*> test_nodes,
             std::vector<FlutterSemanticsCustomAction2*> custom_actions = {}) {
    std::vector<FlutterSemanticsNode2*> nodes;
    for (TestNode* node : test_nodes) {
      nodes.push_back(node->Get());
    }
    FlutterSemanticsUpdate2 update = {};
    update.struct_size = sizeof(update);
    update.node_count = nodes.size();
    update.nodes = nodes.data();
    update.custom_action_count = custom_actions.size();
    update.custom_actions = custom_actions.data();
    fl_view_accessible_handle_update_semantics(accessible, &update);
  }

  FlAtspiAccessible* Node(int32_t id) {
    return FL_ATSPI_ACCESSIBLE(fl_view_accessible_lookup_node(accessible, id));
  }

  std::string Name(FlAtspiAccessible* object) {
    g_autofree gchar* name = fl_atspi_accessible_get_name(object);
    return name;
  }

  bool HasState(FlAtspiAccessible* object, FlAtspiState state) {
    return (fl_atspi_accessible_get_states(object) &
            FL_ATSPI_STATE_BIT(state)) != 0;
  }

  // Labels the D-Bus path of |object| and its descendants, with the view as
  // "/w" and nodes as "/w/<id>".
  void AddLabels(std::map<std::string, std::string>& labels,
                 FlAtspiAccessible* object,
                 const std::string& path) {
    labels[path] = object == view
                       ? "/w"
                       : "/w/" + std::to_string(fl_accessible_node_get_id(
                                     FL_ACCESSIBLE_NODE(object)));
    for (gsize i = 0; i < fl_atspi_accessible_get_n_children(object); i++) {
      AddLabels(labels, fl_atspi_accessible_get_child(object, i),
                bus->GetChildPath(path, i));
    }
  }

  // Events received, see AtspiTestBus::Describe.
  std::vector<std::string> Events() {
    bus->Sync();
    std::map<std::string, std::string> labels;
    AddLabels(labels, view,
              bus->GetChildPath("/org/a11y/atspi/accessible/root", 0));
    return bus->Describe(labels);
  }

  std::string EventData(size_t index) {
    const auto& signal = bus->signals()[index];
    g_autoptr(GVariant) data = nullptr;
    g_variant_get(signal.parameters, "(siiva{sv})", nullptr, nullptr, nullptr,
                  &data, nullptr);
    return g_variant_get_string(data, nullptr);
  }

  void ClearEvents() {
    bus->Sync();
    bus->ClearSignals();
  }

  std::unique_ptr<flutter::testing::AtspiTestBus> bus;
  FlAtspiApplication* application = nullptr;
  FlViewAccessible* accessible = nullptr;
  FlAtspiAccessible* view = nullptr;
  std::vector<DispatchedAction> actions;
};

}  // namespace

TEST_F(FlViewAccessibleTest, BuildTree) {
  TestNode root(0, {1, 2});
  TestNode button(1);
  button.SetLabel("OK");
  button.flags.is_button = true;
  button.flags.is_enabled = kFlutterTristateTrue;
  TestNode text(2);
  text.SetLabel("Hello");
  Apply({&root, &button, &text});

  ASSERT_EQ(fl_atspi_accessible_get_n_children(view), 1u);
  FlAtspiAccessible* root_object = fl_atspi_accessible_get_child(view, 0);
  ASSERT_NE(root_object, nullptr);
  EXPECT_EQ(fl_atspi_accessible_get_parent(root_object), view);
  EXPECT_EQ(fl_atspi_accessible_get_role(root_object), FL_ATSPI_ROLE_PANEL);
  ASSERT_EQ(fl_atspi_accessible_get_n_children(root_object), 2u);

  FlAtspiAccessible* button_object =
      fl_atspi_accessible_get_child(root_object, 0);
  EXPECT_EQ(fl_atspi_accessible_get_role(button_object), FL_ATSPI_ROLE_BUTTON);
  EXPECT_EQ(Name(button_object), "OK");
  EXPECT_EQ(fl_atspi_accessible_get_index_in_parent(button_object), 0);
  EXPECT_EQ(fl_atspi_accessible_get_parent(button_object), root_object);
  EXPECT_TRUE(HasState(button_object, FL_ATSPI_STATE_ENABLED));
  EXPECT_TRUE(HasState(button_object, FL_ATSPI_STATE_SHOWING));

  FlAtspiAccessible* text_object =
      fl_atspi_accessible_get_child(root_object, 1);
  EXPECT_EQ(fl_atspi_accessible_get_role(text_object), FL_ATSPI_ROLE_LABEL);
  EXPECT_EQ(Name(text_object), "Hello");
  EXPECT_EQ(fl_atspi_accessible_get_index_in_parent(text_object), 1);

  EXPECT_EQ(Events(), std::vector<std::string>{"/w ChildrenChanged:add 0"});
}

TEST_F(FlViewAccessibleTest, Roles) {
  TestNode root(0, {1, 2, 3, 4, 5});
  TestNode field(1);
  field.flags.is_text_field = true;
  TestNode password(2);
  password.flags.is_text_field = true;
  password.flags.is_obscured = true;
  TestNode check(3);
  check.flags.is_checked = kFlutterCheckStateTrue;
  TestNode radio(4);
  radio.flags.is_checked = kFlutterCheckStateFalse;
  radio.flags.is_in_mutually_exclusive_group = true;
  TestNode toggle(5);
  toggle.flags.is_toggled = kFlutterTristateTrue;
  Apply({&root, &field, &password, &check, &radio, &toggle});

  EXPECT_EQ(fl_atspi_accessible_get_role(Node(1)), FL_ATSPI_ROLE_ENTRY);
  EXPECT_EQ(fl_atspi_accessible_get_role(Node(2)), FL_ATSPI_ROLE_PASSWORD_TEXT);
  EXPECT_EQ(fl_atspi_accessible_get_role(Node(3)), FL_ATSPI_ROLE_CHECK_BOX);
  EXPECT_TRUE(HasState(Node(3), FL_ATSPI_STATE_CHECKED));
  EXPECT_EQ(fl_atspi_accessible_get_role(Node(4)), FL_ATSPI_ROLE_RADIO_BUTTON);
  EXPECT_FALSE(HasState(Node(4), FL_ATSPI_STATE_CHECKED));
  EXPECT_EQ(fl_atspi_accessible_get_role(Node(5)), FL_ATSPI_ROLE_SWITCH);
  EXPECT_TRUE(HasState(Node(5), FL_ATSPI_STATE_CHECKED));
}

TEST_F(FlViewAccessibleTest, Extents) {
  TestNode root(0, {1});
  root.node.rect = {0, 0, 800, 600};
  TestNode child(1);
  child.node.rect = {0, 0, 20, 10};
  child.node.transform.transX = 30;
  child.node.transform.transY = 40;
  Apply({&root, &child});

  GdkRectangle extents;
  ASSERT_TRUE(fl_atspi_accessible_get_extents(Node(1), &extents));
  EXPECT_EQ(extents.x, 30);
  EXPECT_EQ(extents.y, 40);
  EXPECT_EQ(extents.width, 20);
  EXPECT_EQ(extents.height, 10);
  EXPECT_EQ(fl_atspi_accessible_get_accessible_at_point(Node(0), 35, 45),
            Node(1));
  EXPECT_EQ(fl_atspi_accessible_get_accessible_at_point(Node(0), 5, 5),
            Node(0));
}

TEST_F(FlViewAccessibleTest, ChangeEvents) {
  TestNode root(0, {1});
  TestNode check(1);
  check.SetLabel("Option");
  check.flags.is_checked = kFlutterCheckStateFalse;
  Apply({&root, &check});
  ClearEvents();

  check.SetLabel("Choice");
  check.flags.is_checked = kFlutterCheckStateTrue;
  Apply({&check});
  EXPECT_EQ(Events(),
            (std::vector<std::string>{"/w/1 PropertyChange:accessible-name 0",
                                      "/w/1 StateChanged:checked 1"}));
  EXPECT_EQ(EventData(0), "Choice");
}

TEST_F(FlViewAccessibleTest, AddRemoveChildren) {
  TestNode root(0, {1, 2});
  TestNode a(1, {3});
  TestNode b(2);
  TestNode c(3);
  Apply({&root, &a, &b, &c});
  ClearEvents();

  // Remove 1 (and its child 3), add 4.
  root.children = {2, 4};
  TestNode d(4);
  Apply({&root, &d});
  EXPECT_EQ(Events(), (std::vector<std::string>{"/w/0 ChildrenChanged:remove 0",
                                                "/w/0 ChildrenChanged:add 1"}));
  EXPECT_EQ(Node(1), nullptr);
  EXPECT_EQ(Node(3), nullptr);
  ASSERT_NE(Node(4), nullptr);
  EXPECT_EQ(fl_atspi_accessible_get_index_in_parent(Node(4)), 1);
  EXPECT_EQ(fl_atspi_accessible_get_parent(Node(4)), Node(0));
}

TEST_F(FlViewAccessibleTest, MoveChild) {
  TestNode root(0, {1, 2});
  TestNode a(1, {3});
  TestNode b(2);
  TestNode c(3);
  Apply({&root, &a, &b, &c});

  // Move 3 from 1 to 2, and remove 1.
  root.children = {2};
  b.children = {3};
  Apply({&root, &b});
  EXPECT_EQ(Node(1), nullptr);
  ASSERT_NE(Node(3), nullptr);
  EXPECT_EQ(fl_atspi_accessible_get_parent(Node(3)), Node(2));
}

TEST_F(FlViewAccessibleTest, Focus) {
  TestNode root(0, {1, 2});
  TestNode a(1);
  a.flags.is_focused = kFlutterTristateTrue;
  TestNode b(2);
  b.flags.is_focused = kFlutterTristateFalse;
  Apply({&root, &a, &b});
  EXPECT_EQ(Events(),
            (std::vector<std::string>{"/w ChildrenChanged:add 0",
                                      "/w/1 StateChanged:focused 1"}));
  ClearEvents();

  a.flags.is_focused = kFlutterTristateFalse;
  b.flags.is_focused = kFlutterTristateTrue;
  Apply({&a, &b});
  EXPECT_EQ(Events(),
            (std::vector<std::string>{"/w/1 StateChanged:focused 0",
                                      "/w/2 StateChanged:focused 1"}));
}

TEST_F(FlViewAccessibleTest, TextChanges) {
  TestNode root(0, {1});
  TestNode field(1);
  field.flags.is_text_field = true;
  field.SetValue("hello");
  field.node.text_selection_base = 5;
  field.node.text_selection_extent = 5;
  Apply({&root, &field});
  FlAtspiAccessible* object = Node(1);
  EXPECT_TRUE(fl_atspi_accessible_has_text(object));
  g_autofree gchar* text = fl_atspi_accessible_get_text(object);
  EXPECT_STREQ(text, "hello");
  EXPECT_EQ(fl_atspi_accessible_get_caret_offset(object), 5);
  EXPECT_TRUE(HasState(object, FL_ATSPI_STATE_EDITABLE));
  ClearEvents();

  field.SetValue("help me");
  field.node.text_selection_base = 7;
  field.node.text_selection_extent = 7;
  Apply({&field});
  EXPECT_EQ(Events(), (std::vector<std::string>{"/w/1 TextChanged:delete 3",
                                                "/w/1 TextChanged:insert 3",
                                                "/w/1 TextCaretMoved 7"}));
  EXPECT_EQ(EventData(0), "lo");
  EXPECT_EQ(EventData(1), "p me");
  ClearEvents();

  field.node.text_selection_base = 0;
  Apply({&field});
  EXPECT_EQ(Events(), std::vector<std::string>{"/w/1 TextSelectionChanged 0"});
  gint start, end;
  ASSERT_TRUE(fl_atspi_accessible_get_selection(object, &start, &end));
  EXPECT_EQ(start, 0);
  EXPECT_EQ(end, 7);
}

TEST_F(FlViewAccessibleTest, Actions) {
  TestNode root(0, {1});
  TestNode button(1);
  button.node.actions = static_cast<FlutterSemanticsAction>(
      kFlutterSemanticsActionTap | kFlutterSemanticsActionLongPress |
      kFlutterSemanticsActionFocus);
  Apply({&root, &button});

  FlAtspiAccessible* object = Node(1);
  g_autoptr(GPtrArray) node_actions = fl_atspi_accessible_get_actions(object);
  ASSERT_EQ(node_actions->len, 2u);
  EXPECT_STREQ(
      static_cast<FlAtspiAction*>(g_ptr_array_index(node_actions, 0))->name,
      "click");
  EXPECT_STREQ(
      static_cast<FlAtspiAction*>(g_ptr_array_index(node_actions, 1))->name,
      "long-press");
  EXPECT_TRUE(HasState(object, FL_ATSPI_STATE_FOCUSABLE));

  EXPECT_TRUE(fl_atspi_accessible_do_action(object, 1));
  EXPECT_FALSE(fl_atspi_accessible_do_action(object, 2));
  EXPECT_TRUE(fl_atspi_accessible_grab_focus(object));
  ASSERT_EQ(actions.size(), 2u);
  EXPECT_EQ(actions[0].node_id, 1u);
  EXPECT_EQ(actions[0].action, kFlutterSemanticsActionLongPress);
  EXPECT_EQ(actions[1].action, kFlutterSemanticsActionFocus);
}

TEST_F(FlViewAccessibleTest, CustomActions) {
  TestNode root(0, {1});
  TestNode node(1);
  node.node.actions = kFlutterSemanticsActionCustomAction;
  int32_t custom_ids[] = {7};
  node.node.custom_accessibility_actions = custom_ids;
  node.node.custom_accessibility_actions_count = 1;
  FlutterSemanticsCustomAction2 custom = {};
  custom.struct_size = sizeof(custom);
  custom.id = 7;
  custom.label = "Archive";
  Apply({&root, &node}, {&custom});

  g_autoptr(GPtrArray) node_actions = fl_atspi_accessible_get_actions(Node(1));
  ASSERT_EQ(node_actions->len, 1u);
  EXPECT_STREQ(
      static_cast<FlAtspiAction*>(g_ptr_array_index(node_actions, 0))->name,
      "Archive");
  EXPECT_TRUE(fl_atspi_accessible_do_action(Node(1), 0));
  ASSERT_EQ(actions.size(), 1u);
  EXPECT_EQ(actions[0].action, kFlutterSemanticsActionCustomAction);
  g_autoptr(FlStandardMessageCodec) codec = fl_standard_message_codec_new();
  g_autoptr(GBytes) message =
      g_bytes_new(actions[0].data.data(), actions[0].data.size());
  g_autoptr(FlValue) data = fl_message_codec_decode_message(
      FL_MESSAGE_CODEC(codec), message, nullptr);
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(fl_value_get_type(data), FL_VALUE_TYPE_INT);
  EXPECT_EQ(fl_value_get_int(data), 7);
}

TEST_F(FlViewAccessibleTest, Value) {
  TestNode root(0, {1});
  TestNode slider(1);
  slider.flags.is_slider = true;
  slider.SetValue("40%");
  slider.node.actions = static_cast<FlutterSemanticsAction>(
      kFlutterSemanticsActionIncrease | kFlutterSemanticsActionDecrease);
  Apply({&root, &slider});

  FlAtspiAccessible* object = Node(1);
  g_autoptr(FlAtspiValue) value = fl_atspi_accessible_get_value(object);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->current, 40);
  EXPECT_EQ(value->maximum, 100);
  EXPECT_TRUE(fl_atspi_accessible_set_value(object, 50));
  EXPECT_TRUE(fl_atspi_accessible_set_value(object, 10));
  ASSERT_EQ(actions.size(), 2u);
  EXPECT_EQ(actions[0].action, kFlutterSemanticsActionIncrease);
  EXPECT_EQ(actions[1].action, kFlutterSemanticsActionDecrease);
}

TEST_F(FlViewAccessibleTest, SetValueSteps) {
  TestNode root(0, {1});
  TestNode slider(1);
  slider.flags.is_slider = true;
  slider.SetValue("40%");
  slider.node.actions = static_cast<FlutterSemanticsAction>(
      kFlutterSemanticsActionIncrease | kFlutterSemanticsActionDecrease);
  Apply({&root, &slider});

  FlAtspiAccessible* object = Node(1);
  EXPECT_TRUE(fl_atspi_accessible_set_value(object, 65));
  ASSERT_EQ(actions.size(), 1u);

  // Steps until the target is reached or passed.
  slider.SetValue("50%");
  Apply({&slider});
  ASSERT_EQ(actions.size(), 2u);
  EXPECT_EQ(actions[1].action, kFlutterSemanticsActionIncrease);
  slider.SetValue("60%");
  Apply({&slider});
  ASSERT_EQ(actions.size(), 3u);
  slider.SetValue("70%");
  Apply({&slider});
  slider.SetValue("80%");
  Apply({&slider});
  EXPECT_EQ(actions.size(), 3u);

  // Stops when the value doesn't change, e.g. at a limit.
  EXPECT_TRUE(fl_atspi_accessible_set_value(object, 0));
  ASSERT_EQ(actions.size(), 4u);
  EXPECT_EQ(actions[3].action, kFlutterSemanticsActionDecrease);
  Apply({&slider});
  EXPECT_EQ(actions.size(), 4u);
  slider.SetValue("70%");
  Apply({&slider});
  EXPECT_EQ(actions.size(), 4u);
}

TEST_F(FlViewAccessibleTest, Routes) {
  TestNode root(0, {1});
  TestNode page(1, {2, 3});
  page.flags.scopes_route = true;
  TestNode title(2);
  title.flags.names_route = true;
  title.SetLabel("Settings");
  TestNode dialog(3, {4});
  dialog.flags.scopes_route = true;
  dialog.flags.names_route = true;
  dialog.SetLabel("Confirm");
  TestNode dialog_title(4);
  dialog_title.flags.names_route = true;
  dialog_title.SetLabel("Inner");
  Apply({&root, &page, &title, &dialog, &dialog_title});

  EXPECT_EQ(fl_atspi_accessible_get_role(Node(1)), FL_ATSPI_ROLE_PANEL);
  EXPECT_EQ(Name(Node(1)), "Settings");
  EXPECT_EQ(fl_atspi_accessible_get_role(Node(3)), FL_ATSPI_ROLE_DIALOG);
  EXPECT_EQ(Name(Node(3)), "Confirm");
}

TEST_F(FlViewAccessibleTest, RadioGroupRelations) {
  TestNode root(0, {1, 2, 3});
  TestNode radio1(1);
  radio1.flags.is_checked = kFlutterCheckStateTrue;
  radio1.flags.is_in_mutually_exclusive_group = true;
  TestNode radio2(2);
  radio2.flags.is_checked = kFlutterCheckStateFalse;
  radio2.flags.is_in_mutually_exclusive_group = true;
  TestNode other(3);
  other.flags.is_checked = kFlutterCheckStateFalse;
  Apply({&root, &radio1, &radio2, &other});

  g_autoptr(GPtrArray) relations = fl_atspi_accessible_get_relations(Node(2));
  ASSERT_EQ(relations->len, 1u);
  FlAtspiRelation* relation =
      static_cast<FlAtspiRelation*>(g_ptr_array_index(relations, 0));
  EXPECT_EQ(relation->type, FL_ATSPI_RELATION_MEMBER_OF);
  ASSERT_EQ(relation->targets->len, 2u);
  EXPECT_EQ(g_ptr_array_index(relation->targets, 0), Node(1));
  EXPECT_EQ(g_ptr_array_index(relation->targets, 1), Node(2));
  g_autoptr(GPtrArray) other_relations =
      fl_atspi_accessible_get_relations(Node(3));
  EXPECT_EQ(other_relations->len, 0u);
}

TEST_F(FlViewAccessibleTest, Selection) {
  TestNode root(0, {1});
  TestNode tabs(1, {2, 3});
  TestNode tab1(2);
  tab1.flags.is_selected = kFlutterTristateTrue;
  tab1.node.actions = kFlutterSemanticsActionTap;
  TestNode tab2(3);
  tab2.flags.is_selected = kFlutterTristateFalse;
  tab2.node.actions = kFlutterSemanticsActionTap;
  Apply({&root, &tabs, &tab1, &tab2});

  FlAtspiAccessible* container = Node(1);
  EXPECT_FALSE(fl_atspi_accessible_is_selection_container(Node(0)));
  EXPECT_TRUE(fl_atspi_accessible_is_selection_container(container));
  {
    g_autoptr(GArray) selected =
        fl_atspi_accessible_get_selected_children(container);
    ASSERT_EQ(selected->len, 1u);
    EXPECT_EQ(g_array_index(selected, guint, 0), 0u);
  }
  EXPECT_TRUE(fl_atspi_accessible_select_child(container, 0));
  EXPECT_TRUE(actions.empty());
  EXPECT_TRUE(fl_atspi_accessible_select_child(container, 1));
  ASSERT_EQ(actions.size(), 1u);
  EXPECT_EQ(actions[0].node_id, 3u);
  EXPECT_EQ(actions[0].action, kFlutterSemanticsActionTap);
  EXPECT_FALSE(fl_atspi_accessible_select_child(container, 2));

  ClearEvents();
  tab1.flags.is_selected = kFlutterTristateFalse;
  tab2.flags.is_selected = kFlutterTristateTrue;
  Apply({&tab1, &tab2});
  {
    g_autoptr(GArray) selected =
        fl_atspi_accessible_get_selected_children(container);
    ASSERT_EQ(selected->len, 1u);
    EXPECT_EQ(g_array_index(selected, guint, 0), 1u);
  }
  std::vector<std::string> events = Events();
  EXPECT_EQ(std::count(events.begin(), events.end(), "/w/1 SelectionChanged 0"),
            1);
}

TEST_F(FlViewAccessibleTest, ImageAndPlaceholder) {
  TestNode root(0, {1, 2});
  TestNode image(1);
  image.flags.is_image = true;
  image.SetLabel("Logo");
  TestNode field(2);
  field.flags.is_text_field = true;
  field.node.hint = "Name";
  Apply({&root, &image, &field});

  EXPECT_TRUE(fl_atspi_accessible_is_image(Node(1)));
  EXPECT_FALSE(fl_atspi_accessible_is_image(Node(2)));
  g_autoptr(GHashTable) attributes =
      fl_atspi_accessible_get_attributes(Node(2));
  EXPECT_STREQ(static_cast<const gchar*>(
                   g_hash_table_lookup(attributes, "placeholder-text")),
               "Name");
}

TEST_F(FlViewAccessibleTest, Announce) {
  fl_view_accessible_send_announcement(accessible, "Saved", TRUE);
  EXPECT_EQ(Events(), std::vector<std::string>{"/w Announcement 2"});
  EXPECT_EQ(EventData(0), "Saved");
}

TEST_F(FlViewAccessibleTest, Inactive) {
  g_clear_object(&accessible);
  g_clear_object(&application);
  application = fl_atspi_application_new();
  accessible = fl_view_accessible_new(engine, application, 456);
  view = FL_ATSPI_ACCESSIBLE(accessible);
  TestNode root(0);
  Apply({&root});
  fl_view_accessible_send_announcement(accessible, "Hello", FALSE);
  bus->Sync();
  EXPECT_TRUE(bus->signals().empty());
  EXPECT_EQ(fl_atspi_accessible_get_n_children(view), 1u);
}
