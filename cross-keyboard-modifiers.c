// cross-keyboard-modifiers
//
// macOS tracks modifier keys per keyboard, so holding Shift on one keyboard does
// nothing to a key typed on another. This tiny event tap remembers which
// Shift / Control / Option / Command keys are held on ANY keyboard and adds them
// to every key event.
//
// Latency: only keyboard events are tapped (never mouse), the callback is a few
// integer ops with no allocation or syscalls, and it sleeps when you aren't typing.

#include <ApplicationServices/ApplicationServices.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// Left/right modifier bits that macOS puts in event flags (IOKit/hidsystem/IOLLEvent.h).
enum {
    L_CTRL  = 0x00000001, R_CTRL  = 0x00002000,
    L_SHIFT = 0x00000002, R_SHIFT = 0x00000004,
    L_CMD   = 0x00000008, R_CMD   = 0x00000010,
    L_OPT   = 0x00000020, R_OPT   = 0x00000040,
};

static CFMachPortRef tap;
static CGEventFlags held;   // left/right bits of modifiers held on any keyboard
static CGEventFlags extra;  // `held` plus the matching generic flags; OR'd into key events

// Virtual keycode -> its left/right bit (0 if it isn't a modifier we share).
static CGEventFlags side_bit(int64_t keycode) {
    switch (keycode) {
    case 0x38: return L_SHIFT;  case 0x3C: return R_SHIFT;
    case 0x3B: return L_CTRL;   case 0x3E: return R_CTRL;
    case 0x3A: return L_OPT;    case 0x3D: return R_OPT;
    case 0x37: return L_CMD;    case 0x36: return R_CMD;
    default:   return 0;
    }
}

static void update_extra(void) {
    extra = held;
    if (held & (L_SHIFT | R_SHIFT)) extra |= kCGEventFlagMaskShift;
    if (held & (L_CTRL  | R_CTRL))  extra |= kCGEventFlagMaskControl;
    if (held & (L_OPT   | R_OPT))   extra |= kCGEventFlagMaskAlternate;
    if (held & (L_CMD   | R_CMD))   extra |= kCGEventFlagMaskCommand;
}

static CGEventRef on_event(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *ctx) {
    (void)proxy; (void)ctx;

    if (type == kCGEventFlagsChanged) {
        // A modifier went up or down. The event's flags describe only the keyboard
        // it came from, so the key's own left/right bit says pressed vs released.
        CGEventFlags bit = side_bit(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        if (bit) {
            if (CGEventGetFlags(event) & bit) held |= bit;
            else                              held &= ~bit;
            update_extra();
        }
    } else if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        // We may have missed events while disabled; start clean so nothing sticks.
        held = 0;
        update_extra();
        CGEventTapEnable(tap, true);
        return event;
    }

    if (extra) {
        CGEventFlags flags = CGEventGetFlags(event);
        if ((flags | extra) != flags) CGEventSetFlags(event, flags | extra);
    }
    return event;
}

int main(void) {
    // Same scheduling class as the UI, so we're never the reason a key is late.
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                       CGEventMaskBit(kCGEventKeyUp) |
                       CGEventMaskBit(kCGEventFlagsChanged);

    // Needs Accessibility permission: ask once, then wait until it's granted.
    int asked = 0;
    while (!(tap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap,
                                    kCGEventTapOptionDefault, mask, on_event, NULL))) {
        if (!asked) {
            const void *keys[] = { kAXTrustedCheckOptionPrompt };
            const void *vals[] = { kCFBooleanTrue };
            CFDictionaryRef opts = CFDictionaryCreate(NULL, keys, vals, 1,
                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            AXIsProcessTrustedWithOptions(opts);
            CFRelease(opts);
            fprintf(stderr, "cross-keyboard-modifiers: waiting for Accessibility permission "
                            "(System Settings > Privacy & Security > Accessibility)\n");
            asked = 1;
        }
        sleep(3);
    }

    CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(NULL, tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);
    fprintf(stderr, "cross-keyboard-modifiers: running\n");
    CFRunLoopRun();
    return 0;
}
