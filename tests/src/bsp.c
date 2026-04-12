struct bsp_global_state
{
    bool window_zoom_persist;
    enum window_node_child window_placement;
    float split_ratio;
};

static struct bsp_global_state push_bsp_global_state(bool window_zoom_persist, enum window_node_child window_placement, float split_ratio)
{
    struct bsp_global_state state = {
        .window_zoom_persist = g_space_manager.window_zoom_persist,
        .window_placement = g_space_manager.window_placement,
        .split_ratio = g_space_manager.split_ratio,
    };

    g_space_manager.window_zoom_persist = window_zoom_persist;
    g_space_manager.window_placement = window_placement;
    g_space_manager.split_ratio = split_ratio;

    return state;
}

static void pop_bsp_global_state(struct bsp_global_state state)
{
    g_space_manager.window_zoom_persist = state.window_zoom_persist;
    g_space_manager.window_placement = state.window_placement;
    g_space_manager.split_ratio = state.split_ratio;
}

TEST_FUNC(bsp_insert_orders_above_zoomed_sibling,
{
    struct bsp_global_state state = push_bsp_global_state(true, CHILD_SECOND, 0.5f);

    struct view view = {0};
    struct window_node root = {0};
    struct window_node left = {0};
    struct window_node right = {0};
    struct window window = { .id = 22 };

    view.layout = VIEW_BSP;
    view.root = &root;
    view.split_type = SPLIT_Y;
    view.insertion_point = 11;

    root.area.x = 0;
    root.area.y = 0;
    root.area.w = 100;
    root.area.h = 100;
    root.left = &left;
    root.right = &right;
    root.split = SPLIT_Y;
    root.ratio = 0.5f;

    left.parent = &root;
    left.area.x = 0;
    left.area.y = 0;
    left.area.w = 50;
    left.area.h = 100;
    left.window_list[0] = 11;
    left.window_order[0] = 11;
    left.window_count = 1;
    left.zoom = &root;

    right.parent = &root;
    right.area.x = 50;
    right.area.y = 0;
    right.area.w = 50;
    right.area.h = 100;
    right.window_list[0] = 33;
    right.window_order[0] = 33;
    right.window_count = 1;

    memset(&g_test_scripting_addition_order_window, 0, sizeof(g_test_scripting_addition_order_window));

    view_add_window_node_with_insertion_point(&view, &window, 0);
    struct window_node *new_node = view_find_window_node(&view, 22);
    struct window_node *old_node = view_find_window_node(&view, 11);

    TEST_CHECK(new_node != NULL, true);
    TEST_CHECK(old_node != NULL, true);
    TEST_CHECK(old_node->zoom == new_node->parent, true);

    window_manager_order_window_after_insert(&view, &window);

    TEST_CHECK(g_test_scripting_addition_order_window.count, 1);
    TEST_CHECK(g_test_scripting_addition_order_window.a_wid, 22);
    TEST_CHECK(g_test_scripting_addition_order_window.order, 1);
    TEST_CHECK(g_test_scripting_addition_order_window.b_wid, 11);

    pop_bsp_global_state(state);
});
