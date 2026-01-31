#include "TextScroller.h"

TextScroller::TextScroller() :
    last_change_time(0),
    last_scroll_time(0),
    reached_end_time(0),
    scroll_offset(0),
    scrolling_back(false),
    at_end(false) {
}

void TextScroller::reset() {
    scroll_offset = 0;
    last_change_time = millis();
    last_scroll_time = millis();
    reached_end_time = 0;
    scrolling_back = false;
    at_end = false;
}

int TextScroller::update(int text_width, int available_width) {
    // If text fits, no scrolling needed
    if (text_width <= available_width) {
        scroll_offset = 0;
        return 0;
    }

    int max_scroll = text_width - available_width;
    unsigned long now = millis();

    // Wait for initial delay before starting to scroll
    if (now - last_change_time < MENU_SCROLL_DELAY) {
        return scroll_offset;
    }

    // Handle end-of-scroll pause and reset
    if (scroll_offset >= max_scroll) {
        if (!at_end) {
            at_end = true;
            reached_end_time = now;
        }
        if (now - reached_end_time > MENU_SCROLL_DELAY) {
            scroll_offset = 0;
            at_end = false;
            last_change_time = now;  // Start delay again at beginning
        }
    } else {
        // Normal scrolling
        if (now - last_scroll_time >= MENU_SCROLL_SPEED) {
            last_scroll_time = now;
            scroll_offset++;
        }
    }

    return scroll_offset;
}
