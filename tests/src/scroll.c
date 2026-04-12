struct scroll_global_state
{
    enum window_insertion_point window_insertion_point;
    uint32_t focused_window_id;
};

static struct scroll_global_state push_scroll_global_state(enum window_insertion_point window_insertion_point, uint32_t focused_window_id)
{
    struct scroll_global_state state = {
        .window_insertion_point = g_space_manager.window_insertion_point,
        .focused_window_id = g_window_manager.focused_window_id,
    };

    g_space_manager.window_insertion_point = window_insertion_point;
    g_window_manager.focused_window_id = focused_window_id;

    return state;
}

static void pop_scroll_global_state(struct scroll_global_state state)
{
    g_space_manager.window_insertion_point = state.window_insertion_point;
    g_window_manager.focused_window_id = state.focused_window_id;
}

TEST_FUNC(scroll_window_order_navigation,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.focused_index = 1;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 50 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 120, .h = 50 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 33, .w = 140, .h = 50 }));

    TEST_CHECK(view_find_window_index(&view, 11), 0);
    TEST_CHECK(view_find_window_index(&view, 22), 1);
    TEST_CHECK(view_find_window_index(&view, 33), 2);
    TEST_CHECK(view_find_first_window_id(&view), 11);
    TEST_CHECK(view_find_last_window_id(&view), 33);
    TEST_CHECK(view_find_prev_window_id(&view, 22), 11);
    TEST_CHECK(view_find_next_window_id(&view, 22), 33);
    TEST_CHECK(view_find_prev_window_id(&view, 11), 0);
    TEST_CHECK(view_find_next_window_id(&view, 33), 0);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(scroll_viewport_centering_and_edge_peek,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.x = 0;
    view.scroll.area.y = 0;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 0;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .x = 0,   .w = 120, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .x = 130, .w = 120, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 33, .x = 400, .w = 120, .h = 100 }));

    scroll_view_center_on_index(&view, 1);
    TEST_CHECK((int) view.scroll.viewport_x, 40);

    scroll_view_center_on_index(&view, 2);
    TEST_CHECK((int) view.scroll.viewport_x, 220);

    view.scroll.viewport_x = 250;
    TEST_CHECK((int) scroll_view_adjust_window_x(&view.scroll, &view.scroll.column_list[0]), -112);
    TEST_CHECK((int) scroll_view_adjust_window_x(&view.scroll, &view.scroll.column_list[2]), 150);

    view.scroll.viewport_x = 0;
    TEST_CHECK((int) scroll_view_adjust_window_x(&view.scroll, &view.scroll.column_list[2]), 292);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(scroll_focus_index_after_removal,
{
    TEST_CHECK(scroll_view_focus_index_after_removal(3, 1, 1), 1);
    TEST_CHECK(scroll_view_focus_index_after_removal(3, 2, 2), 1);
    TEST_CHECK(scroll_view_focus_index_after_removal(3, 0, 2), 1);
    TEST_CHECK(scroll_view_focus_index_after_removal(3, 2, 0), 0);
    TEST_CHECK(scroll_view_focus_index_after_removal(1, 0, 0), -1);
});

TEST_FUNC(scroll_focus_state_is_idempotent,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 0;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .x = 0,   .w = 120, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .x = 130, .w = 120, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 33, .x = 260, .w = 120, .h = 100 }));

    TEST_CHECK(view_set_focused_window(&view, 22), true);
    TEST_CHECK(view.scroll.focused_index, 1);
    TEST_CHECK((int) view.scroll.viewport_x, 40);

    view_clear_flag(&view, VIEW_IS_DIRTY);
    TEST_CHECK(view_set_focused_window(&view, 22), false);
    TEST_CHECK(view_is_dirty(&view), false);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(scroll_step_rejects_unsupported_directions,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.focused_index = 0;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 100, .h = 100 }));

    TEST_CHECK(view_scroll_step(&view, DIR_NORTH), false);
    TEST_CHECK(view.scroll.focused_index, 0);
    TEST_CHECK(view_is_dirty(&view), false);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(scroll_background_insert_preserves_focus,
{
    struct scroll_global_state state = push_scroll_global_state(INSERT_FIRST, 22);

    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 1;

    struct window window = { .id = 33 };

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 100, .h = 100 }));

    view_add_window_node_with_insertion_point(&view, &window, 0);

    TEST_CHECK(view.scroll.column_list[0].window_id, 33);
    TEST_CHECK(view.scroll.column_list[2].window_id, 22);
    TEST_CHECK(view.scroll.focused_index, 2);

    buf_free(view.scroll.column_list);
    pop_scroll_global_state(state);
});

TEST_FUNC(scroll_focused_insert_updates_focus,
{
    struct scroll_global_state state = push_scroll_global_state(INSERT_LAST, 33);

    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 0;

    struct window window = { .id = 33 };

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 100, .h = 100 }));

    view_add_window_node_with_insertion_point(&view, &window, 0);

    TEST_CHECK(view.scroll.column_list[2].window_id, 33);
    TEST_CHECK(view.scroll.focused_index, 2);

    buf_free(view.scroll.column_list);
    pop_scroll_global_state(state);
});

TEST_FUNC(scroll_warp_moves_in_both_directions,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 1;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 33, .w = 100, .h = 100 }));

    TEST_CHECK(view_warp_window_order(&view, 22, 33), true);
    TEST_CHECK(view.scroll.column_list[0].window_id, 11);
    TEST_CHECK(view.scroll.column_list[1].window_id, 33);
    TEST_CHECK(view.scroll.column_list[2].window_id, 22);
    TEST_CHECK(view.scroll.focused_index, 2);

    TEST_CHECK(view_warp_window_order(&view, 22, 33), true);
    TEST_CHECK(view.scroll.column_list[0].window_id, 11);
    TEST_CHECK(view.scroll.column_list[1].window_id, 22);
    TEST_CHECK(view.scroll.column_list[2].window_id, 33);
    TEST_CHECK(view.scroll.focused_index, 1);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(scroll_warp_preserves_unmoved_focus,
{
    struct view view = {0};
    view.layout = VIEW_SCROLL;
    view.scroll.area.w = 300;
    view.scroll.area.h = 100;
    view.scroll.focused_index = 2;

    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 11, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 22, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 33, .w = 100, .h = 100 }));
    buf_push(view.scroll.column_list, ((struct scroll_column) { .window_id = 44, .w = 100, .h = 100 }));

    TEST_CHECK(view_warp_window_order(&view, 11, 44), true);
    TEST_CHECK(view.scroll.column_list[1].window_id, 33);
    TEST_CHECK(view.scroll.focused_index, 1);

    TEST_CHECK(view_warp_window_order(&view, 44, 33), true);
    TEST_CHECK(view.scroll.column_list[2].window_id, 33);
    TEST_CHECK(view.scroll.focused_index, 2);

    buf_free(view.scroll.column_list);
});

TEST_FUNC(display_local_user_space_navigation,
{
    uint64_t user_space_list[3];
    user_space_list[0] = 101;
    user_space_list[1] = 202;
    user_space_list[2] = 303;

    TEST_CHECK((int) space_manager_prev_user_space_in_list(user_space_list, array_count(user_space_list), 101), 0);
    TEST_CHECK((int) space_manager_prev_user_space_in_list(user_space_list, array_count(user_space_list), 202), 101);
    TEST_CHECK((int) space_manager_prev_user_space_in_list(user_space_list, array_count(user_space_list), 303), 202);

    TEST_CHECK((int) space_manager_next_user_space_in_list(user_space_list, array_count(user_space_list), 101), 202);
    TEST_CHECK((int) space_manager_next_user_space_in_list(user_space_list, array_count(user_space_list), 202), 303);
    TEST_CHECK((int) space_manager_next_user_space_in_list(user_space_list, array_count(user_space_list), 303), 0);
});
