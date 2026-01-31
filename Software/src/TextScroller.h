#ifndef TEXT_SCROLLER_H
#define TEXT_SCROLLER_H

#include <Arduino.h>
#include "settings.h"

class TextScroller {
public:
    TextScroller();

    // Call when the text content changes to reset scroll position
    void reset();

    // Update scroll state and return pixel offset for drawing
    // text_width: width of the full text in pixels
    // available_width: width available for display in pixels
    // Returns: pixel offset to subtract from x position when drawing
    int update(int text_width, int available_width);

private:
    unsigned long last_change_time;   // Time when text changed (for initial delay)
    unsigned long last_scroll_time;   // Time of last scroll step
    unsigned long reached_end_time;   // Time when scroll reached the end
    int scroll_offset;                // Current pixel offset
    bool scrolling_back;              // True if scrolling back to start
    bool at_end;                      // True if scroll has reached max offset
};

#endif // TEXT_SCROLLER_H
