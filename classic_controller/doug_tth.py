from machine import Pin

BTN_A = 6
BTN_Y = 3
BTN_SPIN = 2
BTN_LEFT = 7
BTN_RIGHT = 10
BTN_UP = 8
BTN_DOWN = 9

PRESS = 0
RELEASE = 1
DELAY = 2

# Even configure unused pins as inputs
buttons = [None] * 11
for i in range(11):
    buttons[i] = Pin(i, Pin.OPEN_DRAIN, pull=None)
    buttons[i].value(1)

# Press button
def press(btn):
    global buttons
    buttons[btn].value(0)
def release(btn):
    global buttons
    buttons[btn].value(1)

actionIndex = 0
actions = [
    # Menuing to start over
    (RELEASE, BTN_A),
    (RELEASE, BTN_DOWN),
    (DELAY, 4),
    (PRESS, BTN_DOWN),
    (DELAY, 4),
    (RELEASE, BTN_DOWN),
    (DELAY, 3),
    (PRESS, BTN_A),
    (DELAY, 3),
    (RELEASE, BTN_A),
    # Start of actual TAS
    (DELAY, 212),
    (PRESS, BTN_Y),
    (PRESS, BTN_SPIN),
    (DELAY, 16),
    (PRESS, BTN_RIGHT),
    (DELAY, 7),
    (PRESS, BTN_DOWN),
    (DELAY, 19),
    (RELEASE, BTN_RIGHT),
    (DELAY, 19),
    (PRESS, BTN_LEFT),
    (DELAY, 16),
    (RELEASE, BTN_LEFT),
    (RELEASE, BTN_SPIN),
    (DELAY, 3),
    (PRESS, BTN_RIGHT),
    (DELAY, 2),
    (RELEASE, BTN_RIGHT),
    (DELAY, 4),
    (RELEASE, BTN_Y),
    (DELAY, 4),
    (PRESS, BTN_Y),
    (DELAY, 4),
    (PRESS, BTN_RIGHT),
    (DELAY, 21),
    (PRESS, BTN_A),
    (DELAY, 13),
    (RELEASE, BTN_RIGHT),
    (DELAY, 2),
    (PRESS, BTN_LEFT),
    (DELAY, 11),
    (RELEASE, BTN_A),
    (DELAY, 9),
    (RELEASE, BTN_LEFT),
    (DELAY, 1),
    (PRESS, BTN_RIGHT),
    (DELAY, 1),
    (RELEASE, BTN_RIGHT),
    (DELAY, 2),
    (RELEASE, BTN_Y),
    (DELAY, 2),
    (PRESS, BTN_Y),
    (DELAY, 11),
    (PRESS, BTN_RIGHT),
    (DELAY, 16),
    (PRESS, BTN_A),
    (DELAY, 14),
    (RELEASE, BTN_RIGHT),
    (DELAY, 1),
    (PRESS, BTN_LEFT),
    (DELAY, 7),
    (RELEASE, BTN_A),
    (DELAY, 4),
    (RELEASE, BTN_LEFT),
    (DELAY, 1),
    (PRESS, BTN_RIGHT),
    (DELAY, 1),
    (RELEASE, BTN_RIGHT),
    (DELAY, 2),
    (RELEASE, BTN_Y),
    (DELAY, 4),
    (PRESS, BTN_LEFT),
    (DELAY, 5),
    (RELEASE, BTN_LEFT),
    (DELAY, 5),
    (PRESS, BTN_RIGHT),
    (DELAY, 8),
    (PRESS, BTN_Y),
    (DELAY, 8),
    (PRESS, BTN_A),
    (DELAY, 7),
    (RELEASE, BTN_RIGHT),
    (DELAY, 29),
    (PRESS, BTN_LEFT),
    (DELAY, 17),
    (RELEASE, BTN_LEFT),
    (DELAY, 7),
    (PRESS, BTN_LEFT),
    (DELAY, 8),
    (RELEASE, BTN_LEFT),
    (DELAY, 37),
    (PRESS, BTN_RIGHT),
    (DELAY, 15),
    (RELEASE, BTN_RIGHT),
    (DELAY, 4),
    (RELEASE, BTN_A),
    (DELAY, 4),
    (PRESS, BTN_A),
    (DELAY, 23),
    (RELEASE, BTN_Y),
    (DELAY, 5),
    (PRESS, BTN_Y),
    (DELAY, 19),
    (PRESS, BTN_LEFT),
    (DELAY, 6),
    (RELEASE, BTN_LEFT),
    (DELAY, 5),
    (PRESS, BTN_LEFT),
    (DELAY, 68),
    (RELEASE, BTN_LEFT),
    (DELAY, 2),
    (PRESS, BTN_RIGHT),
    (DELAY, 30),
    (RELEASE, BTN_A),
    (DELAY, 15),
    (PRESS, BTN_A),
    (RELEASE, BTN_RIGHT),
    (DELAY, 9),
    (PRESS, BTN_LEFT),
    (DELAY, 11),
    (RELEASE, BTN_LEFT),
    (DELAY, 3),
    (RELEASE, BTN_A),
    (DELAY, 4),
    (RELEASE, BTN_Y),
    (DELAY, 2),
    (PRESS, BTN_A),
    (DELAY, 2),
    (RELEASE, BTN_A),
    (DELAY, 7),
    (PRESS, BTN_LEFT),
    (DELAY, 31),
    (PRESS, BTN_A),
    (DELAY, 3),
    (RELEASE, BTN_A),
    (PRESS, BTN_Y),
    (DELAY, 3),
    (PRESS, BTN_A),
    (DELAY, 4),
    (RELEASE, BTN_DOWN),
    (DELAY, 14),
    (RELEASE, BTN_LEFT),
    (DELAY, 2),
    (RELEASE, BTN_Y),
    (PRESS, BTN_RIGHT),
    (DELAY, 19),
    (RELEASE, BTN_RIGHT),
    (DELAY, 5),
    (RELEASE, BTN_A)
]
actionCount = len(actions)

def frame(pin):
    global actions
    global actionIndex
    global actionCount

    while (actionIndex < actionCount):
        (action, param) = actions[actionIndex]
        if action == PRESS:
            press(param)
            actionIndex += 1
        elif action == RELEASE:
            release(param)
            actionIndex += 1
        elif action == DELAY:
            if param == 0:
                actionIndex += 1
            else:
                actions[actionIndex] = (DELAY, param - 1)
                return

    pin.irq(handler=None)
    global buttons
    for i in range(11):
        release(i)

irqpin = Pin(11, Pin.IN)
irqpin.irq(trigger=Pin.IRQ_RISING, handler=frame)
